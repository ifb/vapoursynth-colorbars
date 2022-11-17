import vapoursynth as vs

from vapoursynth import core

seconds = 60

# Generate 625 PAL Bars
c = core.colorbars.ColorBars(format=vs.YUV444P12, resolution=1, compatability=1)
c = core.std.SetFrameProp(clip=c, prop="_FieldBased", intval=vs.FIELD_TOP)
c = core.std.Convolution(c,mode="h",matrix=[1,2,4,2,1])
c = core.resize.Point(clip=c,format=vs.YUV422P10)
c = c * (seconds * 25 // 1)
c = core.std.AssumeFPS(clip=c, fpsnum=25, fpsden=1)

c.set_output(alt_output=1)  # enable v210
