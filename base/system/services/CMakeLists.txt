
include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/subsys
    ${REACTOS_SOURCE_DIR}/include/reactos/idl
    ${CMAKE_CURRENT_BINARY_DIR})

add_rpc_files(server ${REACTOS_SOURCE_DIR}/include/reactos/idl/svcctl.idl)


list(APPEND SOURCE
    config.c
    database.c
    driver.c
    groupdb.c
    rpcserver.c
    services.c
    services.rc
    ${CMAKE_CURRENT_BINARY_DIR}/svcctl_s.c)

add_executable(services ${SOURCE})

if(NOT MSVC)
    target_link_libraries(services pseh)
endif()

set_module_type(services win32cui UNICODE)
add_importlibs(services user32 advapi32 rpcrt4 msvcrt kernel32 ntdll)
add_pch(services services.h ${CMAKE_CURRENT_BINARY_DIR}/svcctl_s.h)
add_cd_file(TARGET services DESTINATION reactos/system32 FOR all)
