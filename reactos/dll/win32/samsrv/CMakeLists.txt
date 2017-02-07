
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${REACTOS_SOURCE_DIR}/include/reactos/idl)

add_rpc_files(server ${REACTOS_SOURCE_DIR}/include/reactos/idl/sam.idl)

spec2def(samsrv.dll samsrv.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    database.c
    registry.c
    samrpc.c
    samsrv.c
    setup.c
    samsrv.rc
    ${CMAKE_CURRENT_BINARY_DIR}/samsrv_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/samsrv.def
    ${CMAKE_CURRENT_BINARY_DIR}/sam_s.c)

add_library(samsrv SHARED ${SOURCE})
set_module_type(samsrv win32dll ENTRYPOINT 0 UNICODE)

target_link_libraries(samsrv wine ${PSEH_LIB})
add_importlibs(samsrv rpcrt4 advapi32 kernel32 ntdll)
add_dependencies(samsrv psdk)
add_cd_file(TARGET samsrv DESTINATION reactos/system32 FOR all)
