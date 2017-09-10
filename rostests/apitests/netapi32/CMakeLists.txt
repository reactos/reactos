
list(APPEND SOURCE
    DsRoleGetPrimaryDomainInformation.c
    testlist.c)

add_executable(netapi32_apitest ${SOURCE})
target_link_libraries(netapi32_apitest wine ${PSEH_LIB})
set_module_type(netapi32_apitest win32cui)
add_importlibs(netapi32_apitest netapi32 msvcrt kernel32 ntdll)
add_rostests_file(TARGET netapi32_apitest)
