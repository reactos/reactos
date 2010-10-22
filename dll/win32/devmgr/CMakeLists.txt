
set_unicode()

spec2def(devmgr ${CMAKE_CURRENT_SOURCE_DIR}/devmgr.spec ${CMAKE_CURRENT_BINARY_DIR}/devmgr.def)

list(APPEND SOURCE
    devmgr.rc
    advprop.c
    devprblm.c
    hwpage.c
    misc.c
    stubs.c)

add_library(devmgr SHARED ${CMAKE_CURRENT_BINARY_DIR}/devmgr_precomp.h.gch ${SOURCE})

set_module_type(devmgr win32dll)

target_link_libraries(devmgr
    ${CMAKE_CURRENT_BINARY_DIR}/devmgr.def
    uuid
    wine)

add_importlibs(devmgr
    setupapi
    advapi32
    user32
    version
    kernel32
    ntdll)

add_pch(devmgr ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(devmgr devmgr_def)
add_livecd_target(devmgr reactos/system32)
