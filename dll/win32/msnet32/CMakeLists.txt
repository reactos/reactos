
spec2def(msnet32.dll msnet32.spec)
add_library(msnet32 MODULE msnet_main.c ${CMAKE_CURRENT_BINARY_DIR}/msnet32.def)
set_module_type(msnet32 win32dll)
target_link_libraries(msnet32 wine)
add_importlibs(msnet32 msvcrt kernel32 ntdll)
add_cd_file(TARGET msnet32 DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(msnet32) # CORE-5743
