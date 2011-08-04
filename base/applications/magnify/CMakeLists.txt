
set_rc_compiler()

add_executable(magnify
    magnifier.c
    settings.c
    magnify.rc)

add_pch(magnify magnifier.h)
set_module_type(magnify win32gui)
add_importlibs(magnify user32 gdi32 advapi32 shell32 msvcrt kernel32)
add_cd_file(TARGET magnify DESTINATION reactos/system32 FOR all)
