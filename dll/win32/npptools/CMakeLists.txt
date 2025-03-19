
spec2def(npptools.dll npptools.spec)

add_library(npptools MODULE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/npptools_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/npptools.def)

set_module_type(npptools win32dll)
add_importlibs(npptools msvcrt kernel32 ntdll)
add_cd_file(TARGET npptools DESTINATION reactos/system32 FOR all)
set_wine_module(npptools)
