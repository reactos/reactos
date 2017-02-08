
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(spoolss.dll spoolss.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    spoolss_main.c
    router.c
    spoolss.h
    ${CMAKE_CURRENT_BINARY_DIR}/spoolss_stubs.c)

add_library(spoolss SHARED
    ${SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/spoolss.def)

set_module_type(spoolss win32dll)
target_link_libraries(spoolss wine)
add_importlibs(spoolss msvcrt kernel32 ntdll)
add_pch(spoolss spoolss.h SOURCE)
add_cd_file(TARGET spoolss DESTINATION reactos/system32 FOR all)
