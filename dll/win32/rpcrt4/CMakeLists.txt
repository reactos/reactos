
list(APPEND SOURCE
    cproxy.c
    cpsf.c
    cstub.c
    ndr_clientserver.c
    ndr_contexthandle.c
    ndr_es.c
    ndr_fullpointer.c
    ndr_marshall.c
    ndr_ole.c
    ndr_stubless.c
    rpc_assoc.c
    rpc_async.c
    rpc_binding.c
    rpc_epmap.c
    rpc_message.c
    rpcrt4.rc
    rpcrt4_main.c
    rpc_server.c
    rpc_transport.c
    unix_func.c
    ${CMAKE_CURRENT_BINARY_DIR}/rpcrt4_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/rpcrt4.def)

if(MSVC)
    list(APPEND SOURCE msvc.S)
endif()

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D_RPCRT4_
    -DCOM_NO_WINDOWS_H
    -DMSWMSG)

spec2def(rpcrt4.dll rpcrt4.spec)

add_library(rpcrt4 SHARED ${SOURCE})

set_module_type(rpcrt4 win32dll)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/wine
    ${REACTOS_BINARY_DIR}/include/reactos/wine)

target_link_libraries(rpcrt4
    wine
    uuid
    epmrpc
    ${PSEH_LIB})

add_importlibs(rpcrt4 msvcrt user32 advapi32 secur32 iphlpapi ws2_32 kernel32 ntdll)
add_dependencies(rpcrt4 winesdk)
add_cd_file(TARGET rpcrt4 DESTINATION reactos/system32 FOR all)
add_importlib_target(rpcrt4.spec)
