
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x500)

add_definitions(
    -D__ROS_LONG64__
    -D_DLL -D__USE_CRTIMP)

add_rpc_files(client server.idl)
add_rpc_files(server server.idl)

list(APPEND SOURCE
    cstub.c
    generated.c
    ndr_marshall.c
    rpc.c
    rpc_async.c
    rpc_protseq.c
    server.c
    testlist.c
    ${CMAKE_CURRENT_BINARY_DIR}/server_c.c
    ${CMAKE_CURRENT_BINARY_DIR}/server_s.c)

add_executable(rpcrt4_winetest ${SOURCE})
target_link_libraries(rpcrt4_winetest
    uuid
    wine
    ${PSEH_LIB})

set_module_type(rpcrt4_winetest win32cui)
add_importlibs(rpcrt4_winetest ole32 rpcrt4 msvcrt kernel32 ntdll)
add_cd_file(TARGET rpcrt4_winetest DESTINATION reactos/bin FOR all)
