
list(APPEND SOURCE
    icmp.rc)

add_library(icmp MODULE ${SOURCE})
spec2def(icmp icmp.dll icmp.spec)

set_module_type(icmp win32dll)
add_importlibs(icmp iphlpapi msvcrt kernel32 ntdll)
add_cd_file(TARGET icmp DESTINATION reactos/system32 FOR all)
