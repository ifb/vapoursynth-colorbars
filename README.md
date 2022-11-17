Description
===========

ColorBars is a filter for generating test signals.  The output is a single frame of color bars according to SMPTE RP 219-1, 219-2, or ITU-R BT.2111-2.  For NTSC, the bar pattern is described in SMPTE EG 1.  For PAL, EBU bars are generated.

SMPTE RP 219-2 gives explicit color bar values in 10-bit and 12-bit Y'Cb'Cr'.  ITU BT.2111-2 gives explicit color bar values in 10-bit and 12-bit R'G'B'.  These values are used directly instead of being generated at runtime.


Usage
=====

    colorbars.ColorBars([int resolution=3, int format=vs.YUV444P12, int hdr=0, int wcg=0, int compatability=2, int subblack=1, int superwhite=1, int iq=1])

* resolution: Eight different systems are supported as follows
   * 0 - NTSC
   * 1 - PAL
   * 2 - 720p
   * 3 - 1080i/p
   * 4 - 2K
   * 5 - UHDTV1/UHD
   * 6 - 4K
   * 7 - UHDTV2/8K

* format: Either vs.YUV444P10 or vs.YUV444P12 are supported in SDR mode. Either vs.RGB30 or vs.RGB36 are supported in HDR mode. This is because SMPTE defines bar values in terms of Y'Cb'Cr' and ITU uses R'G'B'.

* hdr: Non-zero values enable BT.2111 HDR mode as follows
   * 0 - SDR
   * 1 - HLG
   * 2 - PQ
   * 3 - PQ (full range)

* wcg: Enable ITU-R BT.2020, aka wide color gamut.  Only valid with UHD and higher resolutions.  Required for 8K, although ColorBars does not enforce this and will generate 8K Rec.709 with a warning.  No effect when hdr > 0.

* compatability: Controls how pedantic you want to be, especially for legacy NTSC/PAL systems.  No effect when hdr > 0.
   * 0 - Use bar dimensions that are nearest integer values to the ideal.  Bar widths are specified as fractions of the active picture.  They can therefore be odd and can cause problems with later chroma subsampling (conversions to YUV420 or YUV422 may be problematic).
   * 1 - Round dimensions to be even to work around the reality of chroma subsampling.
   * 2 - For NTSC and PAL, assume an active image of 720x480 and 720x576 respectively.  For HD and higher resolutions, use dimensions that are compatible with chroma subsampling and with 4:3 center-cut downconversion.  For UHD/4K and 8K, use multiples of four and eight respectively for 2SI compatibility.

NTSC and PAL are commonly 720 or 704 pixels wide due to MPEG-2 mod16 considerations, however the active pixel count is neither.
525-line NTSC is 710.85x484 with two half lines. 712x486 is used in modes 0 and 1 with 4 black pixels on each side.
625-line PAL is 702x574 with two half lines.  702x576 is used in mode 0, with 9 black pixels on each side.  704x576 is used in mode 1, with 8 black pixels on each side.

* subblack: Controls whether to generate the below black ramp in the middle third of the first 0% black patch on the bottom row.  Only valid with HD and higher resolutions.  No effect when hdr > 0.

* superwhite: Controls whether to generate an above white ramp the middle third of the 100% white chip on the bottom row.  Only valid with HD and higher resolutions.  No effect when hdr > 0.

* iq: Controls the second patch of rows 2 and 3.  Only valid with HD and higher resolutions.  No effect when hdr > 0.  Mode 1 and 2 are not valid if wcg=1.
   * 0 - 75% white and 0% black
   * 1 - -I and +Q
   * 2 - +I and 0% black
   * 3 - 100% white and 0% black

The +I/-I/+Q values are conveniently specified in RP 219 for systems with Rec.709 primaries (HD up to 4K).  For NTSC, the values were calculated and later verified to match an Evertz SDI test signal generator.  Note that converting from YUV to RGB will produce out of range values.

Examples
=====
Note that bar transitions are not instant.  RP 219 requires proper shaping.  Rise and fall times are 4 samples (10% to 90%) and +/-10% of the nominal value and the shape is recommended to be an integrated sine-squared pulse.  Shaping may be integrated into ColorBars later, but for now you can apply a horizontal blur.

    # Generate 30 seconds of 1080i HD bars
    c = core.colorbars.ColorBars(format=vs.YUV444P10)
    c = core.std.SetFrameProp(clip=c, prop="_FieldBased", intval=2) # top field first
    c = core.std.Convolution(c,mode="h",matrix=[1,2,4,2,1])
    c = core.resize.Point(clip=c,format=vs.YUV422P10)
    c = c * (30 * 30000 // 1001)
    c = core.std.AssumeFPS(clip=c, fpsnum=30000, fpsden=1001)
    
    # Generate 60 seconds of annoyingly "correct" NTSC bars
    c = core.colorbars.ColorBars(format=vs.YUV444P12, resolution=0, compatability=0)
    c = core.std.SetFrameProp(clip=c, prop="_FieldBased", intval=1) # bottom field first
    c = core.std.Crop(c, 4, 4)
    c = core.std.Convolution(c,mode="h",matrix=[1,2,4,2,1])
    c = core.resize.Point(clip=c,format=vs.YUV422P8)
    c = c * (60 * 30000 // 1001)
    c = core.std.AssumeFPS(clip=c, fpsnum=30000, fpsden=1001)
    
    # Generate UHD Bars with Rec.2020 primaries
    c = core.colorbars.ColorBars(format=vs.YUV444P12, resolution=5, wcg=1)
    c = core.std.Convolution(c,mode="h",matrix=[1,2,4,2,1])
    c = core.resize.Point(clip=c,format=vs.YUV422P10)
    c = core.std.AssumeFPS(clip=c, fpsnum=50, fpsden=1)

    # Generate HLG UHD Bars
    c = core.colorbars.ColorBars(format=vs.RGB30, resolution=5, hdr=1)
    c = core.std.Convolution(c,mode="h",matrix=[1,2,4,2,1])
    c = core.resize.Point(clip=c,format=vs.YUV422P10,matrix_s="2020ncl")

Compilation
===========
The usual autotools method:
```
./autogen.sh
./configure
make
```

On Mingw-w64 you can try something like the following:
```
gcc -c colorbars.c -I include/vapoursynth -O3 -ffast-math -mfpmath=sse -msse2 -march=native -std=c99 -Wall
gcc -shared -o colorbars.dll colorbars.o -Wl,--out-implib,colorbars.a
```
You'll probably need this for Win32 stdcall:
```
gcc -shared -o colorbars.dll colorbars.o -Wl,--kill-at,--out-implib,colorbars.a
```