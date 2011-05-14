
set_unicode()

spec2def(vdmdbg.dll vdmdbg.spec)

list(APPEND SOURCE
    vdmdbg.c
    ${CMAKE_CURRENT_BINARY_DIR}/vdmdbg.def)

add_library(vdmdbg SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/vdmdbg_vdmdbg.h.gch
    ${SOURCE})

set_module_type(vdmdbg win32dll)
add_importlibs(vdmdbg msvcrt kernel32 ntdll)
add_pch(vdmdbg ${CMAKE_CURRENT_SOURCE_DIR}/vdmdbg.h ${SOURCE})
add_cd_file(TARGET vdmdbg DESTINATION reactos/system32 FOR all)
