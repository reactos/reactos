
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

spec2def(icmp ${CMAKE_CURRENT_SOURCE_DIR}/icmp.spec ${CMAKE_CURRENT_BINARY_DIR}/icmp.def)

add_library(icmp SHARED icmp_main.c icmp.rc)

set_module_type(icmp win32dll)

target_link_libraries(icmp
    ${CMAKE_CURRENT_BINARY_DIR}/icmp.def
    wine)

add_importlibs(icmp ws2_32 kernel32 ntdll)
add_dependencies(icmp icmp_def psdk)
add_cab_target(icmp 1)
