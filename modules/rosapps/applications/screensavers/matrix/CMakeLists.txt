
add_executable(matrix
    config.c
    matrix.c
    message.c
    password.c
    screensave.c
    settings.c
    rsrc.rc)

set_module_type(matrix win32gui UNICODE)
set_target_properties(matrix PROPERTIES SUFFIX ".scr")
add_importlibs(matrix user32 gdi32 winspool advapi32 comctl32 comdlg32 shell32 msvcrt kernel32)
add_cd_file(TARGET matrix DESTINATION reactos/system32 FOR all)
