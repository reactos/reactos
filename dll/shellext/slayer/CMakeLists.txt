
set_unicode()

spec2def(slayer ${CMAKE_CURRENT_SOURCE_DIR}/slayer.spec ${CMAKE_CURRENT_BINARY_DIR}/slayer.def)

list(APPEND SOURCE slayer.c slayer.rc)

add_library(slayer SHARED ${CMAKE_CURRENT_BINARY_DIR}/slayer_precomp.h.gch ${SOURCE})

set_module_type(slayer win32dll)

target_link_libraries(slayer ${CMAKE_CURRENT_BINARY_DIR}/slayer.def uuid)

add_importlibs(slayer
    user32
    comctl32
    advapi32
    ole32
    shell32
    kernel32
    ntdll)

add_pch(slayer ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(slayer slayer_def)
add_cab_target(slayer 1)
