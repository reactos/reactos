
list(APPEND SOURCE
    ClosePrinter.c
    EnumPrinters.c
    EnumPrintProcessorDatatypes.c
    GetDefaultPrinter.c
    GetPrinter.c
    GetPrinterData.c
    GetPrintProcessorDirectory.c
    IsValidDevmode.c
    OpenPrinter.c
    StartDocPrinter.c
    testlist.c)

add_executable(winspool_apitest ${SOURCE})
target_link_libraries(winspool_apitest wine ${PSEH_LIB})
set_module_type(winspool_apitest win32cui)
add_importlibs(winspool_apitest winspool msvcrt kernel32 ntdll)
add_rostests_file(TARGET winspool_apitest)
