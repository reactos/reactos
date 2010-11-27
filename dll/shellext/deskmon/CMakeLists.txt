
set_unicode()

spec2def(deskmon.dll deskmon.spec)

list(APPEND SOURCE
    deskmon.c
    shxiface.c
    deskmon.rc
    ${CMAKE_CURRENT_BINARY_DIR}/deskmon.def)

add_library(deskmon SHARED ${CMAKE_CURRENT_BINARY_DIR}/deskmon_precomp.h.gch ${SOURCE})

set_module_type(deskmon win32dll)

target_link_libraries(deskmon uuid)

add_importlibs(deskmon
    user32
    gdi32
    comctl32
    ole32
    msvcrt
    kernel32
    ntdll)

add_pch(deskmon ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

add_cab_target(deskmon 1)
