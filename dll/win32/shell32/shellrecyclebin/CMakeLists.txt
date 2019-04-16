project(SHELL)

add_definitions(
    -DUNICODE
    -D_UNICODE
    -D_ATL_NO_EXCEPTIONS)

list(APPEND SOURCE
    recyclebin.c
    recyclebin_generic.c
    recyclebin_generic_enumerator.c
    recyclebin_v5.c
    recyclebin_v5_enumerator.c
    recyclebin_private.h)

add_library(recyclebin ${SOURCE} guid.c)
add_pch(recyclebin recyclebin_private.h SOURCE)
add_dependencies(recyclebin psdk)
