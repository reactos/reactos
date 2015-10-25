
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${REACTOS_SOURCE_DIR}/include/reactos/idl
    ${REACTOS_SOURCE_DIR}/include/reactos/subsys/lsass)

add_rpc_files(server
    ${REACTOS_SOURCE_DIR}/include/reactos/idl/dssetup.idl
    ${REACTOS_SOURCE_DIR}/include/reactos/idl/lsa.idl)
spec2def(lsasrv.dll lsasrv.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    authpackage.c
    authport.c
    database.c
    dssetup.c
    lookup.c
    lsarpc.c
    lsasrv.c
    policy.c
    privileges.c
    registry.c
    security.c
    session.c
    srm.c
    utils.c
    lsasrv.h
    ${CMAKE_CURRENT_BINARY_DIR}/dssetup_s.c
    ${CMAKE_CURRENT_BINARY_DIR}/lsa_s.c
    ${CMAKE_CURRENT_BINARY_DIR}/lsasrv_stubs.c)

add_library(lsasrv SHARED
    ${SOURCE}
    lsasrv.rc
    ${CMAKE_CURRENT_BINARY_DIR}/lsasrv.def)

set_module_type(lsasrv win32dll UNICODE)
target_link_libraries(lsasrv wine ${PSEH_LIB})
add_importlibs(lsasrv samsrv rpcrt4 msvcrt kernel32 advapi32 ntdll)
add_pch(lsasrv lsasrv.h SOURCE)
add_cd_file(TARGET lsasrv DESTINATION reactos/system32 FOR all)
