spec2def(dcomlaunch.dll dcomlaunch.spec ADD_IMPORTLIB)

add_library(dcomlaunch MODULE
    dcomlaunch.c
    network.c
    dcomlaunch.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dcomlaunch.def)

set_module_type(dcomlaunch win32dll UNICODE)
add_importlibs(dcomlaunch advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dcomlaunch DESTINATION reactos/system32 FOR all)
