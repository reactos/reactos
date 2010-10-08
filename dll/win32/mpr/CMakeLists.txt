
add_definitions(-D__WINESRC__)
add_definitions(-D_WINE)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mpr ${CMAKE_CURRENT_SOURCE_DIR}/mpr.spec ${CMAKE_CURRENT_BINARY_DIR}/mpr.def)

add_library(mpr SHARED
    auth.c
    mpr_main.c
    multinet.c
    nps.c
    pwcache.c
    wnet.c
    mpr.rc)

set_module_type(mpr win32dll)

target_link_libraries(mpr
    ${CMAKE_CURRENT_BINARY_DIR}/mpr.def
    wine)

add_importlibs(mpr advapi32 user32 kernel32 ntdll)
add_dependencies(mpr mpr_def)
