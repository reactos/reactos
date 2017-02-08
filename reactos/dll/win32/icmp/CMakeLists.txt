
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(icmp.dll icmp.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    icmp_main.c
    icmp.rc
    ${CMAKE_CURRENT_BINARY_DIR}/icmp_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/icmp.def)

add_library(icmp SHARED ${SOURCE})
set_module_type(icmp win32dll)
target_link_libraries(icmp wine)
add_importlibs(icmp ws2_32 msvcrt kernel32 ntdll)
add_cd_file(TARGET icmp DESTINATION reactos/system32 FOR all)
