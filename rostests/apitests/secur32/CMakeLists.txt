
list(APPEND SOURCE
    client_server.c
#    client1.c
    client2_msdn.c
#    server1.c
    server2_msdn.c
    utils.c
    testlist.c)

add_executable(secur32_apitest ${SOURCE})
set_module_type(secur32_apitest win32cui)
add_importlibs(secur32_apitest advapi32 secur32 ws2_32 msvcrt kernel32)
add_rostests_file(TARGET secur32_apitest)
