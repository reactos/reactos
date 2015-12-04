
spec2def(icmp.dll icmp.spec)

list(APPEND SOURCE
    icmp.rc
    ${CMAKE_CURRENT_BINARY_DIR}/icmp_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/icmp.def)

add_library(icmp SHARED ${SOURCE})
set_module_type(icmp win32dll)
add_importlibs(icmp iphlpapi msvcrt kernel32 ntdll)
add_cd_file(TARGET icmp DESTINATION reactos/system32 FOR all)
