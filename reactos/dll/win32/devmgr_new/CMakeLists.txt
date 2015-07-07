
spec2def(devmgr.dll devmgr.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    advprop.c
    devprblm.c
    hwpage.c
    hwresource.c
    misc.c
    stubs.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/devmgr_stubs.c)

add_library(devmgr SHARED
    ${SOURCE}
    devmgr.rc
    ${CMAKE_CURRENT_BINARY_DIR}/devmgr.def)

set_module_type(devmgr win32dll UNICODE)
target_link_libraries(devmgr uuid wine)
add_importlibs(devmgr setupapi advapi32 newdev user32 version msvcrt kernel32 ntdll)
add_pch(devmgr precomp.h SOURCE)
add_cd_file(TARGET devmgr DESTINATION reactos/system32 FOR all)
