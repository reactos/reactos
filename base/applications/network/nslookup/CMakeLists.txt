
list(APPEND SOURCE
    nslookup.c
    utility.c
    nslookup.h)

add_executable(nslookup ${SOURCE} nslookup.rc)
set_module_type(nslookup win32cui)
add_importlibs(nslookup user32 ws2_32 snmpapi iphlpapi msvcrt kernel32)
add_pch(nslookup nslookup.h SOURCE)
add_cd_file(TARGET nslookup DESTINATION reactos/system32 FOR all)
