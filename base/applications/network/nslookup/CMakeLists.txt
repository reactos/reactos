
add_executable(nslookup
    nslookup.c
    utility.c
    nslookup.rc)

set_module_type(nslookup win32cui)

add_importlibs(nslookup user32 ws2_32 snmpapi iphlpapi msvcrt kernel32)
