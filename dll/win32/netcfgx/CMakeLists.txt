
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

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
    precomp.h)

add_library(netcfgx MODULE
    ${SOURCE}
    netcfgx.rc)
spec2def(netcfgx netcfgx.dll netcfgx.spec)

set_module_type(netcfgx win32dll UNICODE)
target_link_libraries(netcfgx uuid wine)
add_importlibs(netcfgx rpcrt4 setupapi advapi32 iphlpapi dhcpcsvc ole32 user32 comctl32 ws2_32 msvcrt kernel32 ntdll)
add_pch(netcfgx precomp.h SOURCE)
add_cd_file(TARGET netcfgx DESTINATION reactos/system32 FOR all)
