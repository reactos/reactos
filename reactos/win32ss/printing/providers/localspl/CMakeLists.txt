
spec2def(localspl.dll localspl.spec ADD_IMPORTLIB)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/skiplist)

list(APPEND SOURCE
    jobs.c
    main.c
    monitors.c
    ports.c
    precomp.h
    printerdata.c
    printers.c
    printingthread.c
    printprocessors.c
    tools.c)

add_library(localspl SHARED
    ${SOURCE}
    localspl.rc
    ${CMAKE_CURRENT_BINARY_DIR}/localspl_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/localspl.def)

set_module_type(localspl win32dll UNICODE)
target_link_libraries(localspl skiplist16 wine)
add_importlibs(localspl advapi32 netapi32 rpcrt4 secur32 spoolss msvcrt kernel32 ntdll)
add_pch(localspl precomp.h SOURCE)
add_cd_file(TARGET localspl DESTINATION reactos/system32 FOR all)
