
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    config.c
    debug_ros.c
    loader.c
    wctype.c
    register.c
#    string.c implements _stricmp, already shipped with crt
)

add_library(wine ${SOURCE})
add_dependencies(wine psdk)

add_library(wineldr loader.c)
