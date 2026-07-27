# Source list mirrors libs/strmbase/Makefile.in upstream; keep them in sync on
# resync.
#
# Resynced to wine-10.1-dev (b00c7619f1b) together with <wine/strmbase.h> and
# the DirectShow DLLs that consume it. What used to be here was a much older
# fork with eight extra files -- audio.c, dllfunc.c, enumpins.c, outputqueue.c,
# qualitycontrol.c, transform.c, video.c, window.c -- whose contents upstream
# has since folded into quartz itself (quartz has its own window.c now). Those
# files are dead and should be deleted from the tree; they are left on disk for
# the moment but are deliberately not built, and they will not compile against
# the new header.

remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600 -D__WINESRC__)

list(APPEND SOURCE
    dispatch.c
    filter.c
    mediatype.c
    pin.c
    pospass.c
    renderer.c
    seeking.c)

add_library(strmbase ${SOURCE})
add_dependencies(strmbase psdk dxsdk)
