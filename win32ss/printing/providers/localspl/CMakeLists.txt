remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

spec2def(localspl.dll localspl.spec ADD_IMPORTLIB)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/skiplist)

list(APPEND SOURCE
    forms.c
    jobs.c
    main.c
    monitors.c
    ports.c
    precomp.h
    printerdata.c
    printerdrivers.c
    printers.c
    printingthread.c
    printprocessors.c
    spoolfile.c
    tools.c
    xcv.c)

add_library(localspl MODULE
    ${SOURCE}
    localspl.rc
    ${CMAKE_CURRENT_BINARY_DIR}/localspl_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/localspl.def)

set_module_type(localspl win32dll UNICODE)
target_link_libraries(localspl skiplist16 wine)
add_importlibs(localspl user32 advapi32 advapi32_vista netapi32 rpcrt4 secur32 spoolss msvcrt kernel32 ntdll)
add_pch(localspl precomp.h SOURCE)
add_cd_file(TARGET localspl DESTINATION reactos/system32 FOR all)
