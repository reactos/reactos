
add_executable(fontview
    display.c
    fontview.c
    fontview.rc)

set_module_type(fontview win32gui)
add_importlibs(fontview gdi32 shell32 user32 msvcrt kernel32)
