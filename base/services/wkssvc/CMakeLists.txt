
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl)
add_rpc_files(server ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl/wkssvc.idl)

add_library(wkssvc MODULE
    rpcserver.c
    wkssvc.c
    wkssvc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/wkssvc_s.c)
spec2def(wkssvc wkssvc.dll wkssvc.spec ADD_IMPORTLIB)

set_module_type(wkssvc win32dll UNICODE)
target_link_libraries(wkssvc wine)
add_importlibs(wkssvc advapi32 rpcrt4 msvcrt kernel32 ntdll)
add_cd_file(TARGET wkssvc DESTINATION reactos/system32 FOR all)
