
list(APPEND SOURCE
    udmihelp.c
    precomp.h)

add_library(udmihelp STATIC ${SOURCE})
add_pch(udmihelp precomp.h SOURCE)
target_include_directories(udmihelp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(udmihelp dmilib)
add_dependencies(udmihelp psdk)
