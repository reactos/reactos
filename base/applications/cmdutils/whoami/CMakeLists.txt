
add_executable(whoami whoami.c whoami.rc)
set_module_type(whoami win32cui UNICODE)
target_link_libraries(whoami conutils)
add_importlibs(whoami advapi32 secur32 msvcrt kernel32)
add_cd_file(TARGET whoami DESTINATION reactos/system32 FOR all)
