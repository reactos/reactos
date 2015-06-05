
list(APPEND SOURCE
    EnumPrintProcessorDatatypesW.c
    testlist.c)

add_executable(winprint_apitest ${SOURCE})
target_link_libraries(winprint_apitest wine ${PSEH_LIB})
set_module_type(winprint_apitest win32cui)
add_importlibs(winprint_apitest winprint msvcrt kernel32 ntdll)
add_cd_file(TARGET winprint_apitest DESTINATION reactos/bin FOR all)
