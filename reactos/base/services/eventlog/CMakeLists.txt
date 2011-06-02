
include_directories(${REACTOS_BINARY_DIR}/include/reactos/idl)

set_unicode()

list(APPEND SOURCE
    eventlog.c
    eventsource.c
    logport.c
    eventlog.rc
    rpc.c
    file.c)

add_executable(eventlog ${SOURCE})

target_link_libraries(eventlog eventlogrpc ${PSEH_LIB})

add_pch(eventlog eventlog.h)

set_module_type(eventlog win32cui)
add_importlibs(eventlog advapi32 rpcrt4 msvcrt kernel32 ntdll)
add_cd_file(TARGET eventlog DESTINATION reactos/system32 FOR all)
