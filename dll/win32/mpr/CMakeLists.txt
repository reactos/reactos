
add_definitions(
    -D__WINESRC__
    -D_WINE)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    auth.c
    mpr_main.c
    multinet.c
    nps.c
    pwcache.c
    wnet.c
    precomp.h)

add_library(mpr MODULE
    ${SOURCE}
    mpr.rc)
spec2def(mpr mpr.dll mpr.spec ADD_IMPORTLIB)

set_module_type(mpr win32dll)
target_link_libraries(mpr wine)
add_importlibs(mpr advapi32 user32 msvcrt kernel32 ntdll)
add_pch(mpr precomp.h SOURCE)
add_cd_file(TARGET mpr DESTINATION reactos/system32 FOR all)
