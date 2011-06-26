
set_unicode()

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/dll/win32/dhcpcsvc/include
    ${REACTOS_SOURCE_DIR}/lib/tdilib)

spec2def(iphlpapi.dll iphlpapi.spec)

list(APPEND SOURCE
    dhcp_reactos.c
    ifenum_reactos.c
    ipstats_reactos.c
    iphlpapi_main.c
    media.c
    registry.c
    resinfo_reactos.c
    route_reactos.c
    iphlpapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/iphlpapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/iphlpapi.def)

add_library(iphlpapi SHARED ${SOURCE})

set_module_type(iphlpapi win32dll)

target_link_libraries(iphlpapi wine tdilib)
add_importlibs(iphlpapi dhcpcsvc advapi32 ws2_32 msvcrt kernel32 ntdll)
add_cd_file(TARGET iphlpapi DESTINATION reactos/system32 FOR all)
add_importlib_target(iphlpapi.spec)
