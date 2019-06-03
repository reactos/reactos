
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/dll/win32/dhcpcsvc/include
    ${REACTOS_SOURCE_DIR}/sdk/lib/tdilib)

add_definitions(
    -DGetAdaptersAddressesV2
    -D_CRT_NO_POSIX_ERROR_CODES)

list(APPEND SOURCE
    address.c
    dhcp_reactos.c
    icmp.c
    ifenum_reactos.c
    ipstats_reactos.c
    iphlpapi_reactos.c
    iphlpapi_main.c
    media.c
    registry.c
    resinfo_reactos.c
    route_reactos.c
    iphlpapi_private.h)

add_library(iphlpapi MODULE
    ${SOURCE}
    iphlpapi.rc
    iphlpapi_fakestubs.c)
spec2def(iphlpapi iphlpapi.dll iphlpapi.spec ADD_IMPORTLIB)

set_module_type(iphlpapi win32dll UNICODE)
target_link_libraries(iphlpapi wine tdilib)
add_importlibs(iphlpapi dhcpcsvc advapi32 psapi ws2_32 msvcrt kernel32 ntdll)
add_pch(iphlpapi iphlpapi_private.h SOURCE)
add_cd_file(TARGET iphlpapi DESTINATION reactos/system32 FOR all)
