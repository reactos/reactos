
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/idl)
add_rpc_files(server ${REACTOS_SOURCE_DIR}/include/reactos/idl/winspool.idl)

list(APPEND SOURCE
    init.c
    jobs.c
    main.c
    printers.c
    printprocessors.c
    precomp.h
    rpcserver.c
    rpcstubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/winspool_s.c)

add_executable(spoolsv ${SOURCE} spoolsv.rc)
set_module_type(spoolsv win32cui UNICODE)
target_link_libraries(spoolsv wine)
add_delay_importlibs(spoolsv spoolss)
add_importlibs(spoolsv advapi32 rpcrt4 msvcrt kernel32 ntdll)
add_pch(spoolsv precomp.h SOURCE)
add_cd_file(TARGET spoolsv DESTINATION reactos/system32 FOR all)
