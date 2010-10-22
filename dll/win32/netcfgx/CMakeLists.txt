
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(netcfgx ${CMAKE_CURRENT_SOURCE_DIR}/netcfgx.spec ${CMAKE_CURRENT_BINARY_DIR}/netcfgx.def)

add_library(netcfgx SHARED
    netcfgx.c
    classfactory.c
    netcfg_iface.c
    inetcfgcomp_iface.c
    tcpipconf_notify.c
    netcfgx.rc)

set_module_type(netcfgx win32dll)

target_link_libraries(netcfgx
    ${CMAKE_CURRENT_BINARY_DIR}/netcfgx.def
    uuid
    wine)

add_importlibs(netcfgx rpcrt4 setupapi advapi32 iphlpapi ole32 user32 comctl32 ws2_32 kernel32 ntdll)
add_dependencies(netcfgx netcfgx_def)
add_livecd_target(netcfgx reactos/system32)
