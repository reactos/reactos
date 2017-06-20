
add_executable(user user.c user.rc)
set_module_type(user win32gui ENTRYPOINT mainCRTStartup)
add_importlibs(user msvcrt kernel32)
add_cd_file(TARGET user DESTINATION reactos/system32 FOR all)
