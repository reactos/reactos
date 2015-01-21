
include_directories(../../apitests/include)

list(APPEND SOURCE
    InterfaceInfo.c
    tcp_info.c
    testlist.c)

add_executable(tcpip_drvtest ${SOURCE})
target_link_libraries(tcpip_drvtest wine)
set_module_type(tcpip_drvtest win32cui)
add_importlibs(tcpip_drvtest msvcrt kernel32 ntdll)
add_cd_file(TARGET tcpip_drvtest DESTINATION reactos/bin FOR all)
