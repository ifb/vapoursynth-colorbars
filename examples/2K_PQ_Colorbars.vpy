import vapoursynth as vs

from vapoursynth import core

seconds = 60

# Generate 2K PQ Bars
c = core.colorbars.ColorBars(format=vs.RGB30, resolution=4, hdr=2)
c = core.std.Convolution(c,mode="h",matrix=[1,2,4,2,1])
c = core.resize.Point(clip=c,format=vs.YUV422P10,matrix_s="2020ncl")
c = c * (seconds * 24000 // 1001)
c = core.std.AssumeFPS(clip=c, fpsnum=24000, fpsden=1001)

c.set_output()
