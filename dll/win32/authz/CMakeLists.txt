
set_unicode()

spec2def(authz.dll authz.spec)

list(APPEND SOURCE
    authz.c
    clictx.c
    resman.c
    authz.rc
    ${CMAKE_CURRENT_BINARY_DIR}/authz_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/authz.def)

add_library(authz SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/authz_precomp.h.gch
    ${SOURCE})

set_module_type(authz win32dll)

add_pch(authz ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})
add_importlibs(authz advapi32 msvcrt kernel32 ntdll)

add_cd_file(TARGET authz DESTINATION reactos/system32 FOR all)
