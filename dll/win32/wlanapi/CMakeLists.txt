
include_directories(${REACTOS_BINARY_DIR}/include/reactos/idl)

spec2def(wlanapi.dll wlanapi.spec)

add_library(wlanapi SHARED main.c
    ${CMAKE_CURRENT_BINARY_DIR}/wlanapi.def)

set_entrypoint(wlanapi 0)

target_link_libraries(wlanapi
    wlansvc_client
    wine
    ${PSEH_LIB})

add_importlibs(wlanapi rpcrt4 kernel32 ntdll)
add_dependencies(wlanapi psdk buildno_header)

add_cab_target(wlanapi 1)