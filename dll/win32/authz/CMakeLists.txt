
list(APPEND SOURCE
    authz.c
    clictx.c
    resman.c
    precomp.h)

add_library(authz MODULE
    ${SOURCE}
    authz.rc)
spec2def(authz authz.dll authz.spec)

set_module_type(authz win32dll UNICODE)
add_importlibs(authz advapi32 msvcrt kernel32 ntdll)
add_pch(authz precomp.h SOURCE)
add_cd_file(TARGET authz DESTINATION reactos/system32 FOR all)
