
spec2def(netutils.dll netutils.spec)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/netutils_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/netutils.def)

add_library(netutils MODULE ${SOURCE})
set_module_type(netutils win32dll)
target_link_libraries(netutils wine)
add_importlibs(netutils msvcrt kernel32 ntdll)
add_cd_file(TARGET netutils DESTINATION reactos/system32 FOR all)
set_wine_module(netutils)
