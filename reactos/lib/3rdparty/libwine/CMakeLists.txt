
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

list(APPEND SOURCE
    config.c
    debug_ros.c
    isinf.c
    isfinite.c
    isnan.c
    loader.c
    ${REACTOS_SOURCE_DIR}/lib/sdk/crt/string/wctype.c
    register.c
#    string.c implements _stricmp, already shipped with crt
)

add_library(wine ${SOURCE})
add_dependencies(wine psdk)

add_library(wineldr loader.c)
add_dependencies(wineldr xdk)
