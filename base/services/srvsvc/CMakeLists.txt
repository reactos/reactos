
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl)
add_rpc_files(server ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl/srvsvc.idl)

add_library(srvsvc MODULE
    rpcserver.c
    srvsvc.c
    srvsvc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/srvsvc_s.c)
spec2def(srvsvc srvsvc.dll srvsvc.spec ADD_IMPORTLIB)

set_module_type(srvsvc win32dll UNICODE)
target_link_libraries(srvsvc wine)
add_importlibs(srvsvc advapi32 rpcrt4 msvcrt kernel32 ntdll)
add_cd_file(TARGET srvsvc DESTINATION reactos/system32 FOR all)
