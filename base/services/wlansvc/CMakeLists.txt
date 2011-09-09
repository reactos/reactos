
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/idl)

add_rpc_files(server ${REACTOS_SOURCE_DIR}/include/reactos/idl/wlansvc.idl)


add_executable(wlansvc
    wlansvc.c
    rpcserver.c
    ${CMAKE_CURRENT_BINARY_DIR}/wlansvc_s.c)

if(NOT MSVC)
    target_link_libraries(wlansvc pseh)
endif()

set_module_type(wlansvc win32cui UNICODE)
add_importlibs(wlansvc advapi32 rpcrt4 iphlpapi msvcrt kernel32 ntdll)
add_cd_file(TARGET wlansvc DESTINATION reactos/system32 FOR all)
