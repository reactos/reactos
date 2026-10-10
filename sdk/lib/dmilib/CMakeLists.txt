
list(APPEND SOURCE
    dmilib.c
    precomp.h)

add_library(dmilib STATIC ${SOURCE})
add_pch(dmilib precomp.h SOURCE)
add_dependencies(dmilib psdk)
target_include_directories(dmilib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
