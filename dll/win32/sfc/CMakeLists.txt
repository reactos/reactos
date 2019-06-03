
list(APPEND SOURCE
    sfc.c)

add_library(sfc MODULE ${SOURCE})
spec2def(sfc sfc.dll sfc.spec)

set_module_type(sfc win32dll)
add_importlibs(sfc sfc_os msvcrt kernel32 ntdll)
add_cd_file(TARGET sfc DESTINATION reactos/system32 FOR all)
