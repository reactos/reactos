
add_definitions(-Dtzset=_tzset)

include_directories(include)

spec2def(dhcpcsvc ${CMAKE_CURRENT_SOURCE_DIR}/dhcpcsvc.spec ${CMAKE_CURRENT_BINARY_DIR}/dhcpcsvc.def)

list(APPEND SOURCE 
    dhcp/adapter.c
    dhcp/alloc.c
    dhcp/api.c
    dhcp/compat.c
    dhcp/dhclient.c
    dhcp/dispatch.c
    dhcp/hash.c
    dhcp/options.c
    dhcp/pipe.c
    dhcp/socket.c
    dhcp/tables.c
    dhcp/util.c
    dhcpcsvc.c
    dhcpcsvc.rc)

add_library(dhcpcsvc SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/dhcpcsvc_rosdhcp.h.gch
    ${SOURCE})

set_module_type(dhcpcsvc win32dll)

target_link_libraries(dhcpcsvc ${CMAKE_CURRENT_BINARY_DIR}/dhcpcsvc.def)

add_importlibs(dhcpcsvc ws2_32 iphlpapi advapi32 msvcrt kernel32 ntdll)
add_pch(dhcpcsvc ${CMAKE_CURRENT_SOURCE_DIR}/include/rosdhcp.h ${SOURCE})
add_dependencies(dhcpcsvc dhcpcsvc_def)
