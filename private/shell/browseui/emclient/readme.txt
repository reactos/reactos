event monitor client side

- files
the evtmon-supplied files are:
    shell/inc/
        emact.h
        emkwd.h
        emoci.h
        emocii.h
        emrule.h
        emrulini.h
        emruloci.h
        emrultk.h
        emutil.h

        mso.h
        msodbglg.h
        msoem.h
        msoemtyp.h
        msolex.h

    shell/lib/
        genem.c
    
app-specific client-side files are:
    ./
        libem.c         wrapper for genem.c (and rulc-generated files)
        ierules.rul     rules

- makefile
the build process is a bit confusing.

what we're trying to do is build two (logical) pieces:
    evtmon generic code (client-side)
    application-specific rules

libem.c contains both of these.  the generic code comes from genem.c
(#include from shell/lib); the app-specific rules come from ierules.rul
(genem.c does #include of $O/em*.h which is built w/ rulc.exe).

note that the em*.h files are DEBUG/RETAIL-dependent so they *must*
go in $O not in ".".

to make this happen w/ build.exe we have:
    NTTARGETFILE0 = $(O)\emeval.c   # force us to build ierules.rul
    SOURCES = libem.c               # 'real' SOURCE we eventually build
    C_DEFINES = -I$O                # generated stuff is here
