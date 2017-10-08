
add_executable(createspec createspec.c)
set_module_type(createspec win32cui)
target_link_libraries(createspec wine)
add_importlibs(createspec dbghelp msvcrt kernel32)
add_cd_file(TARGET createspec DESTINATION reactos/system32 FOR all)
