
spec2def(ntprint.dll ntprint.spec)

list(APPEND SOURCE
    ntprint.c
    ${CMAKE_CURRENT_BINARY_DIR}/ntprint_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/ntprint.def)

add_library(ntprint MODULE ${SOURCE} ntprint.rc)
set_module_type(ntprint win32dll)
target_link_libraries(ntprint wine)
add_importlibs(ntprint winspool msvcrt kernel32 ntdll)
add_cd_file(TARGET ntprint DESTINATION reactos/system32 FOR all)
set_wine_module(ntprint)
