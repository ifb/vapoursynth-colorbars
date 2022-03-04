import vapoursynth as vs

from vapoursynth import core

seconds = 60

# Generate HD 1080i Bars
c = core.colorbars.ColorBars(format=vs.YUV444P10)
c = core.std.SetFrameProp(clip=c, prop="_FieldBased", intval=2) # top field first
c = core.std.Convolution(c,mode="h",matrix=[1,2,4,2,1])
c = core.resize.Point(clip=c,format=vs.YUV422P10)
c = c * (seconds * 30000 // 1001)
c = core.std.AssumeFPS(clip=c, fpsnum=30000, fpsden=1001)

c.set_output(alt_output=1)  # enable v210
