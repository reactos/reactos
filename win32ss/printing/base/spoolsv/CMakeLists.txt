
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl)
add_rpc_files(server ${REACTOS_SOURCE_DIR}/sdk/include/reactos/idl/winspool.idl)

spec2def(spoolsv.exe spoolsv.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    forms.c
    init.c
    jobs.c
    main.c
    monitors.c
    notifications.c
    ports.c
    precomp.h
    printerdata.c
    printerdrivers.c
    printers.c
    printprocessors.c
    printproviders.c
    rpcserver.c
    rpcstubs.c
    spoolfile.c
    xcv.c
    ${CMAKE_CURRENT_BINARY_DIR}/winspool_s.c
    ${CMAKE_CURRENT_BINARY_DIR}/spoolsv_stubs.c)

add_executable(spoolsv
    ${SOURCE}
    spoolsv.rc
    ${CMAKE_CURRENT_BINARY_DIR}/spoolsv.def)

set_target_properties(spoolsv
    PROPERTIES
    ENABLE_EXPORTS TRUE
    DEFINE_SYMBOL "")

if(USE_CLANG_CL)
    target_compile_options(spoolsv PRIVATE "-Wno-cast-calling-convention")
endif()

set_module_type(spoolsv win32cui UNICODE)
target_link_libraries(spoolsv wine)
add_delay_importlibs(spoolsv spoolss)
add_importlibs(spoolsv advapi32 rpcrt4 msvcrt kernel32 ntdll)
add_pch(spoolsv precomp.h SOURCE)
add_cd_file(TARGET spoolsv DESTINATION reactos/system32 FOR all)
