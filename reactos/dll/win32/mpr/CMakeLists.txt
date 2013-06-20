
add_definitions(
    -D__WINESRC__
    -D_WINE)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(mpr.dll mpr.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    auth.c
    mpr_main.c
    multinet.c
    nps.c
    pwcache.c
    wnet.c
    mpr.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mpr_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mpr.def)

add_library(mpr SHARED ${SOURCE})
set_module_type(mpr win32dll)
target_link_libraries(mpr wine)
add_importlibs(mpr advapi32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mpr DESTINATION reactos/system32 FOR all)
