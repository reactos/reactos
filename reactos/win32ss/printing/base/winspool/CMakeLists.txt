
include_directories(${REACTOS_SOURCE_DIR}/include/reactos/idl)
add_rpc_files(client ${REACTOS_SOURCE_DIR}/include/reactos/idl/winspool.idl)
spec2def(winspool winspool.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/winspool_c.c)

add_library(winspool SHARED
    ${SOURCE}
    winspool.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winspool_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/winspool.def)

set_target_properties(winspool PROPERTIES SUFFIX ".drv")
set_module_type(winspool win32dll UNICODE)
target_link_libraries(winspool wine)
add_importlibs(winspool kernel32 msvcrt ntdll rpcrt4)
add_pch(winspool precomp.h SOURCE)
add_cd_file(TARGET winspool DESTINATION reactos/system32 FOR all)
