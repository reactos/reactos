
add_executable(label label.c label.rc)
set_module_type(label win32cui UNICODE)
target_link_libraries(label conutils)
add_importlibs(label msvcrt kernel32)
add_cd_file(TARGET label DESTINATION reactos/system32 FOR all)
