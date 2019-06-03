
add_library(batt MODULE
    batt.c
    batt.rc)
spec2def(batt batt.dll batt.spec)

set_module_type(batt win32dll UNICODE)
add_importlibs(batt msvcrt kernel32 ntdll)
add_cd_file(TARGET batt DESTINATION reactos/system32 FOR all)
