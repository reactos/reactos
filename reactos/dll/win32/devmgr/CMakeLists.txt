
set_rc_compiler()

spec2def(devmgr.dll devmgr.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    devmgr.rc
    advprop.c
    devprblm.c
    hwpage.c
    misc.c
    stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/devmgr_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/devmgr.def)

add_library(devmgr SHARED ${SOURCE})

set_module_type(devmgr win32dll UNICODE)

target_link_libraries(devmgr uuid wine)

add_importlibs(devmgr
    setupapi
    advapi32
    user32
    version
    msvcrt
    kernel32
    ntdll)

add_pch(devmgr precomp.h)

add_cd_file(TARGET devmgr DESTINATION reactos/system32 FOR all)

