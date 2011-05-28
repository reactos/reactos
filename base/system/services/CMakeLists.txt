
set_unicode()

include_directories(
    ${REACTOS_BINARY_DIR}/include/reactos/idl
    ${REACTOS_SOURCE_DIR}/include/reactos/subsys)

list(APPEND SOURCE
    config.c
    database.c
    driver.c
    groupdb.c
    rpcserver.c
    services.c
    services.rc)

add_executable(services ${CMAKE_CURRENT_BINARY_DIR}/services_services.h.gch ${SOURCE})

target_link_libraries(services
    svcctlrpc
    ${PSEH_LIB})

add_pch(services ${CMAKE_CURRENT_SOURCE_DIR}/services.h ${SOURCE})

set_module_type(services win32cui)
add_importlibs(services user32 advapi32 rpcrt4 msvcrt kernel32 ntdll)
add_cd_file(TARGET services DESTINATION reactos/system32 FOR all)
