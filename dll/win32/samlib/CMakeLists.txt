
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

add_definitions(-D__WINESRC__)
spec2def(samlib.dll samlib.spec ADD_IMPORTLIB)
add_rpc_files(client ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl/sam.idl)

list(APPEND SOURCE
    dllmain.c
    samlib.c
    precomp.h
    ${CMAKE_CURRENT_BINARY_DIR}/sam_c.c
    ${CMAKE_CURRENT_BINARY_DIR}/samlib_stubs.c)

add_library(samlib MODULE
    ${SOURCE}
    samlib.rc
    ${CMAKE_CURRENT_BINARY_DIR}/samlib.def)

set_module_type(samlib win32dll UNICODE)
target_link_libraries(samlib wine ${PSEH_LIB})
add_importlibs(samlib rpcrt4 advapi32 msvcrt kernel32 ntdll)
add_pch(samlib precomp.h SOURCE)
add_cd_file(TARGET samlib DESTINATION reactos/system32 FOR all)
