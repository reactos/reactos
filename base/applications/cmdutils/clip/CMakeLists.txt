
add_executable(clip clip.c clip.rc)
set_module_type(clip win32cui UNICODE)
add_importlibs(clip user32 msvcrt kernel32 advapi32)
add_cd_file(TARGET clip DESTINATION reactos/system32 FOR all)
