
set_unicode()

list(APPEND SOURCE vdmdbg.c vdmdbg.def)

if(NOT MSVC)
    set_source_files_properties(vdmdbg.def PROPERTIES EXTERNAL_OBJECT TRUE)
endif()

add_library(vdmdbg SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/vdmdbg_vdmdbg.h.gch
    ${SOURCE})

set_module_type(vdmdbg win32dll)

add_importlibs(vdmdbg msvcrt kernel32 ntdll)
add_pch(vdmdbg ${CMAKE_CURRENT_SOURCE_DIR}/vdmdbg.h ${SOURCE})

add_cab_target(vdmdbg 1)
