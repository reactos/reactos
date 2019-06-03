
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)

list(APPEND SOURCE
    main.c)

add_library(windowscodecsext MODULE ${SOURCE})
spec2def(windowscodecsext windowscodecsext.dll windowscodecsext.spec ADD_IMPORTLIB)

set_module_type(windowscodecsext win32dll)
target_link_libraries(windowscodecsext wine)
add_importlibs(windowscodecsext ole32 msvcrt kernel32 ntdll)
add_cd_file(TARGET windowscodecsext DESTINATION reactos/system32 FOR all)
