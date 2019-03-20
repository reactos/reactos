
add_definitions(
    -D__WINESRC__
    -DWINE_REGISTER_DLL)

include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
generate_idl_iids(qmgrprxy.idl)
spec2def(qmgrprxy.dll qmgrprxy.spec)
add_rpcproxy_files(qmgrprxy.idl)

list(APPEND SOURCE
    qmgrprxy.rc
    ${CMAKE_CURRENT_BINARY_DIR}/qmgrprxy_i.c
    ${CMAKE_CURRENT_BINARY_DIR}/qmgrprxy_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
    ${CMAKE_CURRENT_BINARY_DIR}/qmgrprxy.def)

add_library(qmgrprxy MODULE ${SOURCE})
set_module_type(qmgrprxy win32dll)
target_link_libraries(qmgrprxy ${PSEH_LIB} wine)
add_importlibs(qmgrprxy rpcrt4 msvcrt kernel32 ntdll)
add_cd_file(TARGET qmgrprxy DESTINATION reactos/system32 FOR all)
