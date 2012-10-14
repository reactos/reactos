remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

set_rc_compiler()
spec2def(netcfgx.dll netcfgx.spec)

list(APPEND SOURCE
    netcfgx.c
    classfactory.c
    netcfg_iface.c
    inetcfgcomp_iface.c
    tcpipconf_notify.c
    netcfgx.rc
    ${CMAKE_CURRENT_BINARY_DIR}/netcfgx_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/netcfgx.def)

add_library(netcfgx SHARED ${SOURCE})

set_module_type(netcfgx win32dll)

target_link_libraries(netcfgx
    uuid
    wine)

add_importlibs(netcfgx rpcrt4 setupapi advapi32 iphlpapi dhcpcsvc ole32 user32 comctl32 ws2_32 msvcrt kernel32 ntdll)
add_pch(netcfgx precomp.h)
add_cd_file(TARGET netcfgx DESTINATION reactos/system32 FOR all)
