This directory is the DVA code from VFW 1.1

what this directory builds is a static link library (DVA.LIB) that contains
one API DVAGetSurface.

DVAGetSurface() first checks the DISPLAY driver (via a escape) and if that
fails it has built in support for SVGAs and some other cards built in.

what this builds:

        DVA.LIB         statis link
        DVA.H           header for lib (a DVA client would use this)
        DVADDI.H        defines structures and constants common to
                        DVA clients and DVA providers.

        TEST\DVATST.EXE Test app to make sure all works.

files:
        dva.h               - public header(s)
        dvaddi.h
        dvaddi.inc

        makefile            - builds DVA.LIB and DVATST.EXE

        dva.c               - DVAGetSurfaceCode has other routines
                              used to verify a surface is valid.

        lockbm.c            - Internal utility functions
        lockbm.h

        dvavga.c            - support for SVGAs
        vflat.asm

        dvaati.c            - support for ATI Mach32

        dvathun.c           - support for SuperMac Thunder/24

        dvavlb.c            - support for Viper VLB

        dvadib.c            - support for *Beta* Chicago display driver
        dibeng.inc

        dvaclip.c           - unused/unfinished/undone code.
