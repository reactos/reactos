
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    config.c
    debug_ros.c
    loader.c
    string.c)

add_library(wine ${SOURCE})

add_library(wineldr loader.c)
