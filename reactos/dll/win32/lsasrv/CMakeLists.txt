
set_unicode()

include_directories(
    ${REACTOS_BINARY_DIR}/include/reactos/idl
    ${REACTOS_SOURCE_DIR}/include/reactos/subsys/lsass)

set_rc_compiler()

spec2def(lsasrv.dll lsasrv.spec ADD_IMPORTLIB GENERATE_STUBS)

list(APPEND SOURCE
    authport.c
    lsarpc.c
    lsasrv.c
    privileges.c
    sids.c
    lsasrv.rc
    ${CMAKE_CURRENT_BINARY_DIR}/lsasrv_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/lsasrv.def)

add_library(lsasrv SHARED ${SOURCE})

set_entrypoint(lsasrv 0)

target_link_libraries(lsasrv
    lsarpc
    wine
    ${PSEH_LIB})

add_importlibs(lsasrv rpcrt4 msvcrt kernel32 ntdll)
add_dependencies(lsasrv psdk)
add_cd_file(TARGET lsasrv DESTINATION reactos/system32 FOR all)
add_importlib_target(lsasrv.spec)
