
add_executable(whoami whoami.c whoami.rc)
set_module_type(whoami win32cui UNICODE)
add_importlibs(whoami user32 secur32 advapi32 msvcrt kernel32)
add_cd_file(TARGET whoami DESTINATION reactos/system32 FOR all)
