
set_unicode()

include_directories(${REACTOS_BINARY_DIR}/include/reactos/idl)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys/lsass)

spec2def(lsasrv ${CMAKE_CURRENT_SOURCE_DIR}/lsasrv.spec ${CMAKE_CURRENT_BINARY_DIR}/lsasrv.def)

add_library(lsasrv SHARED
    authport.c
    lsarpc.c
    lsasrv.c
    privileges.c
    sids.c
    lsasrv.rc)

set_entrypoint(lsasrv 0)

target_link_libraries(lsasrv
    ${CMAKE_CURRENT_BINARY_DIR}/lsasrv.def
    lsa_server
    wine
    pseh)

add_importlibs(lsasrv rpcrt4 kernel32 ntdll)
add_dependencies(lsasrv lsasrv_def  psdk buildno_header)
add_livecd_target(lsasrv reactos/system32)