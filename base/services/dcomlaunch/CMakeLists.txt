
add_library(dcomlaunch MODULE
    dcomlaunch.c
    network.c
    dcomlaunch.rc)
spec2def(dcomlaunch dcomlaunch.dll dcomlaunch.spec ADD_IMPORTLIB)

set_module_type(dcomlaunch win32dll UNICODE)
add_importlibs(dcomlaunch advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dcomlaunch DESTINATION reactos/system32 FOR all)
