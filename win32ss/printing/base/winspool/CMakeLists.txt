
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl)
add_rpc_files(client ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl/winspool.idl)
spec2def(winspool.drv winspool.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    devmode.c
    jobs.c
    main.c
    monitors.c
    ports.c
    precomp.h
    printerdata.c
    printerdrivers.c
    printers.c
    printprocessors.c
    printproviders.c
    ${CMAKE_CURRENT_BINARY_DIR}/winspool_c.c)

add_library(winspool SHARED
    ${SOURCE}
    winspool.rc
    ${CMAKE_CURRENT_BINARY_DIR}/winspool_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/winspool.def)

set_target_properties(winspool PROPERTIES SUFFIX ".drv")
set_module_type(winspool win32dll UNICODE)
target_link_libraries(winspool wine ${PSEH_LIB})
add_importlibs(winspool advapi32 gdi32 rpcrt4 msvcrt kernel32 ntdll)
add_pch(winspool precomp.h SOURCE)
add_cd_file(TARGET winspool DESTINATION reactos/system32 FOR all)
