
include_directories(${REACTOS_BINARY_DIR}/include/reactos/idl)

set_unicode()

list(APPEND SOURCE
    audsrv.c
    audsrv.rc
    rpc.c)

add_executable(audsrv ${SOURCE})

target_link_libraries(audsrv audsrvrpc ${PSEH_LIB})

add_pch(audsrv ${CMAKE_CURRENT_SOURCE_DIR}/audsrv.h ${SOURCE})

set_module_type(audsrv win32cui)
add_importlibs(audsrv advapi32 rpcrt4 msvcrt kernel32 ntdll user32 ksuser setupapi)
add_cab_target(audsrv 1)
