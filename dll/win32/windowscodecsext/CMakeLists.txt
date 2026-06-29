
spec2def(windowscodecsext.dll windowscodecsext.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/windowscodecsext.def)

add_library(windowscodecsext MODULE ${SOURCE})
set_module_type(windowscodecsext win32dll)
target_link_libraries(windowscodecsext wine)
add_importlibs(windowscodecsext ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET windowscodecsext DESTINATION reactos/system32 FOR all)
set_wine_module(windowscodecsext)
