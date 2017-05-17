
list(APPEND SOURCE
    fsutil.c
    genlist.c
    partlist.c
    precomp.h)

add_library(setuplib ${SOURCE})
add_pch(setuplib precomp.h SOURCE)
add_dependencies(setuplib xdk) # psdk
