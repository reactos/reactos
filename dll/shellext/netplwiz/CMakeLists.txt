
list(APPEND SOURCE
    netplwiz.c
    SHDisconnectNetDrives.c)

add_library(netplwiz MODULE
    ${SOURCE}
    netplwiz.rc)
spec2def(netplwiz netplwiz.dll netplwiz.spec ADD_IMPORTLIB)

set_module_type(netplwiz win32dll UNICODE)
target_link_libraries(netplwiz wine)
add_importlibs(netplwiz comctl32 mpr ole32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET netplwiz DESTINATION reactos/system32 FOR all)
