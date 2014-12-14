
add_executable(tree tree.c tree.rc)
set_module_type(tree win32cui UNICODE)
set_target_properties(tree PROPERTIES SUFFIX ".com")
add_importlibs(tree msvcrt kernel32 user32)
add_cd_file(TARGET tree DESTINATION reactos/system32 FOR all)
