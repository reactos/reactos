
list(APPEND SOURCE
    arcname.c
    filesup.c
    fsutil.c
    genlist.c
    inicache.c
    ntverrsrc.c
    osdetect.c
    partlist.c
    precomp.h)

add_library(setuplib ${SOURCE})
add_pch(setuplib precomp.h SOURCE)
add_dependencies(setuplib xdk) # psdk
