
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${REACTOS_SOURCE_DIR}/include/reactos/idl
    ${REACTOS_SOURCE_DIR}/include/reactos/subsys/lsass)

add_rpc_files(server ${REACTOS_SOURCE_DIR}/include/reactos/idl/lsa.idl)

set_rc_compiler()

spec2def(lsasrv.dll lsasrv.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    authport.c
    database.c
    lsarpc.c
    lsasrv.c
    policy.c
    privileges.c
    sids.c
    lsasrv.rc
    ${CMAKE_CURRENT_BINARY_DIR}/lsasrv_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/lsasrv.def
    ${CMAKE_CURRENT_BINARY_DIR}/lsa_s.c)

add_library(lsasrv SHARED ${SOURCE})
set_module_type(lsasrv win32dll ENTRYPOINT 0 UNICODE)

target_link_libraries(lsasrv wine ${PSEH_LIB})
add_importlibs(lsasrv rpcrt4 msvcrt kernel32 ntdll)
add_pch(lsasrv lsasrv.h)
add_dependencies(lsasrv psdk)
add_cd_file(TARGET lsasrv DESTINATION reactos/system32 FOR all)

