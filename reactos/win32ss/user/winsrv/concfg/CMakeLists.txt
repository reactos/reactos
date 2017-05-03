
list(APPEND SOURCE
    font.c
    settings.c
    precomp.h)

add_library(concfg ${SOURCE})
add_pch(concfg precomp.h SOURCE)
add_dependencies(concfg xdk)
