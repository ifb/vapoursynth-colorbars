/*****************************************************************************
 * colorbars: a vapoursynth plugin for generating color bar test patterns
 *****************************************************************************
 * VapourSynth plugin
 *     Copyright (C) 2022 Phillip Blucas
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 *****************************************************************************/
#include <ctype.h>
#include <string.h>

#include <VapourSynth4.h>
#include <VSHelper4.h>
#include <VSConstants4.h>

#define RETERROR(x) do { vsapi->mapSetError(out, (x)); return; } while (0)

typedef enum {
    NTSC = 0,
    PAL,
    HD720,
    HD1080,
    DCI2K,
    UHDTV1,
    DCI4K,
    UHDTV2,
    NTSC_4FSC,
    PAL_4FSC
} system_type_e;

typedef enum {
    IQ_NONE = 0,
    IQ_BOTH,
    IQ_PLUS_I,
    IQ_WHITE
} iq_mode_e;

typedef struct {
    VSVideoInfo vi;
    system_type_e resolution;
    int hdr;
    int wcg;
    int compatability;
    int subblack;
    int superwhite;
    iq_mode_e iq;
    int halfline;
    int filter;
} ColorBarsData;

static const VSFrame *VS_CC colorbarsGetFrame (int n, int activationReason, void* instanceData, void** frameData, VSFrameContext* frameCtx, VSCore* core, const VSAPI* vsapi)
{
    ColorBarsData *d = (ColorBarsData*)instanceData;
    if (activationReason == arInitial)
    {
        // [wcg][bitdepth][value]
        // 0% Black, 75% Gray, 75% Yellow, 75% Cyan, 75% Green, 75% Magenta, 75% Red, 75% Blue, 0% Black
        const uint16_t ntsc1_y[2][9] = { {   64,  721,  646,  525,  450,  335,  260,  139,   64 },
                                         {  256, 2884, 2584, 2098, 1799, 1341, 1042,  556,  256 } };
        const uint16_t ntsc1_u[2][9] = { {  512,  512,  176,  625,  289,  735,  399,  848,  512 },
                                         { 2048, 2048,  704, 2502, 1158, 2938, 1594, 3392, 2048 } };
        const uint16_t ntsc1_v[2][9] = { {  512,  512,  567,  176,  231,  793,  848,  457,  512 },
                                         { 2048, 2048, 2267,  704,  923, 3173, 3392, 1829, 2048 } };

        // 0% Black, 75% Blue, 0% Black, 75% Magenta, 0% Black, 75% Cyan, 0% Black, 75% Gray, 0% Black
        const uint16_t ntsc2_y[2][9] = { {   64,  139,   64,  335,   64,  525,   64,  721,   64 },
                                         {  256,  556,  256, 1341,  256, 2098,  256, 2884,  256 } };
        const uint16_t ntsc2_u[2][9] = { {  512,  848,  512,  735,  512,  625,  512,  512,  512 },
                                         { 2048, 3392, 2048, 2938, 2048, 2502, 2048, 2048, 2048 } };
        const uint16_t ntsc2_v[2][9] = { {  512,  457,  512,  793,  512,  176,  512,  512,  512 },
                                         { 2048, 1829, 2048, 3173, 2048,  704, 2048, 2048, 2048 } };

        // Note that -I/+Q will be invalid if converted to RGB
        // 0% Black, -I, 100% White, +Q, 0% Black, -4% Black, 0% Black, +4% Black, 0% Black, 0% Black
        const uint16_t ntsc3_y[2][10] = { { 64,     64,  940,   64,   64,   29,   64,   99,   64,   64 },
                                          { 256,   256, 3760,  256,  256,  116,  256,  396,  256,  256 } };
        const uint16_t ntsc3_u[2][10] = { { 512,   633,  512,  698,  512,  512,  512,  512,  512,  512 },
                                          { 2048, 2532, 2048, 2793, 2048, 2048, 2048, 2048, 2048, 2048 } };
        const uint16_t ntsc3_v[2][10] = { { 512,   380,  512,  598,  512,  512,  512,  512,  512,  512 },
                                          { 2048, 1520, 2048, 2391, 2048, 2048, 2048, 2048, 2048, 2048 } };

        // 0% Black, 100% White, 75% Yellow, 75% Cyan, 75% Green, 75% Magenta, 75% Red, 75% Blue, 0% Black, 0% Black
        const uint16_t pal_y[2][10] = { { 64,    940,  646,  525,  450,  335,  260,  139,   64,   64 },
                                        { 256,  3760, 2584, 2098, 1799, 1341, 1042,  556,  256,  256 } };
        const uint16_t pal_u[2][10] = { { 512,   512,  176,  625,  289,  735,  399,  848,  512,  512 },
                                        { 2048, 2048,  704, 2502, 1158, 2938, 1594, 3392, 2048, 2048 } };
        const uint16_t pal_v[2][10] = { { 512,   512,  567,  176,  231,  793,  848,  457,  512,  512 },
                                        { 2048, 2048, 2267,  704,  923, 3173, 3392, 1829, 2048, 2048 } };

        // 40% Gray, 75% White, 75% Yellow, 75% Cyan, 75% Green, 75% Magenta, 75% Red, 75% Blue, 40% Gray
        const uint16_t p1_y[2][2][9] = { { {  414,  721,  674,  581,  534,  251,  204,  111,  414 },
                                           { 1658, 2884, 2694, 2325, 2136, 1004,  815,  446, 1658 } },
                                         { {  414,  721,  682,  548,  509,  276,  237,  103,  414 },
                                           { 1658, 2884, 2728, 2194, 2038, 1102,  946,  412, 1658 } } };
        const uint16_t p1_u[2][2][9] = { { {  512,  512,  176,  589,  253,  771,  435,  848,  512 },
                                           { 2048, 2048,  704, 2356, 1012, 3084, 1740, 3392, 2048 } },
                                         { {  512,  512,  176,  606,  270,  754,  418,  848,  512 },
                                           { 2048, 2048,  704, 2423, 1079, 3017, 1673, 3392, 2048 } } };
        const uint16_t p1_v[2][2][9] = { { {  512,  512,  543,  176,  207,  817,  848,  481,  512 },
                                           { 2048, 2048, 2171,  704,  827, 3269, 3392, 1925, 2048 } },
                                         { {  512,  512,  539,  176,  203,  821,  848,  485,  512 },
                                           { 2048, 2048, 2156,  704,  812, 3284, 3392, 1940, 2048 } } };

        // 100% Cyan, 100% White, 75% White (x6), 100% Blue, -I, +I, 75% White
        const uint16_t p2_y[2][2][12] = { { {  754,  940,  721,  721,  721,  721,  721,  721,  127,  244,  245,  721 },
                                            { 3015, 3760, 2884, 2884, 2884, 2884, 2884, 2884,  509,  976,  982, 2884 } },
                                          { {  710,  940,  721,  721,  721,  721,  721,  721,  116,    0,    0,  721 },
                                            { 2839, 3760, 2884, 2884, 2884, 2884, 2884, 2884,  464,    0,    0, 2884 } } };
        const uint16_t p2_u[2][2][12] = { { {  615,  512,  512,  512,  512,  512,  512,  512,  960,  612,  412,  512 },
                                            { 2459, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 3840, 2448, 1648, 2048 } },
                                          { {  637,  512,  512,  512,  512,  512,  512,  512,  960,    0,    0,  512 },
                                            { 2548, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 3840,    0,    0, 2048 } } };
        const uint16_t p2_v[2][2][12] = { { {   64,  512,  512,  512,  512,  512,  512,  512,  471,  395,  629,  512 },
                                            {  256, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 1884, 1580, 2516, 2048 } },
                                          { {   64,  512,  512,  512,  512,  512,  512,  512,  476,    0,    0,  512 },
                                            {  256, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 1904,    0,    0, 2048 } } };

        // 100% Yellow, 0% Black (x5), Ramp 100%, 100% White, 100% Red, +Q
        const uint16_t p3_y[2][2][10] = { { {  877,   64,   64,   64,   64,   64,  940,  940,  250,  141 },
                                            { 3507,  256,  256,  256,  256,  256, 3760, 3760, 1001,  564 } },
                                          { {  888,   64,   64,   64,   64,   64,  940,  940,  294,    0 },
                                            { 3552,  256,  256,  256,  256,  256, 3760, 3760, 1177,    0 } } };
        const uint16_t p3_u[2][2][10] = { { {   64,  512,  512,  512,  512,  512,  512,  512,  409,  697 },
                                            {  256, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 1637, 2787 } },
                                          { {   64,  512,  512,  512,  512,  512,  512,  512,  387,    0 },
                                            {  256, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 1548,    0 } } };
        const uint16_t p3_v[2][2][10] = { { {  553,  512,  512,  512,  512,  512,  512,  512,  960,  606 },
                                            { 2212, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 3840, 2425 } },
                                          { {  548,  512,  512,  512,  512,  512,  512,  512,  960,    0 },
                                            { 2192, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 3840,    0 } } };

        // 15% Gray, 0% Black, 100% White, 0% Black, -2% Black, 0% Black, 2% Black, 0% Black, 4% Black, 0% Black, 15% Gray, Sub-black Valley, Super-white Peak
        const uint16_t p4_y[2][13] = { {  195,   64,  940,   64,   46,   64,   82,   64,   99,   64,  195,    4, 1019 },
                                       {  782,  256, 3760,  256,  186,  256,  326,  256,  396,  256,  782,   16, 4079 } };
        const uint16_t p4_u[2][13] = { {  512,  512,  512,  512,  512,  512,  512,  512,  512,  512,  512,  512,  512 },
                                       { 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048 } };
        const uint16_t p4_v[2][13] = { {  512,  512,  512,  512,  512,  512,  512,  512,  512,  512,  512,  512,  512 },
                                       { 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048, 2048 } };

        // 525-line Systems: 710.85x484 and 2 half lines ~ 711x486
        // 625-line Systems: 702x574 and 2 half lines ~ 702x576
        // EG-1 1990
        // http://xpt.sourceforge.net/techdocs/media/video/dvd/dvd04-DVDAuthoringSpecwise/ar01s02.html
        // https://forum.doom9.org/showpost.php?p=1686753&postcount=17

        // [compatability][pattern height]
        const int ntsc_heights[3][4] = { { 324, 41, 121, 486 },
                                         { 324, 40, 122, 486 },
                                         { 320, 40, 120, 480 } };

        // [hdr system][bitdepth][value]
        const uint16_t hdr_p1_r[3][2][9] = { { {  414,  940,  940,   64,   64,  940,  940,   64,  414 },
                                               { 1656, 3760, 3760,  256,  256, 3760, 3760,  256, 1656 } },
                                             { {  414,  940,  940,   64,   64,  940,  940,   64,  414 },
                                               { 1656, 3760, 3760,  256,  256, 3760, 3760,  256, 1656 } },
                                             { {  409, 1023, 1023,    0,    0, 1023, 1023,    0,  409 },
                                               { 1638, 4095, 4095,    0,    0, 4095, 4095,    0, 1638 } } };
        const uint16_t hdr_p1_g[3][2][9] = { { {  414,  940,  940,  940,  940,   64,   64,   64,  414 },
                                               { 1656, 3760, 3760, 3760, 3760,  256,  256,  256, 1656 } },
                                             { {  414,  940,  940,  940,  940,   64,   64,   64,  414 },
                                               { 1656, 3760, 3760, 3760, 3760,  256,  256,  256, 1656 } },
                                             { { 409, 1023, 1023,  1023, 1023,    0,    0,    0,  409 },
                                               { 1638, 4095, 4095, 4095, 4095,    0,    0,    0, 1638 } } };
        const uint16_t hdr_p1_b[3][2][9] = { { {  414,  940,   64,  940,   64,  940,   64,  940,  414 },
                                               { 1656, 3760,  256, 3760,  256, 3760,  256, 3760, 1656 } },
                                             { {  414,  940,   64,  940,   64,  940,   64,  940,  414 },
                                               { 1656, 3760,  256, 3760,  256, 3760,  256, 3760, 1656 } },
                                             { {  409, 1023,    0, 1023,    0, 1023,    0, 1023,  409 },
                                               { 1638, 4095,    0, 4095,    0, 4095,    0, 4095, 1638 } } };

        const uint16_t hdr_p2_r[3][2][9] = { { {  414,  721,  721,   64,   64,  721,  721,   64,  414 },
                                               { 1656, 2884, 2884,  256,  256, 2884, 2884,  256, 1656 } },
                                             { {  414,  572,  572,   64,   64,  572,  572,   64,  414 },
                                               { 1656, 2288, 2288,  256,  256, 2288, 2288,  256, 1656 } },
                                             { {  409,  593,  593,    0,    0,  593,  593,    0,  409 },
                                               { 1638, 2375, 2375,    0,    0, 2375, 2375,    0, 1638 } } };
        const uint16_t hdr_p2_g[3][2][9] = { { {  414,  721,  721,  721,  721,   64,   64,   64,  414 },
                                               { 1656, 2884, 2884, 2884, 2884,  256,  256,  256, 1656 } },
                                             { {  414,  572,  572,  572,  572,   64,   64,   64,  414 },
                                               { 1656, 2288, 2288, 2288, 2288,  256,  256,  256, 1656 } },
                                             { {  409,  593,  593,  593,  593,    0,    0,    0,  409 },
                                               { 1638, 2375, 2375, 2375, 2375,    0,    0,    0, 1638 } } };
        const uint16_t hdr_p2_b[3][2][9] = { { {  414,  721,   64,  721,   64,  721,   64,  721,  414 },
                                               { 1656, 2884,  256, 2884,  256, 2884,  256, 2884, 1656 } },
                                             { {  414,  572,   64,  572,   64,  572,   64,  572,  414 },
                                               { 1656, 2288,  256, 2288,  256, 2288,  256, 2288, 1656 } },
                                             { {  409,  593,    0,  593,    0,  593,    0,  593,  409 },
                                               { 1638, 2375,    0, 2375,    0, 2375,    0, 2375, 1638 } } };

        const uint16_t hdr_p3_gray[3][2][15] = { { {  721,    4,   64,  152,  239,  327,  414,  502,  590,  677,  765,  852,  940, 1019,  721 },
                                                   { 2884,   16,  256,  608,  956, 1308, 1656, 2008, 2360, 2708, 3060, 3408, 3760, 4076, 2884 } },
                                                 { {  572,    4,   64,  152,  239,  327,  414,  502,  590,  677,  765,  852,  940, 1019,  572 },
                                                   { 2288,   16,  256,  608,  956, 1308, 1656, 2008, 2360, 2708, 3060, 3408, 3760, 4076, 2288 } },
                                                 { {  593,    0,    0,  102,  205,  307,  409,  512,  614,  716,  818,  921, 1023, 1023,  593 },
                                                   { 2375,    0,    0,  410,  819, 1229, 1638, 2048, 2457, 2867, 3276, 3686, 4095, 4095, 2375 } } };

        const uint16_t hdr_p4_gray[3][2][3] = { { {  64,   4, 1019 },
                                                  { 256,  16, 4079 } },
                                                { {  64,   4, 1019 },
                                                  { 256,  16, 4079 } },
                                                { {   0,   0, 1023 },
                                                  {   0,   0, 4095 } } };

        const uint16_t hdr_p5_r[3][2][15] = { { {  713,  538,  512,   64,   48,   64,   80,   64,   99,   64,  721,   64,  651,  639,  227 },
                                                { 2852, 2152, 2048,  256,  192,  256,  320,  256,  396,  256, 2884,  256, 2604, 2556,  908 } },
                                              { {  568,  484,  474,   64,   48,   64,   80,   64,   99,   64,  572,   64,  536,  530,  317 },
                                                { 2272, 1936, 1896,  256,  192,  256,  320,  256,  396,  256, 2288,  256, 2144, 2120, 1268 } },
                                              { {  589,  491,  478,    0,    0,    0,   20,    0,   41,    0,  593,    0,  551,  544,  296 },
                                                { 2356, 1964, 1915,    0,    0,    0,   82,    0,  164,    0, 2375,    0, 2206, 2178, 1184 } } };
        const uint16_t hdr_p5_g[3][2][15] = { { {  719,  709,  706,   64,   48,   64,   80,   64,   99,   64,  721,   64,  286,  269,  147 },
                                                { 2876, 2836, 2824,  256,  192,  256,  320,  256,  396,  256, 2884,  256, 1144, 1076,  588 } },
                                              { {  571,  566,  564,   64,   48,   64,   80,   64,   99,   64,  572,   64,  361,  350,  236 },
                                                { 2284, 2264, 2256,  256,  192,  256,  320,  256,  396,  256, 2288,  256, 1444, 1400,  944 } },
                                              { {  592,  586,  584,    0,    0,    0,   20,    0,   41,    0,  593,    0,  347,  334,  201 },
                                                { 2370, 2345, 2339,    0,    0,    0,   82,    0,  164,    0, 2375,    0, 1389, 1337,  805 } } };
        const uint16_t hdr_p5_b[3][2][15] = { { {  316,  718,  296,   64,   48,   64,   80,   64,   99,   64,  721,   64,  705,  164,  702 },
                                                { 1264, 2872, 1184,  256,  192,  256,  320,  256,  396,  256, 2884,  256, 2820,  656, 2808 } },
                                              { {  381,  571,  368,   64,   48,   64,   80,   64,   99,   64,  572,   64,  564,  256,  562 },
                                                { 1524, 2284, 1472,  256,  192,  256,  320,  256,  396,  256, 2288,  256, 2256, 1024, 2248 } },
                                              { {  370,  592,  355,    0,    0,    0,   20,    0,   41,    0,  593,    0,  584,  225,  582 },
                                                { 1480, 2368, 1420,    0,    0,    0,   82,    0,  164,    0, 2375,    0, 2336,  900, 2328 } } };

        // [resolution][compatability][bar width]
        const int p1_widths[10][3][10] = { { {   4, 101, 102, 102, 102, 102, 102, 101,   4 }, // 525-line (NTSC BT.601)
                                             {   4, 102, 102, 102, 100, 102, 102, 102,   4 },
                                             {   0, 104, 102, 102, 102, 104, 102, 104,   0 } },
                                           { {   9,  87,  88,  88,  88,  88,  88,  88,  87,   9 }, // 625-line (PAL BT.601)
                                             {   8,  88,  88,  88,  88,  88,  88,  88,  88,   8 },
                                             {   0,  90,  90,  90,  90,  90,  90,  90,  90,   0 } },
                                           { { 160, 137, 137, 137, 138, 137, 137, 137, 160 }, // 720
                                             { 160, 138, 136, 138, 136, 138, 136, 138, 160 },
                                             { 156, 142, 136, 138, 136, 138, 136, 142, 156 } },
                                           { { 240, 205, 206, 206, 206, 206, 206, 205, 240 }, // 1080
                                             { 240, 206, 206, 206, 204, 206, 206, 206, 240 },
                                             { 236, 210, 206, 206, 204, 206, 206, 210, 236 } },
                                           { { 304, 205, 206, 206, 206, 206, 206, 205, 304 }, // 2K
                                             { 304, 206, 206, 206, 204, 206, 206, 206, 304 },
                                             { 300, 210, 206, 206, 204, 206, 206, 210, 300 } },
                                           { { 480, 410, 412, 412, 412, 412, 412, 410, 480 }, // UHD
                                             { 480, 412, 412, 412, 410, 412, 412, 412, 480 },
                                             { 472, 420, 412, 412, 408, 412, 412, 420, 472 } },
                                           { { 608, 410, 412, 412, 412, 412, 412, 410, 608 }, // 4K
                                             { 608, 412, 412, 412, 410, 412, 412, 412, 608 },
                                             { 600, 420, 412, 412, 408, 412, 412, 420, 600 } },
                                           { { 960, 820, 824, 824, 824, 824, 824, 820, 960 }, // 8K
                                             { 960, 824, 824, 824, 816, 824, 824, 824, 960 },
                                             { 944, 840, 824, 824, 816, 824, 824, 840, 944 } },
                                           { {   5, 109, 108, 108, 108, 108, 108, 109,   5 }, // 525-line (NTSC 4fsc)
                                             {   4, 110, 108, 108, 108, 108, 108, 110,   4 },
                                             {   0, 108, 110, 110, 112, 110, 110, 108,   0 } },
                                           { {  12, 117, 115, 115, 115, 115, 115, 115, 117, 12 }, // 625-line (PAL 4fsc)
                                             {  12, 116, 116, 116, 114, 114, 116, 116, 116, 12 },
                                             {   0, 120, 118, 118, 118, 118, 118, 118, 120,  0 } } };

        // [resolution][compatability][bar width]
        const int p4_widths[10][3][11] = { { {   4, 127, 128, 127, 128,  34,  34,  34, 101,   4,   0 }, // 525-line (NTSC BT.601)
                                             {   4, 126, 128, 126, 128,  34,  34,  34, 102,   4,   0 },
                                             {   0, 128, 130, 128, 128,  34,  34,  34, 104,   0,   0 } },
                                           { {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 }, // 625-line (PAL BT.601)
                                             {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
                                             {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 } },
                                           { { 160, 206, 274, 115,  46,  45,  46,  46,  45, 137, 160 }, // 720
                                             { 160, 206, 274, 116,  46,  44,  46,  46,  44, 138, 160 },
                                             { 156, 210, 274, 116,  46,  44,  46,  46,  44, 142, 156 } },
                                           { { 240, 309, 411, 171,  69,  68,  69,  68,  69, 206, 240 }, // 1080
                                             { 240, 308, 412, 170,  68,  70,  68,  70,  68, 206, 240 },
                                             { 236, 312, 412, 170,  68,  70,  68,  70,  68, 210, 236 } },
                                           { { 304, 309, 411, 171,  69,  68,  69,  68,  69, 206, 304 }, // 2K
                                             { 304, 308, 412, 170,  68,  70,  68,  70,  68, 206, 304 },
                                             { 300, 312, 412, 170,  68,  70,  68,  70,  68, 210, 300 } },
                                           { { 480, 618, 822, 342, 138, 136, 138, 136, 138, 412, 480 }, // UHD
                                             { 480, 616, 824, 340, 136, 140, 136, 140, 136, 412, 480 },
                                             { 472, 624, 824, 340, 136, 140, 136, 140, 136, 420, 472 } },
                                           { { 608, 618, 822, 342, 138, 136, 138, 136, 138, 412, 608 }, // 4K
                                             { 608, 616, 824, 340, 136, 140, 136, 140, 136, 412, 608 },
                                             { 600, 624, 824, 340, 136, 140, 136, 140, 136, 420, 600 } },
                                           { { 960, 1236,1644,684, 276, 272, 276, 272, 276, 824, 960 }, // 8K
                                             { 960, 1232,1648,680, 272, 280, 272, 280, 272, 824, 960 },
                                             { 944, 1248,1648,680, 272, 280, 272, 280, 272, 840, 944 } },
                                           { {   5, 136, 135, 135, 135,  36,  36,  36, 109,   5,   0 }, // 525-line (NTSC 4fsc)
                                             {   4, 136, 136, 136, 136,  36,  36,  36, 110,   4,   0 },
                                             {   0, 138, 138, 138, 136,  36,  38,  36, 108,   0,   0 } },
                                           { {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 }, // 625-line (PAL 4fsc)
                                             {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
                                             {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 } } };

        // [resolution][bar width]
        const int hdr_p1_widths[5][9] = { { 240, 206, 206, 206, 204, 206, 206, 206, 240 },   // 1080
                                          { 304, 206, 206, 206, 204, 206, 206, 206, 304 },   // 2K
                                          { 480, 412, 412, 412, 408, 412, 412, 412, 480 },   // UHD
                                          { 608, 412, 412, 412, 408, 412, 412, 412, 608 },   // 4K
                                          { 960, 824, 824, 824, 816, 824, 824, 824, 960 } }; // 8K

        const int hdr_p3_widths[5][15] = { { 240, 206, 103, 103, 103, 103, 102, 102, 103, 103, 103, 103, 103, 103, 240 },   // 1080
                                           { 304, 206, 103, 103, 103, 103, 102, 102, 103, 103, 103, 103, 103, 103, 304 },   // 2K
                                           { 480, 412, 206, 206, 206, 206, 204, 204, 206, 206, 206, 206, 206, 206, 480 },   // UHD
                                           { 608, 412, 206, 206, 206, 206, 204, 204, 206, 206, 206, 206, 206, 206, 608 },   // 4K
                                           { 960, 824, 412, 412, 412, 412, 408, 408, 412, 412, 412, 412, 412, 412, 960 } }; // 8K

        // [depth][hdr system][resolution][bar width]
        const int hdr_p4_widths[2][3][5][4] = { { { {  240,  559, 1014,  107 }, // HLG 10-bit
                                                    {  304,  559, 1014,  171 },
                                                    {  480, 1118, 2028,  214 },
                                                    {  608, 1118, 2028,  342 },
                                                    {  960, 2236, 4056,  428 } } ,
                                                  { {  240,  559, 1014,  107 }, // PQ 10-bit
                                                    {  304,  559, 1014,  171 },
                                                    {  480, 1118, 2028,  214 },
                                                    {  608, 1118, 2028,  342 },
                                                    {  960, 2236, 4056,  428 } } ,
                                                  { {  240,  551, 1022,  107 }, // PQ full range 10-bit
                                                    {  304,  551, 1022,  171 },
                                                    {  480, 1102, 2044,  214 },
                                                    {  608, 1102, 2044,  342 },
                                                    {  960, 2204, 4088,  428 } } } ,
                                                { { {  240,  559, 1015,  106 }, // HLG 12-bit
                                                    {  304,  559, 1015,  170 },
                                                    {  480, 1117, 2031,  212 },
                                                    {  608, 1117, 2031,  340 },
                                                    {  960, 2233, 4062,  425 } } ,
                                                  { {  240,  559, 1015,  106 }, // PQ 12-bit
                                                    {  304,  559, 1015,  170 },
                                                    {  480, 1117, 2031,  212 },
                                                    {  608, 1117, 2031,  340 },
                                                    {  960, 2233, 4062,  425 } } ,
                                                  { {  240,  551, 1023,  106 }, // PQ full range 12-bit
                                                    {  304,  551, 1023,  170 },
                                                    {  480, 1101, 2047,  212 },
                                                    {  608, 1101, 2047,  340 },
                                                    {  960, 2201, 4094,  425 } } } };

        const int hdr_p5_widths[5][15] = { {  80,   80,   80,  136,   70,   68,   70,   68,   70,  238,  438,  282,   80,   80,   80 },   // 1080
                                           { 144,   80,   80,  136,   70,   68,   70,   68,   70,  238,  438,  282,   80,   80,  144 },   // 2K
                                           { 160,  160,  160,  272,  140,  136,  140,  136,  140,  476,  876,  564,  160,  160,  160 },   // UHD
                                           { 288,  160,  160,  272,  140,  136,  140,  136,  140,  476,  876,  564,  160,  160,  288 },   // 4K
                                           { 320,  320,  320,  544,  280,  272,  280,  272,  280,  952, 1752, 1128,  320,  320,  320 } }; // 8K

        const int compat = d->compatability;
        const int resolution = d->resolution;
        const int hdr = d->hdr;
        const int wcg = d->wcg;
        const int depth = d->vi.format.bitsPerSample == 10 ? 0 : 1;
        const int iq = d->iq;
        const int height = d->vi.height;
        const int width = d->vi.width;

        VSFrame *frame = 0;
        frame = vsapi->newVideoFrame(&d->vi.format, width, height, 0, core);
        VSMap *props = vsapi->getFramePropertiesRW(frame);

        if (hdr)
        {
            vsapi->mapSetInt(props, "_Matrix", VSC_MATRIX_RGB, maReplace);
            vsapi->mapSetInt(props, "_Transfer", hdr == 1 ? VSC_TRANSFER_ARIB_B67 : VSC_TRANSFER_ST2084, maReplace);
            vsapi->mapSetInt(props, "_Primaries", VSC_PRIMARIES_BT2020, maReplace);
            vsapi->mapSetInt(props, "_SARNum", 1, maReplace);
            vsapi->mapSetInt(props, "_SARDen", 1, maReplace);
        }
        else
        {
            if (resolution < HD720 || resolution > UHDTV2)
            {
                if (resolution == PAL || resolution == PAL_4FSC)
                {
                    vsapi->mapSetInt(props, "_Matrix", VSC_MATRIX_BT470_BG, maReplace);
                    vsapi->mapSetInt(props, "_Primaries", VSC_PRIMARIES_BT470_BG, maReplace);
                    vsapi->mapSetInt(props, "_SARNum", resolution == PAL ? 128 : 547, maReplace);
                    vsapi->mapSetInt(props, "_SARDen", resolution == PAL ? 117 : 657, maReplace);
                }
                else
                {
                    vsapi->mapSetInt(props, "_Matrix", VSC_MATRIX_ST170_M, maReplace);
                    vsapi->mapSetInt(props, "_Primaries", VSC_PRIMARIES_ST170_M, maReplace);
                    vsapi->mapSetInt(props, "_SARNum", resolution == NTSC ? 4320 : 352, maReplace);
                    vsapi->mapSetInt(props, "_SARDen", resolution == NTSC ? 4739 : 413, maReplace);
                }
                vsapi->mapSetInt(props, "_Transfer", VSC_TRANSFER_BT601, maReplace);
            }
            else
            {
                vsapi->mapSetInt(props, "_Matrix",    wcg ? VSC_MATRIX_BT2020_NCL : VSC_MATRIX_BT709, maReplace);
                vsapi->mapSetInt(props, "_Transfer", !wcg ? VSC_TRANSFER_BT709 :
                                                    depth ? VSC_TRANSFER_BT2020_12 : VSC_TRANSFER_BT2020_10, maReplace);
                vsapi->mapSetInt(props, "_Primaries", wcg ? VSC_PRIMARIES_BT2020 : VSC_PRIMARIES_BT709, maReplace);
                vsapi->mapSetInt(props, "_SARNum", 1, maReplace);
                vsapi->mapSetInt(props, "_SARDen", 1, maReplace);
            }
        }
        vsapi->mapSetInt(props, "_ColorRange", hdr == 3 ? 0 : 1, maReplace); // limited, unless full range PQ

        uint16_t *y = (uint16_t *)vsapi->getWritePtr(frame, 0);
        uint16_t *u = (uint16_t *)vsapi->getWritePtr(frame, 1);
        uint16_t *v = (uint16_t *)vsapi->getWritePtr(frame, 2);
        intptr_t stride = vsapi->getStride(frame, 0) / sizeof(uint16_t);
        if (resolution == NTSC || resolution == NTSC_4FSC)
        {
            // pattern 1
            for (int h = 0; h < ntsc_heights[compat][0]; h++)
            {
                uint16_t *edge_y = y;
                uint16_t *edge_u = u;
                uint16_t *edge_v = v;
                for (int bar = 0; bar < 9; bar++)
                    for (int i = 0; i < p1_widths[resolution][compat][bar]; i++, edge_y++, edge_u++, edge_v++)
                    {
                        *edge_y = ntsc1_y[depth][bar];
                        *edge_u = ntsc1_u[depth][bar];
                        *edge_v = ntsc1_v[depth][bar];
                    }
                y += stride;
                u += stride;
                v += stride;
            }
            // pattern 2
            for (int h = 0; h < ntsc_heights[compat][1]; h++)
            {
                uint16_t *edge_y = y;
                uint16_t *edge_u = u;
                uint16_t *edge_v = v;
                for (int bar = 0; bar < 9; bar++)
                    for (int i = 0; i < p1_widths[resolution][compat][bar]; i++, edge_y++, edge_u++, edge_v++)
                    {
                        *edge_y = ntsc2_y[depth][bar];
                        *edge_u = ntsc2_u[depth][bar];
                        *edge_v = ntsc2_v[depth][bar];
                    }
                y += stride;
                u += stride;
                v += stride;
            }
            // pattern 3
            for (int h = 0; h < ntsc_heights[compat][2]; h++)
            {
                uint16_t *edge_y = y;
                uint16_t *edge_u = u;
                uint16_t *edge_v = v;
                for (int bar = 0; bar < 10; bar++)
                    for (int i = 0; i < p4_widths[resolution][compat][bar]; i++, edge_y++, edge_u++, edge_v++)
                    {
                        *edge_y = ntsc3_y[depth][bar];
                        *edge_u = ntsc3_u[depth][bar];
                        *edge_v = ntsc3_v[depth][bar];
                    }
                y += stride;
                u += stride;
                v += stride;
            }
            if (d->halfline)
            {
                int blank_y = 64 * (depth * 4);
                int blank_c = 512 * (depth * 4);

                uint16_t* edge_y = (uint16_t*)vsapi->getWritePtr(frame, 0);
                uint16_t* edge_u = (uint16_t*)vsapi->getWritePtr(frame, 1);
                uint16_t* edge_v = (uint16_t*)vsapi->getWritePtr(frame, 2);
                // video starts 41.259 us after 0H
                int blankposition = resolution == NTSC_4FSC ? 461 : 413;
                for (int i = 0; i < blankposition; i++)
                {
                    edge_y[i] = blank_y;
                    edge_u[i] = blank_c;
                    edge_v[i] = blank_c;
                }

                edge_y = (uint16_t*)vsapi->getWritePtr(frame, 0);
                edge_u = (uint16_t*)vsapi->getWritePtr(frame, 1);
                edge_v = (uint16_t*)vsapi->getWritePtr(frame, 2);
                // video ends 30.592 us after 0H
                blankposition = resolution == NTSC_4FSC ? 309 : 291;
                edge_y += stride * (height - 1);
                edge_u += stride * (height - 1);
                edge_v += stride * (height - 1);
                for (int i = blankposition; i < width; i++)
                {
                    edge_y[i] = blank_y;
                    edge_u[i] = blank_c;
                    edge_v[i] = blank_c;
                }
            }
        }
        else if (resolution == PAL || resolution == PAL_4FSC)
        {
            for (int h = 0; h < height; h++)
            {
                uint16_t *edge_y = y;
                uint16_t *edge_u = u;
                uint16_t *edge_v = v;
                for (int bar = 0; bar < 10; bar++)
                    for (int i = 0; i < p1_widths[resolution][compat][bar]; i++, edge_y++, edge_u++, edge_v++)
                    {
                        *edge_y = pal_y[depth][bar];
                        *edge_u = pal_u[depth][bar];
                        *edge_v = pal_v[depth][bar];
                    }
                y += stride;
                u += stride;
                v += stride;
            }
            if (d->halfline)
            {
                int blank_y = 64 * (depth * 4);
                int blank_c = 512 * (depth * 4);

                uint16_t* edge_y = (uint16_t*)vsapi->getWritePtr(frame, 0);
                uint16_t* edge_u = (uint16_t*)vsapi->getWritePtr(frame, 1);
                uint16_t* edge_v = (uint16_t*)vsapi->getWritePtr(frame, 2);
                // video starts 42.5 us after 0H
                int blankposition = resolution == PAL_4FSC ? 580 : 410;
                for (int i = 0; i < blankposition; i++)
                {
                    edge_y[i] = blank_y;
                    edge_u[i] = blank_c;
                    edge_v[i] = blank_c;
                }

                edge_y = (uint16_t*)vsapi->getWritePtr(frame, 0);
                edge_u = (uint16_t*)vsapi->getWritePtr(frame, 1);
                edge_v = (uint16_t*)vsapi->getWritePtr(frame, 2);
                // video ends 30.35 us after 0H
                blankposition = resolution == PAL_4FSC ? 365 : 278;
                edge_y += stride * (height - 1);
                edge_u += stride * (height - 1);
                edge_v += stride * (height - 1);
                for (int i = blankposition; i < width; i++)
                {
                    edge_y[i] = blank_y;
                    edge_u[i] = blank_c;
                    edge_v[i] = blank_c;
                }
            }
        }
        else if ( hdr ) // HDR systems
        {
            // pattern 1 - 100% top strip
            for (int h = 0; h < height / 12; h++)
            {
                uint16_t *edge_y = y;
                uint16_t *edge_u = u;
                uint16_t *edge_v = v;
                for (int bar = 0; bar < 9; bar++)
                    for (int i = 0; i < hdr_p1_widths[resolution-3][bar]; i++, edge_y++, edge_u++, edge_v++)
                    {
                        *edge_y = hdr_p1_r[hdr-1][depth][bar];
                        *edge_u = hdr_p1_g[hdr-1][depth][bar];
                        *edge_v = hdr_p1_b[hdr-1][depth][bar];
                    }
                y += stride;
                u += stride;
                v += stride;
            }
            // pattern 2 - 75%/58% bars
            for (int h = 0; h < height / 2; h++)
            {
                uint16_t *edge_y = y;
                uint16_t *edge_u = u;
                uint16_t *edge_v = v;
                for (int bar = 0; bar < 9; bar++)
                    for (int i = 0; i < hdr_p1_widths[resolution - 3][bar]; i++, edge_y++, edge_u++, edge_v++)
                    {
                        *edge_y = hdr_p2_r[hdr - 1][depth][bar];
                        *edge_u = hdr_p2_g[hdr - 1][depth][bar];
                        *edge_v = hdr_p2_b[hdr - 1][depth][bar];
                    }
                y += stride;
                u += stride;
                v += stride;
            }
            // pattern 3 - grayscale
            for (int h = 0; h < height / 12; h++)
            {
                uint16_t *edge_y = y;
                uint16_t *edge_u = u;
                uint16_t *edge_v = v;
                for (int bar = 0; bar < 15; bar++)
                    for (int i = 0; i < hdr_p3_widths[resolution - 3][bar]; i++, edge_y++, edge_u++, edge_v++)
                        *edge_y = *edge_u = *edge_v = hdr_p3_gray[hdr - 1][depth][bar];
                y += stride;
                u += stride;
                v += stride;
            }
            // pattern 4 - ramp
            for (int h = 0; h < height / 12; h++)
            {
                uint16_t *edge_y = y;
                uint16_t *edge_u = u;
                uint16_t *edge_v = v;
                for (int bar = 0; bar < 2; bar++)
                    for (int i = 0; i < hdr_p4_widths[depth][hdr - 1][resolution - 3][bar]; i++, edge_y++, edge_u++, edge_v++)
                        *edge_y = *edge_u = *edge_v = hdr_p4_gray[hdr - 1][depth][bar];
                uint16_t rampwidth = hdr_p4_widths[depth][hdr - 1][resolution - 3][2];
                uint16_t rampheight = hdr_p4_gray[hdr - 1][depth][2] - hdr_p4_gray[hdr - 1][depth][1];
                float slope = (float)rampheight / (float)rampwidth;
                for (int i = 0; i < rampwidth; i++, edge_y++, edge_u++, edge_v++)
                    *edge_y = *edge_u = *edge_v = (int)(hdr_p4_gray[hdr - 1][depth][1] + i * slope);
                for (int i = 0; i < hdr_p4_widths[depth][hdr - 1][resolution - 3][3]; i++, edge_y++, edge_u++, edge_v++)
                    *edge_y = *edge_u = *edge_v = hdr_p4_gray[hdr - 1][depth][2];
                y += stride;
                u += stride;
                v += stride;
            }
            // pattern 5 - 75%/58% 709 bars
            for (int h = 0; h < height / 4; h++)
            {
                uint16_t *edge_y = y;
                uint16_t *edge_u = u;
                uint16_t *edge_v = v;
                for (int bar = 0; bar < 15; bar++)
                    for (int i = 0; i < hdr_p5_widths[resolution - 3][bar]; i++, edge_y++, edge_u++, edge_v++)
                    {
                        *edge_y = hdr_p5_r[hdr - 1][depth][bar];
                        *edge_u = hdr_p5_g[hdr - 1][depth][bar];
                        *edge_v = hdr_p5_b[hdr - 1][depth][bar];
                    }
                y += stride;
                u += stride;
                v += stride;
            }
        }
        else // HD and higher SDR systems
        {
            // pattern 1
            for (int h = 0; h < height / 12 * 7; h++)
            {
                uint16_t *edge_y = y;
                uint16_t *edge_u = u;
                uint16_t *edge_v = v;
                for (int bar = 0; bar < 9; bar++)
                    for (int i = 0; i < p1_widths[resolution][compat][bar]; i++, edge_y++, edge_u++, edge_v++)
                    {
                        *edge_y = p1_y[wcg][depth][bar];
                        *edge_u = p1_u[wcg][depth][bar];
                        *edge_v = p1_v[wcg][depth][bar];
                    }
                y += stride;
                u += stride;
                v += stride;
            }
            // pattern 2
            for (int h = 0; h < height / 12; h++)
            {
                uint16_t *edge_y = y;
                uint16_t *edge_u = u;
                uint16_t *edge_v = v;
                for (int i = 0; i < p1_widths[resolution][compat][0]; i++, edge_y++, edge_u++, edge_v++)
                {
                    *edge_y = p2_y[wcg][depth][0];
                    *edge_u = p2_u[wcg][depth][0];
                    *edge_v = p2_v[wcg][depth][0];
                }
                // sub-pattern *2: 100% white, -I, +I, or 75% white
                int iqbar = iq ? iq + 8 : 1;
                for (int i = 0; i < p1_widths[resolution][compat][1]; i++, edge_y++, edge_u++, edge_v++)
                {
                    *edge_y = p2_y[wcg][depth][iqbar];
                    *edge_u = p2_u[wcg][depth][iqbar];
                    *edge_v = p2_v[wcg][depth][iqbar];
                }
                for (int bar = 2; bar < 9; bar++)
                    for (int i = 0; i < p1_widths[resolution][compat][bar]; i++, edge_y++, edge_u++, edge_v++)
                    {
                        *edge_y = p2_y[wcg][depth][bar];
                        *edge_u = p2_u[wcg][depth][bar];
                        *edge_v = p2_v[wcg][depth][bar];
                    }
                y += stride;
                u += stride;
                v += stride;
            }
            // pattern 3
            for (int h = 0; h < height / 12; h++)
            {
                uint16_t *edge_y = y;
                uint16_t *edge_u = u;
                uint16_t *edge_v = v;
                for (int i = 0; i < p1_widths[resolution][compat][0]; i++, edge_y++, edge_u++, edge_v++)
                {
                    *edge_y = p3_y[wcg][depth][0];
                    *edge_u = p3_u[wcg][depth][0];
                    *edge_v = p3_v[wcg][depth][0];
                }
                // sub-pattern *3: 0% black or +Q
                int iqbar = iq == IQ_BOTH ? iq + 8 : 1;
                for (int i = 0; i < p1_widths[resolution][compat][1]; i++, edge_y++, edge_u++, edge_v++)
                {
                    *edge_y = p3_y[wcg][depth][iqbar];
                    *edge_u = p3_u[wcg][depth][iqbar];
                    *edge_v = p3_v[wcg][depth][iqbar];
                }
                // Y ramp
                uint16_t rampwidth = p1_widths[resolution][compat][2] + p1_widths[resolution][compat][3] +
                    p1_widths[resolution][compat][4] + p1_widths[resolution][compat][5] +
                    p1_widths[resolution][compat][6];
                uint16_t rampheight = p3_y[wcg][depth][6] - p3_y[wcg][depth][2];
                float slope = (float)rampheight / (float)rampwidth;
                for (int i = 0; i < rampwidth; i++, edge_y++, edge_u++, edge_v++)
                {
                    *edge_y = (int)(p3_y[wcg][depth][2] + i * slope);
                    *edge_u = p3_u[wcg][depth][2];
                    *edge_v = p3_v[wcg][depth][2];
                }
                for (int bar = 7; bar < 9; bar++)
                    for (int i = 0; i < p1_widths[resolution][compat][bar]; i++, edge_y++, edge_u++, edge_v++)
                    {
                        *edge_y = p3_y[wcg][depth][bar];
                        *edge_u = p3_u[wcg][depth][bar];
                        *edge_v = p3_v[wcg][depth][bar];
                    }
                y += stride;
                u += stride;
                v += stride;
            }
            // pattern 4a
            for (int h = 0; h < height / 12; h++)
            {
                uint16_t *edge_y = y;
                uint16_t *edge_u = u;
                uint16_t *edge_v = v;
                for (int bar = 0; bar < 11; bar++)
                    for (int i = 0; i < p4_widths[resolution][compat][bar]; i++, edge_y++, edge_u++, edge_v++)
                    {
                        *edge_y = p4_y[depth][bar];
                        *edge_u = p4_u[depth][bar];
                        *edge_v = p4_v[depth][bar];
                    }
                y += stride;
                u += stride;
                v += stride;
            }
            // pattern 4b
            for (int h = 0; h < height / 12; h++)
            {
                uint16_t *edge_y = y;
                uint16_t *edge_u = u;
                uint16_t *edge_v = v;
                for (int i = 0; i < p4_widths[resolution][compat][0]; i++, edge_y++, edge_u++, edge_v++)
                {
                    *edge_y = p4_y[depth][0];
                    *edge_u = p4_u[depth][0];
                    *edge_v = p4_v[depth][0];
                }
                // sub black
                const int subblack = d->subblack;
                uint16_t rampwidth = p4_widths[resolution][compat][1] / 2;
                uint16_t rampheight = p4_y[depth][1] - p4_y[depth][11];
                float slope = (float)subblack * (float)rampheight / (float)rampwidth;
                for (int i = 0; i < rampwidth; i++, edge_y++, edge_u++, edge_v++)
                {
                    *edge_y = (int)(p4_y[depth][1] - i * slope);
                    *edge_u = p4_u[depth][1];
                    *edge_v = p4_v[depth][1];
                }
                for (int i = 0; i < rampwidth; i++, edge_y++, edge_u++, edge_v++)
                {
                    *edge_y = (int)(p4_y[depth][1 + subblack * 10] + i * slope);
                    *edge_u = p4_u[depth][1];
                    *edge_v = p4_v[depth][1];
                }
                // super-white
                const int superwhite = d->superwhite;
                rampwidth = p4_widths[resolution][compat][2] / 2;
                rampheight = p4_y[depth][12] - p4_y[depth][2];
                slope = (float)superwhite * (float)rampheight / (float)rampwidth;
                for (int i = 0; i < rampwidth; i++, edge_y++, edge_u++, edge_v++)
                {
                    *edge_y = (int)(p4_y[depth][2] + i * slope);
                    *edge_u = p4_u[depth][2];
                    *edge_v = p4_v[depth][2];
                }
                for (int i = 0; i < rampwidth; i++, edge_y++, edge_u++, edge_v++)
                {
                    *edge_y = (int)(p4_y[depth][2 + superwhite * 10] - i * slope);
                    *edge_u = p4_u[depth][2];
                    *edge_v = p4_v[depth][2];
                }
                for (int bar = 3; bar < 11; bar++)
                    for (int i = 0; i < p4_widths[resolution][compat][bar]; i++, edge_y++, edge_u++, edge_v++)
                    {
                        *edge_y = p4_y[depth][bar];
                        *edge_u = p4_u[depth][bar];
                        *edge_v = p4_v[depth][bar];
                    }
                y += stride;
                u += stride;
                v += stride;
            }
            // pattern 4c
            for (int h = 0; h < height / 12; h++)
            {
                uint16_t *edge_y = y;
                uint16_t *edge_u = u;
                uint16_t *edge_v = v;
                for (int bar = 0; bar < 11; bar++)
                    for (int i = 0; i < p4_widths[resolution][compat][bar]; i++, edge_y++, edge_u++, edge_v++)
                    {
                        *edge_y = p4_y[depth][bar];
                        *edge_u = p4_u[depth][bar];
                        *edge_v = p4_v[depth][bar];
                    }
                y += stride;
                u += stride;
                v += stride;
            }
        }
        return frame;
    }
    return 0;
}

static void VS_CC colorbarsFree( void *instanceData, VSCore *core, const VSAPI *vsapi )
{
    ColorBarsData *d = (ColorBarsData *)instanceData;
    free( d );
}

static void VS_CC colorbarsCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    ColorBarsData d = { 0 };
    ColorBarsData *data;

    int err = 0;
    d.compatability = (int)vsapi->mapGetInt(in, "compatability", 0, &err);
    if (err)
        d.compatability = 2;
    if (d.compatability < 0 || d.compatability > 2)
        RETERROR( "ColorBars: invalid compatability mode" );

    d.resolution = vsapi->mapGetInt(in, "resolution", 0, &err);
    if (err)
        d.resolution = HD1080;
    if (d.resolution < NTSC || d.resolution > PAL_4FSC)
        RETERROR("ColorBars: invalid resolution");

    const int resolutions[10][4] = { {  720,  486, 30000, 1001 },
                                     {  720,  576,    25,    1 },
                                     { 1280,  720, 60000, 1001 },
                                     { 1920, 1080, 30000, 1001 },
                                     { 2048, 1080, 24000, 1001 },
                                     { 3840, 2160, 60000, 1001 },
                                     { 4096, 2160, 24000, 1001 },
                                     { 7680, 4320, 60000, 1001 },
                                     {  768,  486, 30000, 1001 },
                                     {  948,  576,    25,    1 } };
    d.vi.width = resolutions[d.resolution][0];
    d.vi.height = resolutions[d.resolution][1];
    if (d.compatability == 2 && (d.resolution == NTSC || d.resolution == NTSC_4FSC))
        d.vi.height = 480;
    d.hdr = (int)vsapi->mapGetInt(in, "hdr", 0, &err);
    if (err)
        d.hdr = 0;
    if (d.hdr < 0 || d.hdr > 3)
        RETERROR("ColorBars: invalid HDR mode");

    int pixformat = (int)vsapi->mapGetInt(in, "format", 0, &err);
    if (err)
        RETERROR("ColorBars: invalid format");

    vsapi->getVideoFormatByID(&d.vi.format, pixformat, core);
    if (!d.hdr && pixformat != pfYUV444P12 && pixformat != pfYUV444P10)
        RETERROR("ColorBars: invalid format, only YUV444P10 and YUV444P12 for SDR formats");
    if (d.hdr && pixformat != pfRGB30 && pixformat != pfRGB36)
        RETERROR( "ColorBars: invalid format, only RGB30 and RGB36 for HDR formats");

    d.subblack = (int)vsapi->mapGetInt(in, "subblack", 0, &err);
    if (err)
        d.subblack = 1;
    d.subblack = !!d.subblack;
    d.superwhite = (int)vsapi->mapGetInt(in, "superwhite", 0, &err);
    if (err)
        d.superwhite = 1;
    d.superwhite = !!d.superwhite;
    d.iq = vsapi->mapGetInt(in, "iq", 0, &err);
    if (err)
        d.iq = d.hdr ? IQ_NONE : d.resolution < UHDTV1 ? IQ_BOTH : IQ_NONE;
    if (d.iq < 0 || d.iq > 3)
        RETERROR("ColorBars: invalid I/Q mode");
    d.wcg = (int)vsapi->mapGetInt(in, "wcg", 0, &err);
    if (err)
        d.wcg = 0;
    d.wcg = !!d.wcg;
    if (d.wcg && !d.hdr)
    {
        if (d.resolution < UHDTV1 || d.resolution > UHDTV2)
            RETERROR("ColorBars: wide color (Rec.2020) only valid with UHDTV systems");
        if (d.iq == IQ_BOTH || d.iq == IQ_PLUS_I)
            RETERROR("ColorBars: -I/+Q and +I not valid with wide color (Rec.2020)");
    }
    if (d.resolution == UHDTV2)
    {
        if (!d.wcg && !d.hdr)
            vsapi->logMessage(mtWarning, "ColorBars: wide color (Rec.2020) required with 8K/UHDTV2", core);
        if (d.iq == IQ_BOTH || d.iq == IQ_PLUS_I)
            vsapi->logMessage(mtWarning, "ColorBars: -I/+Q and +I not valid with 8K/UHDTV2 systems", core);
    }
    if (d.hdr)
    {
        if (d.resolution < HD1080)
            RETERROR("ColorBars: HDR mode only valid with 1080 or higher resolutions");
        if (d.wcg)
            vsapi->logMessage(mtWarning, "ColorBars: HDR mode always uses wide color (Rec.2020). Setting wcg=1 has no effect.", core);
        if (d.iq)
            vsapi->logMessage(mtWarning, "ColorBars: I/Q is not valid option with HDR", core);
    }

    d.halfline = (int)vsapi->mapGetInt(in, "halfline", 0, &err);
    if (err)
        d.halfline = 0;
    d.halfline = !!d.halfline;
    if (d.halfline && (d.resolution > PAL && d.resolution < NTSC_4FSC))
        RETERROR("ColorBars: Half line blanking only valid with NTSC/PAL");

    d.filter = (int)vsapi->mapGetInt(in, "filter", 0, &err);
    if (err)
        d.filter = 1;
    d.filter = !!d.filter;

    d.vi.fpsNum = resolutions[d.resolution][2];
    d.vi.fpsDen = resolutions[d.resolution][3];
    d.vi.numFrames = 1;

    data = (ColorBarsData*)malloc(sizeof(d));
    *data = d;

    vsapi->createVideoFilter(out, "ColorBars", &d.vi, colorbarsGetFrame, colorbarsFree, fmParallel, NULL, 0, data, core);
}

VS_EXTERNAL_API(void) VapourSynthPluginInit2( VSPlugin* plugin, const VSPLUGINAPI* vspapi)
{
    vspapi->configPlugin( "com.ifb.colorbars", "colorbars", "SMPTE RP 219-2:2016 and ITU-BT.2111 color bar generator for VapourSynth", VS_MAKE_VERSION(1, 0), VAPOURSYNTH_API_VERSION, 0, plugin );
    vspapi->registerFunction( "ColorBars",
                              "resolution:int:opt;"
                              "format:int:opt;"
                              "hdr:int:opt;"
                              "wcg:int:opt;"
                              "compatability:int:opt;"
                              "subblack:int:opt;"
                              "superwhite:int:opt;"
                              "iq:int:opt;"
                              "halfline:int:opt;"
                              "filter:int:opt;",
                              "clip:vnode;",
                              colorbarsCreate, NULL, plugin );
}
