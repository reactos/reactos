
list(APPEND SOURCE
    getaddrinfo.c
    helpers.c
    ioctlsocket.c
    nostartup.c
    recv.c
    send.c
    WSAStartup.c
    testlist.c)

add_executable(ws2_32_apitest ${SOURCE})
target_link_libraries(ws2_32_apitest wine ${PSEH_LIB})
set_module_type(ws2_32_apitest win32cui)
add_importlibs(ws2_32_apitest ws2_32 msvcrt kernel32 ntdll)
add_cd_file(TARGET ws2_32_apitest DESTINATION reactos/bin FOR all)
