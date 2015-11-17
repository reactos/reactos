
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/include/reactos/wine)
spec2def(rpcrt4.dll rpcrt4.spec ADD_IMPORTLIB)
add_rpc_files(client epm.idl)

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
    rpcrt4_main.c
    rpc_server.c
    rpc_transport.c
    unix_func.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/epm_c.c)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -D_RPCRT4_
    -DCOM_NO_WINDOWS_H
    -DMSWMSG)

if(MSVC AND NOT ARCH STREQUAL "arm")
    add_asm_files(rpcrt4_asm msvc.S)
endif()

add_library(rpcrt4 SHARED
    ${SOURCE}
    ${rpcrt4_asm}
    rpcrt4.rc
    ${CMAKE_CURRENT_BINARY_DIR}/rpcrt4_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/rpcrt4.def)

set_module_type(rpcrt4 win32dll)
target_link_libraries(rpcrt4 wine uuid ${PSEH_LIB})
add_delay_importlibs(rpcrt4 iphlpapi wininet secur32 user32)
add_importlibs(rpcrt4 advapi32 advapi32_vista ws2_32 msvcrt kernel32 ntdll)
add_pch(rpcrt4 precomp.h SOURCE)
add_cd_file(TARGET rpcrt4 DESTINATION reactos/system32 FOR all)
