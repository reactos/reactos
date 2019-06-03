
add_library(wmisvc MODULE
    wmisvc.c
    wmisvc.rc)
spec2def(wmisvc wmisvc.dll wmisvc.spec ADD_IMPORTLIB)

set_module_type(wmisvc win32dll UNICODE)
add_importlibs(wmisvc advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wmisvc DESTINATION reactos/system32/wbem FOR all)
