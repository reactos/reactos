
add_executable(cmd_help
    help.c
    help.rc)

set_module_type(cmd_help win32cui UNICODE)
add_importlibs(cmd_help msvcrt kernel32 user32)

set_target_properties(cmd_help PROPERTIES OUTPUT_NAME "help")
add_cd_file(TARGET cmd_help DESTINATION reactos/system32 FOR all)
