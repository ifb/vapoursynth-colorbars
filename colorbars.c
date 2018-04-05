/*****************************************************************************
 * colorbars: a vapoursynth plugin for generating SMPTE RP 219 color bars
 *****************************************************************************
 * VapourSynth plugin
 *     Copyright (C) 2018 Phillip Blucas
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

#include <VapourSynth.h>
#include <VSHelper.h>

typedef enum {
    NTSC = 0,
    PAL,
    HD720,
    HD1080,
    DCI2K,
    UHDTV1,
    DCI4K,
    UHDTV2
} system_type_e;

typedef enum {
    IQ_NONE = 0,
    IQ_BOTH,
    IQ_PLUS_I,
    IQ_WHITE
} iq_mode_e;

typedef struct {
    VSVideoInfo vi;
    VSNodeRef *node;
    system_type_e resolution;
    int wcg;
    int compatability;
    int subblack;
    int superwhite;
    iq_mode_e iq;
    int filter;
} ColorBarsData;

static void VS_CC colorbarsInit( VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi )
{
    ColorBarsData *d = (ColorBarsData *) * instanceData;
    vsapi->setVideoInfo( &d->vi, 1, node );
}

static const VSFrameRef *VS_CC colorbarsGetFrame( int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi )
{
    ColorBarsData *d = (ColorBarsData *) * instanceData;
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
        // [resolution][compatability][bar width]
        const int p1_widths[8][3][10] = { { {   4, 101, 102, 102, 102, 102, 102, 101,   4 }, // 525-line (NTSC)
                                            {   4, 102, 102, 102, 100, 102, 102, 102,   4 },
                                            {   0, 104, 102, 102, 102, 104, 102, 104,   0 } },
                                          { {   9,  87,  88,  88,  88,  88,  88,  88,  87,   9 }, // 625-line (PAL)
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
                                            { 944, 840, 824, 824, 816, 824, 824, 840, 944 } } };

        // [resolution][compatability][bar width]
        const int p4_widths[8][3][11] = { { {   4, 127, 128, 127, 128,  34,  34,  34, 101,   4,   0 }, // 525-line (NTSC)
                                            {   4, 126, 128, 126, 128,  34,  34,  34, 102,   4,   0 },
                                            {   0, 128, 130, 128, 128,  34,  34,  34, 104,   0,   0 } },
                                          { {   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 }, // 625-line (PAL)
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
                                            { 944, 1248,1648,680, 272, 280, 272, 280, 272, 840, 944 } } };

        const int compat = d->compatability;
        const int resolution = d->resolution;
        const int wcg = d->wcg;
        const int depth = d->vi.format->bitsPerSample == 10 ? 0 : 1;
        const int iq = d->iq;
        const int height = d->vi.height;
        const int width = d->vi.width;

        VSFrameRef *frame = 0;
        frame = vsapi->newVideoFrame(d->vi.format, width, height, 0, core);
        if (resolution < HD720)
        {
            vsapi->propSetInt(vsapi->getFramePropsRW(frame), "_Matrix", resolution == PAL ? 5 : 6, paReplace);
            vsapi->propSetInt(vsapi->getFramePropsRW(frame), "_Primaries", resolution == PAL ? 5 : 6, paReplace);
            vsapi->propSetInt(vsapi->getFramePropsRW(frame), "_Transfer", 6, paReplace);
        }
        else
        {
            vsapi->propSetInt(vsapi->getFramePropsRW(frame), "_Matrix", wcg ? 9 : 1, paReplace);
            vsapi->propSetInt(vsapi->getFramePropsRW(frame), "_Primaries", wcg ? 9 : 1, paReplace);
            vsapi->propSetInt(vsapi->getFramePropsRW(frame), "_Transfer", wcg ? 14 : 1, paReplace);
        }
        vsapi->propSetInt(vsapi->getFramePropsRW(frame), "_ColorRange", 1, paReplace); // limited

        uint16_t *y = (uint16_t *)vsapi->getWritePtr(frame, 0);
        uint16_t *u = (uint16_t *)vsapi->getWritePtr(frame, 1);
        uint16_t *v = (uint16_t *)vsapi->getWritePtr(frame, 2);
        intptr_t stride = vsapi->getStride(frame, 0) / sizeof(uint16_t);
        if (resolution == NTSC)
        {
            // pattern 1
            for (int h = 0; h < ntsc_heights[compat][0]; h++)
            {
                uint16_t *edge_y = y;
                uint16_t *edge_u = u;
                uint16_t *edge_v = v;
                for (int bar = 0; bar < 9; bar++)
                {
                    for (int i = 0; i < p1_widths[resolution][compat][bar]; i++, edge_y++, edge_u++, edge_v++)
                    {
                        *edge_y = ntsc1_y[depth][bar];
                        *edge_u = ntsc1_u[depth][bar];
                        *edge_v = ntsc1_v[depth][bar];
                    }
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
                {
                    for (int i = 0; i < p1_widths[resolution][compat][bar]; i++, edge_y++, edge_u++, edge_v++)
                    {
                        *edge_y = ntsc2_y[depth][bar];
                        *edge_u = ntsc2_u[depth][bar];
                        *edge_v = ntsc2_v[depth][bar];
                    }
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
                {
                    for (int i = 0; i < p4_widths[resolution][compat][bar]; i++, edge_y++, edge_u++, edge_v++)
                    {
                        *edge_y = ntsc3_y[depth][bar];
                        *edge_u = ntsc3_u[depth][bar];
                        *edge_v = ntsc3_v[depth][bar];
                    }
                }
                y += stride;
                u += stride;
                v += stride;
            }

        }
        else if (resolution == PAL)
        {
            for (int h = 0; h < height; h++)
            {
                uint16_t *edge_y = y;
                uint16_t *edge_u = u;
                uint16_t *edge_v = v;
                for (int bar = 0; bar < 10; bar++)
                {
                    for (int i = 0; i < p1_widths[resolution][compat][bar]; i++, edge_y++, edge_u++, edge_v++)
                    {
                        *edge_y = pal_y[depth][bar];
                        *edge_u = pal_u[depth][bar];
                        *edge_v = pal_v[depth][bar];
                    }
                }
                y += stride;
                u += stride;
                v += stride;
            }
        }
        else // HD and higher systems
        {
            // pattern 1
            for (int h = 0; h < height / 12 * 7; h++)
            {
                uint16_t *edge_y = y;
                uint16_t *edge_u = u;
                uint16_t *edge_v = v;
                for (int bar = 0; bar < 9; bar++)
                {
                    for (int i = 0; i < p1_widths[resolution][compat][bar]; i++, edge_y++, edge_u++, edge_v++)
                    {
                        *edge_y = p1_y[wcg][depth][bar];
                        *edge_u = p1_u[wcg][depth][bar];
                        *edge_v = p1_v[wcg][depth][bar];
                    }
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
                    *edge_y = p3_y[wcg][depth][2] + i * slope;
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
                    *edge_y = p4_y[depth][1] - i * slope;
                    *edge_u = p4_u[depth][1];
                    *edge_v = p4_v[depth][1];
                }
                for (int i = 0; i < rampwidth; i++, edge_y++, edge_u++, edge_v++)
                {
                    *edge_y = p4_y[depth][1 + subblack * 10] + i * slope;
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
                    *edge_y = p4_y[depth][2] + i * slope;
                    *edge_u = p4_u[depth][2];
                    *edge_v = p4_v[depth][2];
                }
                for (int i = 0; i < rampwidth; i++, edge_y++, edge_u++, edge_v++)
                {
                    *edge_y = p4_y[depth][2 + superwhite * 10] - i * slope;
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
    vsapi->freeNode( d->node );
    free( d );
}

static void VS_CC colorbarsCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    ColorBarsData d = { 0 };
    ColorBarsData *data;

    int err = 0;
    d.compatability = vsapi->propGetInt(in, "compatability", 0, &err);
    if (err)
        d.compatability = 2;
    if (d.compatability < 0 || d.compatability > 2) {
        vsapi->setError(out, "ColorBars: invalid compatability mode");
        return;
    }
    d.resolution = vsapi->propGetInt(in, "resolution", 0, &err);
    if (err)
        d.resolution = HD1080;
    if (d.resolution < NTSC || d.resolution > UHDTV2) {
        vsapi->setError(out, "ColorBars: invalid resolution");
        return;
    }

    const int resolutions[8][2] = { {  720,  486 },
                                    {  720,  576 },
                                    { 1280,  720 },
                                    { 1920, 1080 },
                                    { 2048, 1080 },
                                    { 3840, 2160 },
                                    { 4096, 2160 },
                                    { 7680, 4320 } };
    d.vi.width = resolutions[d.resolution][0];
    d.vi.height = resolutions[d.resolution][1];
    if (d.compatability == 2 && d.resolution == NTSC)
        d.vi.height = 480;

    int pixformat = vsapi->propGetInt(in, "pixelformat", 0, &err);
    if (err)
        pixformat = pfYUV444P12;
    if (pixformat != pfYUV444P12 && pixformat != pfYUV444P10)
    {
        vsapi->setError(out, "ColorBars: invalid pixelformat, only YUV444P10 and YUV444P12 supported");
        return;
    }
    d.vi.format = vsapi->getFormatPreset(pixformat, core);

    d.subblack = vsapi->propGetInt(in, "subblack", 0, &err);
    if (err)
        d.subblack = 1;
    d.subblack = !!d.subblack;
    d.superwhite = vsapi->propGetInt(in, "superwhite", 0, &err);
    if (err)
        d.superwhite = 1;
    d.superwhite = !!d.superwhite;
    d.iq = vsapi->propGetInt(in, "iq", 0, &err);
    if (err)
        d.iq = d.resolution < UHDTV1 ? IQ_BOTH : IQ_NONE;
    if (d.iq < 0 || d.iq > 3)
    {
        vsapi->setError(out, "ColorBars: invalid I/Q mode");
        return;
    }

    d.wcg = vsapi->propGetInt(in, "wcg", 0, &err);
    if (err)
        d.wcg = 0;
    d.wcg = !!d.wcg;
    if (d.wcg)
    {
        if (d.resolution < UHDTV1)
        {
            vsapi->setError(out, "ColorBars: wide color (Rec.2020) only valid with UHDTV systems");
            return;
        }
        if (d.iq == IQ_BOTH || d.iq == IQ_PLUS_I)
        {
            vsapi->setError(out, "ColorBars: -I/+Q and +I not valid with wide color (Rec.2020)");
            return;
        }
    }
    if (d.resolution == UHDTV2)
    {
        if (!d.wcg)
            vsapi->logMessage(mtWarning, "ColorBars: wide color (Rec.2020) required with 8K/UHDTV2");
        if (d.iq == IQ_BOTH || d.iq == IQ_PLUS_I)
            vsapi->logMessage(mtWarning, "ColorBars: -I/+Q and +I not valid with 8K/UHDTV2 systems");
    }

    d.filter = vsapi->propGetInt(in, "filter", 0, &err);
    if (err)
        d.filter = 1;
    d.filter = !!d.filter;

    d.vi.fpsNum = 60000;
    d.vi.fpsDen = 1001;
    d.vi.numFrames = 1;
    d.vi.flags = 0;

    data = malloc(sizeof(d));
    *data = d;

    vsapi->createFilter( in, out, "ColorBars", colorbarsInit, colorbarsGetFrame, colorbarsFree, fmParallel, 0, data, core );
}

VS_EXTERNAL_API(void) VapourSynthPluginInit( VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin )
{
    configFunc( "com.ifb.colorbars", "colorbars", "SMPTE RP 219-2:2016 color bar generator for VapourSynth", VAPOURSYNTH_API_VERSION, 1, plugin );
    registerFunc( "ColorBars",
                  "resolution:int:opt;"
                  "pixelformat:int:opt;"
                  "wcg:int:opt;"
                  "compatability:int:opt;"
                  "subblack:int:opt;"
                  "superwhite:int:opt;"
                  "iq:int:opt;"
                  "filter:int:opt;",
                  colorbarsCreate, 0, plugin );
}
