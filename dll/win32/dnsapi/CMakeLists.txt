
include_directories(
    include
    ${REACTOS_SOURCE_DIR}/lib/3rdparty/adns/src
    ${REACTOS_SOURCE_DIR}/lib/3rdparty/adns/adns_win32)

add_definitions(-DADNS_JGAA_WIN32)

spec2def(dnsapi.dll dnsapi.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    dnsapi/adns.c
    dnsapi/context.c
    dnsapi/memory.c
    dnsapi/names.c
    dnsapi/query.c
    dnsapi/record.c
    dnsapi/stubs.c
    dnsapi/precomp.h)

add_library(dnsapi SHARED
    ${SOURCE}
    dnsapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/dnsapi.def)

set_module_type(dnsapi win32dll)
target_link_libraries(dnsapi adns)
add_importlibs(dnsapi user32 ws2_32 iphlpapi msvcrt kernel32 ntdll)
add_pch(dnsapi dnsapi/precomp.h SOURCE)
add_dependencies(dnsapi psdk)
add_cd_file(TARGET dnsapi DESTINATION reactos/system32 FOR all)

