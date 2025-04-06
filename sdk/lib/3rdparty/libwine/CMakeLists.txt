
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    config.c
    debug_ros.c
    isinf.c
    isfinite.c
    isnan.c
    loader.c
    ${REACTOS_SOURCE_DIR}/sdk/lib/crt/string/wctype.c
    path.c
    register.c
#    string.c implements _stricmp, already shipped with crt
)

add_library(wine ${SOURCE})
add_dependencies(wine psdk)
target_compile_definitions(wine PRIVATE _WINE)

add_library(wineldr loader.c)
add_dependencies(wineldr xdk)

add_library(wine_dll_register dll_register.c ${SOURCE})
add_dependencies(wine_dll_register psdk)
target_compile_definitions(wine_dll_register PRIVATE _WINE)
