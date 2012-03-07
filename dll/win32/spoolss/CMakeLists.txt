
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(spoolss.dll spoolss.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    spoolss_main.c
    router.c
    ${CMAKE_CURRENT_BINARY_DIR}/spoolss_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/spoolss.def)

add_library(spoolss SHARED ${SOURCE})
allow_warnings(spoolss)
set_module_type(spoolss win32dll)
target_link_libraries(spoolss wine)
add_importlibs(spoolss winspool msvcrt kernel32 ntdll)

add_cd_file(TARGET spoolss DESTINATION reactos/system32 FOR all)
