
add_subdirectory(dll)

list(APPEND SOURCE
    service.c
    tests.c
    testlist.c)

add_executable(localspl_apitest ${SOURCE})
target_link_libraries(localspl_apitest wine ${PSEH_LIB})
set_module_type(localspl_apitest win32cui)
add_importlibs(localspl_apitest advapi32 winspool msvcrt kernel32 ntdll)
add_rostests_file(TARGET localspl_apitest)
