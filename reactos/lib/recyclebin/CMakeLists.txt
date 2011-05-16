
list(APPEND SOURCE
    guid.c
    recyclebin.c
    recyclebin_generic.c
    recyclebin_generic_enumerator.c
    recyclebin_v5.c
    recyclebin_v5_enumerator.c)

add_library(recyclebin ${SOURCE})
add_dependencies(recyclebin psdk)

