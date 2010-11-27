
set_unicode()

spec2def(deskadp.dll deskadp.spec)

list(APPEND SOURCE
    deskadp.c
    shxiface.c
    deskadp.rc
    ${CMAKE_CURRENT_BINARY_DIR}/deskadp.def)

add_library(deskadp SHARED ${CMAKE_CURRENT_BINARY_DIR}/deskadp_precomp.h.gch ${SOURCE})

set_module_type(deskadp win32dll)

target_link_libraries(deskadp uuid)

add_importlibs(deskadp
    user32
    gdi32
    comctl32
    ole32
    msvcrt
    kernel32
    ntdll)

add_pch(deskadp ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

add_cab_target(deskadp 1)
