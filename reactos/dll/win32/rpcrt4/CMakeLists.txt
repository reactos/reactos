
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
    ${CMAKE_CURRENT_BINARY_DIR}/epm_c.c
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

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

add_library(rpcrt4 SHARED
    ${SOURCE}
    rpcrt4.rc)

set_module_type(rpcrt4 win32dll)
target_link_libraries(rpcrt4 wine uuid ${PSEH_LIB})

add_delay_importlibs(rpcrt4 iphlpapi wininet secur32 user32)
add_importlibs(rpcrt4 msvcrt advapi32 ws2_32 kernel32 ntdll)
add_cd_file(TARGET rpcrt4 DESTINATION reactos/system32 FOR all)


if(NOT MSVC)
    # FIXME: http://www.cmake.org/Bug/view.php?id=12998
    #allow_warnings(rpcrt4)
    set_source_files_properties(${SOURCE} PROPERTIES COMPILE_FLAGS "-Wno-error")
endif()
