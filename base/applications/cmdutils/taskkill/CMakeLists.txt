
add_executable(taskkill taskkill.c taskkill.rc)
target_link_libraries(taskkill wine)
set_module_type(taskkill win32cui UNICODE)
add_importlibs(taskkill user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET taskkill DESTINATION reactos/system32 FOR all)
set_wine_module_FIXME(taskkill) # CORE-5743: No ARRAY_SIZE macro
