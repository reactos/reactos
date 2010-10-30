
set_unicode()

spec2def(slayer.dll slayer.spec)

list(APPEND SOURCE
    slayer.c
    slayer.rc
    ${CMAKE_CURRENT_BINARY_DIR}/slayer.def)

add_library(slayer SHARED ${CMAKE_CURRENT_BINARY_DIR}/slayer_precomp.h.gch ${SOURCE})

set_module_type(slayer win32dll)

target_link_libraries(slayer uuid)

add_importlibs(slayer
    user32
    comctl32
    advapi32
    ole32
    shell32
    kernel32
    ntdll)

add_pch(slayer ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

add_cab_target(slayer 1)
