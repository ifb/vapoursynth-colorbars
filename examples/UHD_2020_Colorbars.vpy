import vapoursynth as vs

from vapoursynth import core

seconds = 60

# Generate UHD SDR Bars with Rec.2020 primaries
c = core.colorbars.ColorBars(format=vs.YUV444P12, resolution=5, wcg=1)
c = core.std.Convolution(c,mode="h",matrix=[1,2,4,2,1])
c = core.resize.Point(clip=c,format=vs.YUV422P10)
c = c * (seconds * 60000 // 1001)
c = core.std.AssumeFPS(clip=c, fpsnum=60000, fpsden=1001)

c.set_output()
