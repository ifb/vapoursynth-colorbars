AM_CFLAGS = -Wall -Wextra -Wno-unused-parameter -std=c99 -O3 -ffast-math $(MFLAGS)

AM_CPPFLAGS = $(VapourSynth_CFLAGS)

lib_LTLIBRARIES = libcolorbars.la
libcolorbars_la_SOURCES = colorbars.c
libcolorbars_la_LDFLAGS = -no-undefined -avoid-version $(PLUGINLDFLAGS)
