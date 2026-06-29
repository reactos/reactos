
add_definitions(
    -D_WINE)

spec2def(msvidc32.dll msvidc32.spec)

list(APPEND SOURCE
    msvideo1.c
    ${CMAKE_CURRENT_BINARY_DIR}/msvidc32.def)

add_library(msvidc32 MODULE ${SOURCE} rsrc.rc)
set_module_type(msvidc32 win32dll)
target_link_libraries(msvidc32 wine)
add_importlibs(msvidc32 user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET msvidc32 DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(msvidc32) # CORE-5743: No ARRAY_SIZE macro
