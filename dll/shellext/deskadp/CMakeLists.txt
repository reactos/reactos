
set_unicode()

spec2def(deskadp ${CMAKE_CURRENT_SOURCE_DIR}/deskadp.spec ${CMAKE_CURRENT_BINARY_DIR}/deskadp.def)

list(APPEND SOURCE
    deskadp.c
    shxiface.c
    deskadp.rc)

add_library(deskadp SHARED ${CMAKE_CURRENT_BINARY_DIR}/deskadp_precomp.h.gch ${SOURCE})

set_module_type(deskadp win32dll)

target_link_libraries(deskadp ${CMAKE_CURRENT_BINARY_DIR}/deskadp.def uuid)

add_importlibs(deskadp
    user32
    gdi32
    comctl32
    ole32
    kernel32
    ntdll)

add_pch(deskadp ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(deskadp deskadp_def)
add_livecd_target(deskadp reactos/system32)
