
spec2def(netplwiz.dll netplwiz.spec)

list(APPEND SOURCE
    netplwiz.c
    SHDisconnectNetDrives.c)

add_library(netplwiz SHARED
    ${SOURCE}
    netplwiz.rc
    ${CMAKE_CURRENT_BINARY_DIR}/netplwiz_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/netplwiz.def)

set_module_type(netplwiz win32dll UNICODE)
target_link_libraries(netplwiz wine)
add_importlibs(netplwiz comctl32 mpr ole32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET netplwiz DESTINATION reactos/system32 FOR all)
