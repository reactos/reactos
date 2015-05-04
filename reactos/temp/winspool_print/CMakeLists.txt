add_executable(winspool_print main.c)
set_module_type(winspool_print win32cui)
add_importlibs(winspool_print kernel32 msvcrt winspool)
add_cd_file(TARGET winspool_print DESTINATION reactos/system32 FOR all)
