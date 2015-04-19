
add_executable(syscalldump syscalldump.c)
set_module_type(syscalldump win32cui)
target_link_libraries(syscalldump wine)
add_importlibs(syscalldump dbghelp msvcrt kernel32)
add_cd_file(TARGET syscalldump DESTINATION reactos/system32 FOR all)
