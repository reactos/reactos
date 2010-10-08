
include_directories(${REACTOS_BINARY_DIR}/include/reactos/idl)

spec2def(wlanapi ${CMAKE_CURRENT_SOURCE_DIR}/wlanapi.spec ${CMAKE_CURRENT_BINARY_DIR}/wlanapi.def)

add_library(wlanapi SHARED main.c)

set_entrypoint(wlanapi 0)

target_link_libraries(wlanapi
    ${CMAKE_CURRENT_BINARY_DIR}/wlanapi.def
    wlansvc_client
    wine
    pseh)

add_importlibs(wlanapi rpcrt4 kernel32 ntdll)
add_dependencies(wlanapi wlanapi_def psdk buildno_header)
