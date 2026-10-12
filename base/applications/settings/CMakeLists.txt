
list(APPEND SOURCE
    search.c
    settings.c)

add_executable(settings ${SOURCE} settings.rc)
set_module_type(settings win32gui UNICODE)
add_importlibs(settings user32 gdi32 shell32 comctl32 advapi32 msvcrt kernel32)
add_cd_file(TARGET settings DESTINATION reactos/system32 FOR all)
