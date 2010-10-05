
set_unicode()

add_executable(welcome
    welcome.c
    welcome.rc)

set_module_type(welcome win32gui)
add_importlibs(welcome gdi32 user32 msvcrt)
