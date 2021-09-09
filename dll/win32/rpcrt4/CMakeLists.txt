
remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

add_definitions(
    -D__WINESRC__
    -D_RPCRT4_
    -DCOM_NO_WINDOWS_H
    -DMSWMSG)

include_directories(BEFORE ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(rpcrt4.dll rpcrt4.spec ADD_IMPORTLIB)

add_rpc_files(client epm.idl)
add_idl_headers(ndr_types_header ndr_types.idl)

set(OLD_IDL_FLAGS ${IDL_FLAGS})
set(IDL_FLAGS ${IDL_FLAGS} -Oicf)
add_rpcproxy_files(ndr_types.idl)
set(IDL_FLAGS ${OLD_IDL_FLAGS})

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
    ${CMAKE_CURRENT_BINARY_DIR}/epm_c.c)

if(MSVC)
    add_asm_files(rpcrt4_asm msvc.S)
endif()

list(APPEND PCH_SKIP_SOURCE
    ndr_typelib.c
    ${CMAKE_CURRENT_BINARY_DIR}/ndr_types_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
    ${CMAKE_CURRENT_BINARY_DIR}/rpcrt4_stubs.c)

add_library(rpcrt4 MODULE
    ${SOURCE}
    ${rpcrt4_asm}
    ${PCH_SKIP_SOURCE}
    rpcrt4.rc
    ${CMAKE_CURRENT_BINARY_DIR}/rpcrt4.def)

if(MSVC)
    # Disable warning C4267: '=': conversion from 'size_t' to 'short', possible loss of data
    target_compile_options(rpcrt4 PRIVATE /wd4267)
endif()

set_module_type(rpcrt4 win32dll)
target_link_libraries(rpcrt4 wine uuid ${PSEH_LIB})
add_delay_importlibs(rpcrt4 iphlpapi wininet secur32 user32 oleaut32)
add_importlibs(rpcrt4 advapi32 advapi32_vista kernel32_vista ws2_32 msvcrt kernel32 ntdll)
add_dependencies(rpcrt4 ndr_types_header)
add_pch(rpcrt4 precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET rpcrt4 DESTINATION reactos/system32 FOR all)
