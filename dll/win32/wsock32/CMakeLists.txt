
add_library(wsock32 MODULE stubs.c wsock32.rc)
spec2def(wsock32 wsock32.dll wsock32.spec)

set_module_type(wsock32 win32dll UNICODE)
add_importlibs(wsock32 mswsock ws2_32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wsock32 DESTINATION reactos/system32 FOR all)
