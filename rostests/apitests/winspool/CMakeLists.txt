
list(APPEND SOURCE
    ClosePrinter.c
    EnumPrinters.c
    EnumPrintProcessorDatatypes.c
    GetPrintProcessorDirectory.c
    IsValidDevmode.c
    OpenPrinter.c
    StartDocPrinter.c
    testlist.c)

add_executable(winspool_apitest ${SOURCE})
target_link_libraries(winspool_apitest wine ${PSEH_LIB})
set_module_type(winspool_apitest win32cui)
add_importlibs(winspool_apitest winspool msvcrt kernel32 ntdll)
add_cd_file(TARGET winspool_apitest DESTINATION reactos/bin FOR all)
