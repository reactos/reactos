
list(APPEND SOURCE
    sfc_os.c)

add_library(sfc_os MODULE ${SOURCE})
spec2def(sfc_os sfc_os.dll sfc_os.spec ADD_IMPORTLIB)

set_module_type(sfc_os win32dll)
add_importlibs(sfc_os msvcrt kernel32 ntdll)
add_cd_file(TARGET sfc_os DESTINATION reactos/system32 FOR all)
