
spec2def(mssign32.dll mssign32.spec)

list(APPEND SOURCE
    mssign32_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/mssign32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/mssign32.def)

add_library(mssign32 MODULE ${SOURCE})
set_module_type(mssign32 win32dll)
target_link_libraries(mssign32 wine)
add_importlibs(mssign32 msvcrt kernel32 ntdll)
add_cd_file(TARGET mssign32 DESTINATION reactos/system32 FOR all)
set_wine_module(mssign32)
