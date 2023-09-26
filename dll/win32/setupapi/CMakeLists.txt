
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl
    ${CMAKE_CURRENT_BINARY_DIR})

add_definitions(-D_SETUPAPI_ -D_CFGMGR32_)
remove_definitions(-D_CRT_NON_CONFORMING_SWPRINTFS)

add_rpc_files(client ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl/pnp.idl)
spec2def(setupapi.dll setupapi.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    cfgmgr.c
    devclass.c
    devinst.c
    dialog.c
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
    rpc.c)

list(APPEND PCH_SKIP_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/setupapi_stubs.c)

add_library(setupapi MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    ${CMAKE_CURRENT_BINARY_DIR}/pnp_c.c
    setupapi.rc
    ${CMAKE_CURRENT_BINARY_DIR}/setupapi.def)

set_module_type(setupapi win32dll UNICODE)
target_link_libraries(setupapi uuid wine ${PSEH_LIB} oldnames)
add_delay_importlibs(setupapi comdlg32 shell32 winspool.drv wintrust)
add_importlibs(setupapi gdi32 comctl32 advapi32 user32 rpcrt4 version msvcrt kernel32 ntdll)
add_pch(setupapi setupapi_private.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET setupapi DESTINATION reactos/system32 FOR all)
