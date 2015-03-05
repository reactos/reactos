
include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/subsys
    ${REACTOS_SOURCE_DIR}/include/reactos/idl
    ${CMAKE_CURRENT_BINARY_DIR})

add_rpc_files(server ${REACTOS_SOURCE_DIR}/include/reactos/idl/svcctl.idl)

list(APPEND SOURCE
    config.c
    controlset.c
    database.c
    driver.c
    groupdb.c
    lock.c
    rpcserver.c
    services.c
    services.h
    ${CMAKE_CURRENT_BINARY_DIR}/svcctl_s.c)

add_executable(services ${SOURCE} services.rc)

if(NOT MSVC)
    target_link_libraries(services ${PSEH_LIB})
endif()

set_module_type(services win32gui UNICODE)
add_importlibs(services user32 advapi32 rpcrt4 msvcrt kernel32 ntdll)
add_pch(services services.h SOURCE)
add_cd_file(TARGET services DESTINATION reactos/system32 FOR all)
