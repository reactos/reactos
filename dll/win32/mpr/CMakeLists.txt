
add_definitions(-D__WINESRC__)
add_definitions(-D_WINE)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(mpr.dll mpr.spec)

add_library(mpr SHARED
    auth.c
    mpr_main.c
    multinet.c
    nps.c
    pwcache.c
    wnet.c
    mpr.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mpr.def)

set_module_type(mpr win32dll)

target_link_libraries(mpr wine)

add_importlibs(mpr advapi32 user32 kernel32 ntdll)

add_cab_target(mpr 1)
