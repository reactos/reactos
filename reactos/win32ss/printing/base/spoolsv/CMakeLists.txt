
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/idl)
add_rpc_files(server ${REACTOS_SOURCE_DIR}/include/reactos/idl/winspool.idl)

list(APPEND SOURCE
    init.c
    main.c
    rpcserver.c
    rpcstubs.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/winspool_s.c)

add_executable(spoolsv ${SOURCE} spoolsv.rc)
set_module_type(spoolsv win32cui UNICODE)
target_link_libraries(spoolsv wine)
add_delay_importlibs(spoolsv spoolss)
add_importlibs(spoolsv msvcrt kernel32 advapi32 ntdll rpcrt4)
add_pch(spoolsv precomp.h SOURCE)
add_cd_file(TARGET spoolsv DESTINATION reactos/system32 FOR all)
