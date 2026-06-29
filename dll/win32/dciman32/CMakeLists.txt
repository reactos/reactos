
spec2def(dciman32.dll dciman32.spec)

list(APPEND SOURCE
    dciman_main.c
    ${CMAKE_CURRENT_BINARY_DIR}/dciman32_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/dciman32.def)

add_library(dciman32 MODULE ${SOURCE})
set_module_type(dciman32 win32dll)
target_link_libraries(dciman32 wine)
add_importlibs(dciman32 msvcrt kernel32 ntdll)
add_cd_file(TARGET dciman32 DESTINATION reactos/system32 FOR all)
set_wine_module(dciman32)
