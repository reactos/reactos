
set_unicode()

include_directories(${REACTOS_BINARY_DIR}/include/reactos/idl)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/subsys/lsass)

spec2def(lsasrv.dll lsasrv.spec)

add_library(lsasrv SHARED
    authport.c
    lsarpc.c
    lsasrv.c
    privileges.c
    sids.c
    lsasrv.rc
    ${CMAKE_CURRENT_BINARY_DIR}/lsasrv.def)

set_entrypoint(lsasrv 0)

target_link_libraries(lsasrv
    lsa_server
    wine
    ${PSEH_LIB})

add_importlibs(lsasrv rpcrt4 kernel32 ntdll)
add_dependencies(lsasrv  psdk buildno_header)
add_cab_target(lsasrv 1)