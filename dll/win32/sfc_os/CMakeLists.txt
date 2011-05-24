
spec2def(sfc_os.dll sfc_os.spec)

list(APPEND SOURCE
    sfc_os.c
    ${CMAKE_CURRENT_BINARY_DIR}/sfc_os_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/sfc_os.def)

add_library(sfc_os SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/sfc_os_precomp.h.gch
    ${SOURCE})

set_module_type(sfc_os win32dll)

add_importlibs(sfc_os msvcrt kernel32 ntdll)
add_pch(sfc_os ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

add_cd_file(TARGET sfc_os DESTINATION reactos/system32 FOR all)
