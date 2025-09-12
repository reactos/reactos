
spec2def(ifmon.dll ifmon.spec)

list(APPEND SOURCE
    guid.c
    ifmon.c
    ifmon.rc
    interface.c
    ip.c
    ${CMAKE_CURRENT_BINARY_DIR}/ifmon.def)

add_library(ifmon MODULE ${SOURCE})
set_module_type(ifmon win32dll UNICODE)
add_importlibs(ifmon netsh iphlpapi msvcrt kernel32 ntdll)
add_cd_file(TARGET ifmon DESTINATION reactos/system32 FOR all)
