
add_rpc_files(client epm.idl)

# HACK!!!!!!
if(MSVC)
    set(CMAKE_C_COMPILE_OBJECT "<CMAKE_C_COMPILER> /nologo <DEFINES> /Fo<OBJECT> /Fd<TARGET_PDB> -c <SOURCE> <FLAGS>")
    set_source_files_properties(rpc_transport.c PROPERTIES COMPILE_FLAGS "/link /DELAYLOAD:wininet.dll")
endif()

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

spec2def(rpcrt4.dll rpcrt4.spec)

add_library(rpcrt4 SHARED ${SOURCE})

set_module_type(rpcrt4 win32dll)

target_link_libraries(rpcrt4
    wine
    uuid
    ${PSEH_LIB})
    
if(NOT MSVC)
    add_delay_importlibs(rpcrt4 wininet)
else()
    add_importlibs(rpcrt4 wininet)
    add_linkerflag(rpcrt4 "/DELAYLOAD:wininet.dll")
    target_link_libraries(rpcrt4 delayimp)
endif()
    

add_importlibs(rpcrt4 msvcrt user32 advapi32 secur32 iphlpapi ws2_32 kernel32 ntdll)
add_cd_file(TARGET rpcrt4 DESTINATION reactos/system32 FOR all)
add_importlib_target(rpcrt4.spec)
