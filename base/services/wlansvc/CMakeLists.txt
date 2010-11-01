
set_unicode()
include_directories(${REACTOS_BINARY_DIR}/include/reactos/idl)

add_executable(wlansvc wlansvc.c rpcserver.c)

target_link_libraries(wlansvc wlansvc_server ${PSEH_LIB})

set_module_type(wlansvc win32cui)
add_importlibs(wlansvc
    advapi32
    rpcrt4
    iphlpapi
    msvcrt
    kernel32
    ntdll)
