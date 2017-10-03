
list(APPEND SOURCE
    EnumPrintProcessorDatatypesW.c
    main.c
    testlist.c)

add_executable(winprint_apitest ${SOURCE})
target_link_libraries(winprint_apitest wine ${PSEH_LIB})
set_module_type(winprint_apitest win32cui)
add_importlibs(winprint_apitest winspool msvcrt kernel32 ntdll)
add_rostests_file(TARGET winprint_apitest)
