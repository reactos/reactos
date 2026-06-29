
add_executable(replace replace.c util.c replace.rc)
set_module_type(replace win32cui UNICODE)
target_link_libraries(replace conutils)
add_importlibs(replace msvcrt kernel32)
add_cd_file(TARGET replace DESTINATION reactos/system32 FOR all)
