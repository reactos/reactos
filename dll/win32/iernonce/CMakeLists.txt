
add_library(iernonce MODULE
    iernonce.c
    iernonce.rc)
spec2def(iernonce iernonce.dll iernonce.spec)

set_module_type(iernonce win32dll UNICODE)
add_importlibs(iernonce msvcrt kernel32 ntdll)
add_cd_file(TARGET iernonce DESTINATION reactos/system32 FOR all)
