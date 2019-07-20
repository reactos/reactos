spec2def(wuauserv.dll wuauserv.spec ADD_IMPORTLIB)

add_library(wuauserv MODULE
    wuauserv.c
    wuauserv.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wuauserv.def)

set_module_type(wuauserv win32dll UNICODE)
add_importlibs(wuauserv advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET wuauserv DESTINATION reactos/system32 FOR all)
