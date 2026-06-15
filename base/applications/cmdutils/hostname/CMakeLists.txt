
add_executable(hostname hostname.c hostname.rc)
set_module_type(hostname win32cui UNICODE)
target_link_libraries(hostname conutils)
add_importlibs(hostname msvcrt kernel32)
add_cd_file(TARGET hostname DESTINATION reactos/system32 FOR all)
