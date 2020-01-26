
spec2def(netid.dll netid.spec)

list(APPEND SOURCE
    netid.c
    netid.rc
    ${CMAKE_CURRENT_BINARY_DIR}/netid.def)

add_library(netid MODULE ${SOURCE})
set_module_type(netid win32dll UNICODE)
add_importlibs(netid user32 gdi32 comctl32 advapi32 netapi32 dnsapi msvcrt kernel32 ntdll)
add_cd_file(TARGET netid DESTINATION reactos/system32 FOR all)
