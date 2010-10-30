
set_unicode()

spec2def(deskmon ${CMAKE_CURRENT_SOURCE_DIR}/deskmon.spec ${CMAKE_CURRENT_BINARY_DIR}/deskmon.def)

list(APPEND SOURCE
    deskmon.c
    shxiface.c
    deskmon.rc)

add_library(deskmon SHARED ${CMAKE_CURRENT_BINARY_DIR}/deskmon_precomp.h.gch ${SOURCE})

set_module_type(deskmon win32dll)

target_link_libraries(deskmon ${CMAKE_CURRENT_BINARY_DIR}/deskmon.def uuid)

add_importlibs(deskmon
    user32
    gdi32
    comctl32
    ole32
    kernel32
    ntdll)

add_pch(deskmon ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_dependencies(deskmon deskmon_def)
add_cab_target(deskmon 1)
