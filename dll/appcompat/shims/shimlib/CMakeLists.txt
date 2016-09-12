
list(APPEND SOURCE
    shimlib.c
    shimlib.h)

add_library(shimlib ${SOURCE})
add_dependencies(shimlib xdk)
