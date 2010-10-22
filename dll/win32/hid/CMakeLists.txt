
set_unicode()

list(APPEND SOURCE hid.c stubs.c hid.rc)

spec2def(hid ${CMAKE_CURRENT_SOURCE_DIR}/hid.spec ${CMAKE_CURRENT_BINARY_DIR}/hid.def)

add_library(hid SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/hid_precomp.h.gch
    ${SOURCE})

set_module_type(hid win32dll)

target_link_libraries(hid ${CMAKE_CURRENT_BINARY_DIR}/hid.def)

add_importlibs(hid kernel32 ntdll)
add_pch(hid ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(hid hid_def)
add_livecd_target(hid reactos/system32)
