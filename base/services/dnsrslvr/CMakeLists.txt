
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl)
add_rpc_files(server ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl/dnsrslvr.idl)

list(APPEND SOURCE
    cache.c
    dnsrslvr.c
    hostsfile.c
    rpcserver.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/dnsrslvr_s.c)

spec2def(dnsrslvr.dll dnsrslvr.spec ADD_IMPORTLIB)

add_library(dnsrslvr SHARED ${SOURCE} dnsrslvr.rc ${CMAKE_CURRENT_BINARY_DIR}/dnsrslvr.def)

if(NOT MSVC)
    target_link_libraries(dnsrslvr ${PSEH_LIB})
endif()

set_module_type(dnsrslvr win32dll UNICODE)
add_importlibs(dnsrslvr advapi32 rpcrt4 dnsapi iphlpapi msvcrt kernel32 ntdll)
add_pch(dnsrslvr precomp.h SOURCE)
add_cd_file(TARGET dnsrslvr DESTINATION reactos/system32 FOR all)
