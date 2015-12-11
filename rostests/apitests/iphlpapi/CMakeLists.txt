
list(APPEND SOURCE
    icmp.c
    SendARP.c
    testlist.c)

add_executable(iphlpapi_apitest ${SOURCE})
target_link_libraries(iphlpapi_apitest wine ${PSEH_LIB})
set_module_type(iphlpapi_apitest win32cui)
add_importlibs(iphlpapi_apitest iphlpapi msvcrt kernel32 ntdll)
add_cd_file(TARGET iphlpapi_apitest DESTINATION reactos/bin FOR all)
