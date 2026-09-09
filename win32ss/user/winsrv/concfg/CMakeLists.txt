
list(APPEND SOURCE
    font.c
    settings.c
    precomp.h)

add_library(concfg STATIC ${SOURCE})
add_pch(concfg precomp.h SOURCE)
add_dependencies(concfg xdk)
target_include_directories(concfg INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
