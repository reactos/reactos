
set_unicode()
set_rc_compiler()

spec2def(devmgr.dll devmgr.spec)

list(APPEND SOURCE
    devmgr.rc
    advprop.c
    devprblm.c
    hwpage.c
    misc.c
    stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/devmgr_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/devmgr.def)

add_library(devmgr SHARED ${CMAKE_CURRENT_BINARY_DIR}/devmgr_precomp.h.gch ${SOURCE})

set_module_type(devmgr win32dll)

target_link_libraries(devmgr uuid wine)

add_importlibs(devmgr
    setupapi
    advapi32
    user32
    version
    msvcrt
    kernel32
    ntdll)

add_pch(devmgr ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

add_cd_file(TARGET devmgr DESTINATION reactos/system32 FOR all)
add_importlib_target(devmgr.spec)
