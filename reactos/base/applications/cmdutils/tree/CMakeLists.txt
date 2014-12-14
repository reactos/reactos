
add_executable(tree tree.c)
set_module_type(tree win32cui UNICODE)
add_importlibs(tree msvcrt kernel32 user32)
add_cd_file(TARGET tree DESTINATION reactos/system32 FOR all)
