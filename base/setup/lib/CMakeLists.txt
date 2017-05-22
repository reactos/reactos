
list(APPEND SOURCE
    arcname.c
    fsutil.c
    genlist.c
    ntverrsrc.c
    partlist.c
    precomp.h)

add_library(setuplib ${SOURCE})
add_pch(setuplib precomp.h SOURCE)
add_dependencies(setuplib xdk) # psdk
