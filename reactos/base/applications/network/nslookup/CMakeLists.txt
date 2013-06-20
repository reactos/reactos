
add_executable(nslookup
    nslookup.c
    utility.c
    nslookup.rc)

add_pch(nslookup nslookup.h)
set_module_type(nslookup win32cui)
add_importlibs(nslookup user32 ws2_32 snmpapi iphlpapi msvcrt kernel32)
add_cd_file(TARGET nslookup DESTINATION reactos/system32 FOR all)
