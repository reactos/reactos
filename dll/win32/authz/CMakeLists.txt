
spec2def(authz.dll authz.spec)

list(APPEND SOURCE
    authz.c
    clictx.c
    resman.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/authz_stubs.c)

add_library(authz MODULE
    ${SOURCE}
    authz.rc
    ${CMAKE_CURRENT_BINARY_DIR}/authz.def)

set_module_type(authz win32dll UNICODE)
add_importlibs(authz advapi32 msvcrt kernel32 ntdll)
add_pch(authz precomp.h SOURCE)
add_cd_file(TARGET authz DESTINATION reactos/system32 FOR all)
