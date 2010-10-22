
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(spoolss ${CMAKE_CURRENT_SOURCE_DIR}/spoolss.spec ${CMAKE_CURRENT_BINARY_DIR}/spoolss.def)

add_library(spoolss SHARED spoolss_main.c router.c)

set_module_type(spoolss win32dll)

target_link_libraries(spoolss
    ${CMAKE_CURRENT_BINARY_DIR}/spoolss.def
    wine)

add_importlibs(spoolss winspool kernel32 ntdll)
add_dependencies(spoolss spoolss_def)
add_livecd_target(spoolss reactos/system32)
