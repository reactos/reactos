
list(APPEND SOURCE
    dmilib.c
    precomp.h)

add_library(dmilib ${SOURCE})
add_pch(dmilib precomp.h SOURCE)
add_dependencies(dmilib psdk)
