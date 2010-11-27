
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
add_definitions(-D__WINESRC__)

spec2def(pidgen.dll pidgen.spec)

add_library(pidgen SHARED main.c rsrc.rc ${CMAKE_CURRENT_BINARY_DIR}/pidgen.def)

set_module_type(pidgen win32dll)

target_link_libraries(pidgen wine)

add_importlibs(pidgen msvcrt kernel32 ntdll)

add_cab_target(pidgen 1)
