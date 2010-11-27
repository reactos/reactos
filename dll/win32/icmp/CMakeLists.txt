
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(icmp.dll icmp.spec)

add_library(icmp SHARED icmp_main.c icmp.rc ${CMAKE_CURRENT_BINARY_DIR}/icmp.def)

set_module_type(icmp win32dll)

target_link_libraries(icmp wine)

add_importlibs(icmp ws2_32 msvcrt kernel32 ntdll)
add_dependencies(icmp psdk)
add_cab_target(icmp 1)
