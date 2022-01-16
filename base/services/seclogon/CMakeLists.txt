
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl)
add_rpc_files(server ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl/seclogon.idl)
spec2def(seclogon.dll seclogon.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    rpcserver.c
    seclogon.c
    precomp.h)

add_library(seclogon MODULE
    ${SOURCE}
    seclogon.rc
    ${CMAKE_CURRENT_BINARY_DIR}/seclogon_s.c
    ${CMAKE_CURRENT_BINARY_DIR}/seclogon_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/seclogon.def)

set_module_type(seclogon win32dll UNICODE)
target_link_libraries(seclogon wine ${PSEH_LIB})
add_importlibs(seclogon advapi32 rpcrt4 msvcrt kernel32 ntdll)
add_pch(seclogon precomp.h SOURCE)
add_cd_file(TARGET seclogon DESTINATION reactos/system32 FOR all)
