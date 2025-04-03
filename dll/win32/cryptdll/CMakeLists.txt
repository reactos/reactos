
spec2def(cryptdll.dll cryptdll.spec)

list(APPEND SOURCE
    cryptdll.c
    stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/cryptdll.def)

add_library(cryptdll MODULE ${SOURCE})
set_module_type(cryptdll win32dll)
target_link_libraries(cryptdll wine)
add_importlibs(cryptdll advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET cryptdll DESTINATION reactos/system32 FOR all)
set_wine_module(cryptdll)
