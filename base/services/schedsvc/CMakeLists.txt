
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/idl)
add_rpc_files(server ${REACTOS_SOURCE_DIR}/include/reactos/idl/atsvc.idl)
spec2def(schedsvc.dll schedsvc.spec ADD_IMPORTLIB)

add_library(schedsvc SHARED
    rpcserver.c
    schedsvc.c
    schedsvc.rc
    ${CMAKE_CURRENT_BINARY_DIR}/atsvc_s.c
    ${CMAKE_CURRENT_BINARY_DIR}/schedsvc.def)

set_module_type(schedsvc win32dll UNICODE)
target_link_libraries(schedsvc wine)
add_importlibs(schedsvc advapi32 rpcrt4 msvcrt kernel32 ntdll)
add_cd_file(TARGET schedsvc DESTINATION reactos/system32 FOR all)
