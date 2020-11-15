
spec2def(spoolss.dll spoolss.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    ../marshalling.c
    context.c
    forms.c
    jobs.c
    main.c
    memory.c
    monitors.c
    ports.c
    printerdata.c
    printerdrivers.c
    printers.c
    printprocessors.c
    printproviders.c
    spoolfile.c
    tools.c)

list(APPEND PCH_SKIP_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/spoolss_stubs.c)

add_library(spoolss MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    spoolss.rc
    ${CMAKE_CURRENT_BINARY_DIR}/spoolss.def)

set_module_type(spoolss win32dll UNICODE)
target_link_libraries(spoolss wine)
add_importlibs(spoolss advapi32 msvcrt kernel32 ntdll)
add_pch(spoolss precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET spoolss DESTINATION reactos/system32 FOR all)
