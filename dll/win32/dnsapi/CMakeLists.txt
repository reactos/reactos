
include_directories(
    include
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl
    ${REACTOS_SOURCE_DIR}/sdk/lib/dnsresolv)

add_rpc_files(client ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl/dnsrslvr.idl)

add_definitions(-D_CRT_NO_POSIX_ERROR_CODES)
spec2def(dnsapi.dll dnsapi.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    context.c
    memory.c
    names.c
    query.c
    record.c
    rpc.c
    stubs.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/dnsrslvr_c.c)

add_library(dnsapi MODULE
    ${SOURCE}
    dnsapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dnsapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dnsapi.def)

set_module_type(dnsapi win32dll)
target_link_libraries(dnsapi dnsresolv ${PSEH_LIB})
add_importlibs(dnsapi advapi32 rpcrt4 user32 ws2_32 iphlpapi msvcrt kernel32 ntdll)
add_pch(dnsapi precomp.h SOURCE)
add_cd_file(TARGET dnsapi DESTINATION reactos/system32 FOR all)
