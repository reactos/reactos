
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(spoolss.dll spoolss.spec)

add_library(spoolss SHARED spoolss_main.c router.c ${CMAKE_CURRENT_BINARY_DIR}/spoolss.def)

set_module_type(spoolss win32dll)

target_link_libraries(spoolss wine)

add_importlibs(spoolss winspool kernel32 ntdll)

add_cab_target(spoolss 1)
