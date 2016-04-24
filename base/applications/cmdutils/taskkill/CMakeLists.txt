
add_executable(taskkill taskkill.c taskkill.rc)
target_link_libraries(taskkill wine)
set_module_type(taskkill win32cui UNICODE)
add_importlibs(taskkill psapi user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET taskkill DESTINATION reactos/system32 FOR all)
