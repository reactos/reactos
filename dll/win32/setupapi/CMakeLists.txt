
include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/include/reactos/idl
    ${CMAKE_CURRENT_BINARY_DIR})
    
add_definitions(
    -D_SETUPAPI_
    -D_CFGMGR32_
    -Dwcsnicmp=_wcsnicmp)

add_rpc_files(client ${REACTOS_SOURCE_DIR}/include/reactos/idl/pnp.idl)
spec2def(setupapi.dll setupapi.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    cfgmgr.c
    devclass.c
    devinst.c
    dirid.c
    diskspace.c
    driver.c
    install.c
    interface.c
    misc.c
    parser.c
    query.c
    queue.c
    setupcab.c
    stringtable.c
    stubs.c
    rpc.c
    setupapi_private.h
    ${CMAKE_CURRENT_BINARY_DIR}/pnp_c.c)

add_library(setupapi SHARED
    ${SOURCE}
    setupapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/setupapi_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/setupapi.def)

set_module_type(setupapi win32dll UNICODE)
target_link_libraries(setupapi uuid wine ${PSEH_LIB})
add_delay_importlibs(setupapi shell32 winspool wintrust)

add_importlibs(setupapi
    msvcrt
    gdi32
    comctl32
    advapi32
    user32
    rpcrt4
    version
    kernel32
    ntdll)

add_pch(setupapi setupapi_private.h SOURCE)
add_cd_file(TARGET setupapi DESTINATION reactos/system32 FOR all)
