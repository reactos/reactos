
set_unicode()

add_definitions(
    -D_DLL
    -D__USE_CRTIMP
    -D_SETUPAPI_
    -D_CFGMGR32_
    -Dwcsnicmp=_wcsnicmp)

include_directories(
    ${REACTOS_SOURCE_DIR}/include/reactos/wine
    ${REACTOS_BINARY_DIR}/include/reactos/idl)

spec2def(setupapi ${CMAKE_CURRENT_SOURCE_DIR}/setupapi.spec ${CMAKE_CURRENT_BINARY_DIR}/setupapi.def)

add_library(setupapi SHARED
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
    setupapi.rc)

set_module_type(setupapi win32dll)

target_link_libraries(setupapi
    ${CMAKE_CURRENT_BINARY_DIR}/setupapi.def
    pnp_client
    uuid
    wine
    pseh)

add_importlibs(setupapi
    msvcrt
    gdi32
    comctl32
    advapi32
    user32
    rpcrt4
    version
    shell32
    kernel32
    ntdll)

add_dependencies(setupapi setupapi_def)
add_livecd_target(setupapi reactos/system32)
