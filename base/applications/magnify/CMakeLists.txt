
add_executable(magnify
    magnifier.c
    settings.c
    magnify.rc)

set_module_type(magnify win32gui)
add_importlibs(magnify user32 gdi32 advapi32 shell32 msvcrt kernel32)
add_cab_target(magnify 1)
