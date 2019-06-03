
include_directories( ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl)
add_rpc_files(client ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl/wlansvc.idl)

list(APPEND SOURCE
    main.c
    profile.c
    ${CMAKE_CURRENT_BINARY_DIR}/wlansvc_c.c)

add_library(wlanapi MODULE ${SOURCE})
spec2def(wlanapi wlanapi.dll wlanapi.spec ADD_IMPORTLIB)

set_module_type(wlanapi win32dll)
target_link_libraries(wlanapi wine ${PSEH_LIB})
add_importlibs(wlanapi rpcrt4 msvcrt kernel32 ntdll)
add_cd_file(TARGET wlanapi DESTINATION reactos/system32 FOR all)
