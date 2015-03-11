
include_directories( ${REACTOS_SOURCE_DIR}/include/reactos/idl)
add_rpc_files(client ${REACTOS_SOURCE_DIR}/include/reactos/idl/wlansvc.idl)
spec2def(wlanapi.dll wlanapi.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c
    profile.c
    ${CMAKE_CURRENT_BINARY_DIR}/wlanapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/wlanapi.def
    ${CMAKE_CURRENT_BINARY_DIR}/wlansvc_c.c)

add_library(wlanapi SHARED ${SOURCE})
set_module_type(wlanapi win32dll)
target_link_libraries(wlanapi wine ${PSEH_LIB})
add_importlibs(wlanapi rpcrt4 msvcrt kernel32 ntdll)
add_cd_file(TARGET wlanapi DESTINATION reactos/system32 FOR all)
