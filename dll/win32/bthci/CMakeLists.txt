
add_library(bthci MODULE
    bthci.c
    bthci.rc)
spec2def(bthci bthci.dll bthci.spec)

set_module_type(bthci win32dll UNICODE)
add_importlibs(bthci msvcrt kernel32 ntdll)
add_cd_file(TARGET bthci DESTINATION reactos/system32 FOR all)
