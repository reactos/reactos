
spec2def(spoolss.dll spoolss.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    context.c
    jobs.c
    main.c
    memory.c
    monitors.c
    ports.c
    precomp.h
    printerdata.c
    printers.c
    printprocessors.c
    tools.c)

add_library(spoolss SHARED
    ${SOURCE}
    spoolss.rc
    ${CMAKE_CURRENT_BINARY_DIR}/spoolss_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/spoolss.def)

set_module_type(spoolss win32dll UNICODE)
target_link_libraries(spoolss wine)
add_importlibs(spoolss advapi32 msvcrt kernel32 ntdll)
add_pch(spoolss precomp.h SOURCE)
add_cd_file(TARGET spoolss DESTINATION reactos/system32 FOR all)
