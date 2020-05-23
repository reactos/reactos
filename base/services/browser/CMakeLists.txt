
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl)
add_rpc_files(server ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl/browser.idl)
spec2def(browser.dll browser.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    browser.c
    rpcserver.c
    precomp.h)

add_library(browser MODULE
    ${SOURCE}
    browser.rc
    ${CMAKE_CURRENT_BINARY_DIR}/browser_s.c
    ${CMAKE_CURRENT_BINARY_DIR}/browser_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/browser.def)

set_module_type(browser win32dll UNICODE)
target_link_libraries(browser wine ${PSEH_LIB})
add_importlibs(browser advapi32 rpcrt4 msvcrt kernel32 ntdll)
add_pch(browser precomp.h SOURCE)
add_cd_file(TARGET browser DESTINATION reactos/system32 FOR all)
