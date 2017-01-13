
list(APPEND SOURCE
    DnsQuery.c
    testlist.c)

add_executable(dnsapi_apitest ${SOURCE})
target_link_libraries(dnsapi_apitest wine)
set_module_type(dnsapi_apitest win32cui)
add_importlibs(dnsapi_apitest ws2_32 dnsapi iphlpapi msvcrt kernel32 ntdll)
add_rostests_file(TARGET dnsapi_apitest)
