
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

add_library(getuname MODULE
    getuname.c
    getuname.rc)
spec2def(getuname getuname.dll getuname.spec ADD_IMPORTLIB)

set_module_type(getuname win32dll UNICODE)
add_importlibs(getuname user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET getuname DESTINATION reactos/system32 FOR all)
