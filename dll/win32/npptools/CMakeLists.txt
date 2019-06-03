
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

add_library(npptools MODULE
    main.c)
spec2def(npptools npptools.dll npptools.spec)

set_module_type(npptools win32dll)
add_importlibs(npptools msvcrt kernel32 ntdll)
add_cd_file(TARGET npptools DESTINATION reactos/system32 FOR all)
