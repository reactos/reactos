
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl)
add_rpc_files(server ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl/netlogon.idl)
spec2def(netlogon.dll netlogon.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    netlogon.c
    rpcserver.c
    precomp.h)

add_library(netlogon MODULE
    ${SOURCE}
    netlogon.rc
    ${CMAKE_CURRENT_BINARY_DIR}/netlogon_s.c
    ${CMAKE_CURRENT_BINARY_DIR}/netlogon_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/netlogon.def)

set_module_type(netlogon win32dll UNICODE)
target_link_libraries(netlogon wine ${PSEH_LIB})
add_importlibs(netlogon advapi32 rpcrt4 msvcrt kernel32 ntdll)
add_pch(netlogon precomp.h SOURCE)
add_cd_file(TARGET netlogon DESTINATION reactos/system32 FOR all)
