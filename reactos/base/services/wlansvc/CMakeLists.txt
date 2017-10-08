
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl)
add_rpc_files(server ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl/wlansvc.idl)

list(APPEND SOURCE
    wlansvc.c
    rpcserver.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/wlansvc_s.c)

add_executable(wlansvc ${SOURCE} wlansvc.rc)

if(NOT MSVC)
    target_link_libraries(wlansvc ${PSEH_LIB})
endif()

set_module_type(wlansvc win32cui UNICODE)
add_importlibs(wlansvc advapi32 rpcrt4 iphlpapi msvcrt kernel32 ntdll)
add_pch(wlansvc precomp.h SOURCE)
add_cd_file(TARGET wlansvc DESTINATION reactos/system32 FOR all)
