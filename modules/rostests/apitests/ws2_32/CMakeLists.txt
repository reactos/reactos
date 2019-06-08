
list(APPEND SOURCE
    bind.c
    close.c
    getaddrinfo.c
    gethostname.c
    getnameinfo.c
    getservbyname.c
    getservbyport.c
    helpers.c
    ioctlsocket.c
    nonblocking.c
    nostartup.c
    open_osfhandle.c
    recv.c
    send.c
    WSAAsync.c
    WSAIoctl.c
    WSARecv.c
    WSAStartup.c
    ws2_32.h)

add_executable(ws2_32_apitest ${SOURCE} testlist.c)
target_link_libraries(ws2_32_apitest wine ${PSEH_LIB})
set_module_type(ws2_32_apitest win32cui)
add_importlibs(ws2_32_apitest advapi32 iphlpapi ws2_32 msvcrt kernel32 ntdll)
add_pch(ws2_32_apitest ws2_32.h SOURCE)
add_rostests_file(TARGET ws2_32_apitest)
