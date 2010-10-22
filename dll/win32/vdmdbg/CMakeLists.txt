
set_unicode()

spec2def(vdmdbg ${CMAKE_CURRENT_SOURCE_DIR}/vdmdbg.spec ${CMAKE_CURRENT_BINARY_DIR}/vdmdbg.def)

list(APPEND SOURCE vdmdbg.c)

add_library(vdmdbg SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/vdmdbg_vdmdbg.h.gch
    ${SOURCE})

set_module_type(vdmdbg win32dll)

target_link_libraries(vdmdbg ${CMAKE_CURRENT_BINARY_DIR}/vdmdbg.def)

add_importlibs(vdmdbg kernel32 ntdll)
add_pch(vdmdbg ${CMAKE_CURRENT_SOURCE_DIR}/vdmdbg.h ${SOURCE})
add_dependencies(vdmdbg vdmdbg_def)
add_livecd_target(vdmdbg reactos/system32)
