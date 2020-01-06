
include_directories(${REACTOS_SOURCE_DIR}/win32ss/printing/include)

list(APPEND SOURCE
    fpEnumPrinters.c
    fpGetPrintProcessorDirectory.c
    fpSetJob.c
    main.c)

add_library(localspl_apitest.dll MODULE ${SOURCE})
target_link_libraries(localspl_apitest.dll wine ${PSEH_LIB})
set_module_type(localspl_apitest.dll win32dll)
add_importlibs(localspl_apitest.dll spoolss advapi32 msvcrt kernel32 ntdll)
set_target_properties(localspl_apitest.dll PROPERTIES SUFFIX "")
add_rostests_file(TARGET localspl_apitest.dll)
