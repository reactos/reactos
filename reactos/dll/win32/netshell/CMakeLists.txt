add_definitions(-D_NETSHELL_)
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

set_rc_compiler()
spec2def(netshell.dll netshell.spec GENERATE_STUBS)

list(APPEND SOURCE
    netshell.c
    shfldr_netconnect.c
    enumlist.c
    netshell.rc
    classfactory.c
    connectmanager.c
    lanconnectui.c
    lanstatusui.c
    ${CMAKE_CURRENT_BINARY_DIR}/netshell_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/netshell.def)

add_library(netshell SHARED ${SOURCE})

set_module_type(netshell win32dll)

target_link_libraries(netshell
    uuid
    wine)

add_importlibs(netshell
    msvcrt
    shlwapi
    shell32
    version
    iphlpapi
    ole32
    user32
    advapi32
    setupapi
    ws2_32
    comctl32
    kernel32
    ntdll)

add_pch(netshell precomp.h)

add_cd_file(TARGET netshell DESTINATION reactos/system32 FOR all)
