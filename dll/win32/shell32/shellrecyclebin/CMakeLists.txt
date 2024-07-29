project(SHELL)

add_definitions(
    -DUNICODE
    -D_UNICODE)

list(APPEND SOURCE
    recyclebin.c
    recyclebin_generic.cpp
    recyclebin_generic_enumerator.cpp
    recyclebin_v5.cpp
    recyclebin_v5_enumerator.cpp
    recyclebin_private.h)

add_library(recyclebin ${SOURCE} guid.c)
target_link_libraries(recyclebin PRIVATE atl_classes)
add_pch(recyclebin recyclebin_private.h SOURCE)
add_dependencies(recyclebin psdk)
target_link_libraries(recyclebin PRIVATE atl_classes)
