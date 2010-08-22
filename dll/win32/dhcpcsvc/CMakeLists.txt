
set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> -o <TARGET> <OBJECTS> <LINK_LIBRARIES> -L${REACTOS_BINARY_DIR}/lib/3rdparty/mingw")

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

set_target_properties(dhcpcsvc PROPERTIES LINK_FLAGS "-Wl,-entry,_DllMain@12")

target_link_libraries(dhcpcsvc
    ${CMAKE_CURRENT_BINARY_DIR}/dhcpcsvc.def
    ${REACTOS_SOURCE_DIR}/dll/win32/msvcrt/libmsvcrt.a
    -lws2_32
    -liphlpapi
    ${REACTOS_SOURCE_DIR}/dll/win32/advapi32/libadvapi32.a
    oldnames
    ${REACTOS_SOURCE_DIR}/dll/win32/kernel32/libkernel32.a
    ${REACTOS_SOURCE_DIR}/dll/ntdll/libntdll.a)

add_pch(dhcpcsvc ${CMAKE_CURRENT_SOURCE_DIR}/include/rosdhcp.h ${SOURCE})
add_dependencies(dhcpcsvc dhcpcsvc_def psdk)
