
set_unicode()

add_definitions(-D_DLL -D__USE_CRTIMP)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/dll/win32/dhcpcsvc/include
    ${REACTOS_SOURCE_DIR}/lib/tdilib)

spec2def(iphlpapi ${CMAKE_CURRENT_SOURCE_DIR}/iphlpapi.spec ${CMAKE_CURRENT_BINARY_DIR}/iphlpapi.def)

add_library(iphlpapi SHARED 
    dhcp_reactos.c
    ifenum_reactos.c
    ipstats_reactos.c
    iphlpapi_main.c
    media.c
    registry.c
    resinfo_reactos.c
    route_reactos.c
    iphlpapi.rc)

set_module_type(iphlpapi win32dll)

target_link_libraries(iphlpapi
    ${CMAKE_CURRENT_BINARY_DIR}/iphlpapi.def
    wine
    tdilib)

add_importlibs(iphlpapi dhcpcsvc advapi32 ws2_32 msvcrt kernel32 ntdll)
add_dependencies(iphlpapi iphlpapi_def)
add_livecd_target(iphlpapi reactos/system32)
