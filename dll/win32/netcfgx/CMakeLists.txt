
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(netcfgx.dll netcfgx.spec)

list(APPEND SOURCE
    netcfgx.c
    classfactory.c
    installer.c
    netcfg_iface.c
    netcfgbindinginterface_iface.c
    netcfgbindingpath_iface.c
    inetcfgcomp_iface.c
    tcpipconf_notify.c
    propertypage.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/netcfgx_stubs.c)

add_library(netcfgx MODULE
    ${SOURCE}
    netcfgx.rc
    ${CMAKE_CURRENT_BINARY_DIR}/netcfgx.def)

set_module_type(netcfgx win32dll UNICODE)
target_link_libraries(netcfgx uuid wine)
add_importlibs(netcfgx rpcrt4 setupapi advapi32 iphlpapi dhcpcsvc ole32 user32 comctl32 ws2_32 msvcrt kernel32 ntdll)
add_pch(netcfgx precomp.h SOURCE)
add_cd_file(TARGET netcfgx DESTINATION reactos/system32 FOR all)
