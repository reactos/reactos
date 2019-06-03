
add_library(ksuser MODULE
    ksuser.c
    ksuser.rc)
spec2def(ksuser ksuser.dll ksuser.spec ADD_IMPORTLIB)

set_module_type(ksuser win32dll)
add_importlibs(ksuser advapi32 msvcrt kernel32 ntdll)
add_dependencies(ksuser bugcodes)
add_cd_file(TARGET ksuser DESTINATION reactos/system32 FOR all)
