
include_directories(${REACTOS_SOURCE_DIR}/win32ss/printing/include)

list(APPEND SOURCE
    fpEnumPrinters.c
    main.c)

add_library(localspl_apitest.dll SHARED ${SOURCE})
target_link_libraries(localspl_apitest.dll wine ${PSEH_LIB})
set_module_type(localspl_apitest.dll win32dll)
add_importlibs(localspl_apitest.dll spoolss msvcrt kernel32 ntdll)
set_target_properties(localspl_apitest.dll PROPERTIES SUFFIX "")
add_cd_file(TARGET localspl_apitest.dll DESTINATION reactos/bin FOR all)
