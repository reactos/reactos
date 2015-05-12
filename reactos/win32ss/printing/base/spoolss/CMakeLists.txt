
spec2def(spoolss spoolss.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    context.c
    main.c
    precomp.h)

add_library(spoolss SHARED
    ${SOURCE}
    spoolss.rc
    ${CMAKE_CURRENT_BINARY_DIR}/spoolss_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/spoolss.def)

set_module_type(spoolss win32dll UNICODE)
target_link_libraries(spoolss wine)
add_importlibs(spoolss msvcrt kernel32 advapi32 ntdll)
add_pch(spoolss precomp.h SOURCE)
add_cd_file(TARGET spoolss DESTINATION reactos/system32 FOR all)
