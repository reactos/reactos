
list(APPEND SOURCE
    srclient_main.c)

add_library(srclient MODULE ${SOURCE})
spec2def(srclient srclient.dll srclient.spec)

set_module_type(srclient win32dll)
add_importlibs(srclient msvcrt kernel32 ntdll)
add_cd_file(TARGET srclient DESTINATION reactos/system32 FOR all)
