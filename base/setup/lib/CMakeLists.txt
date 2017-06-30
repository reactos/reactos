
list(APPEND SOURCE
    arcname.c
    bldrsup.c
    filesup.c
    fsutil.c
    genlist.c
    inicache.c
    ntverrsrc.c
    osdetect.c
    partlist.c
    registry.c
    regutil.c
    precomp.h)

add_library(setuplib ${SOURCE})
add_pch(setuplib precomp.h SOURCE)
add_dependencies(setuplib xdk) # psdk
