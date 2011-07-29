
include_directories(${REACTOS_BINARY_DIR}/include/reactos/idl)

spec2def(wlanapi.dll wlanapi.spec)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/wlanapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/wlanapi.def)

add_library(wlanapi SHARED ${SOURCE})

set_entrypoint(wlanapi 0)

target_link_libraries(wlanapi
    wlansvcrpc
    wine
    ${PSEH_LIB})

add_importlibs(wlanapi rpcrt4 msvcrt kernel32 ntdll)
add_importlib_target(wlanapi.spec)

add_dependencies(wlanapi psdk)
add_cd_file(TARGET wlanapi DESTINATION reactos/system32 FOR all)
