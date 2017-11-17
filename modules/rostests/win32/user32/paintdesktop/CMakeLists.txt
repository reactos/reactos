
add_executable(paintdesktop PaintDesktop.c)
set_module_type(paintdesktop win32gui UNICODE)
add_importlibs(paintdesktop user32 msvcrt kernel32)
add_rostests_file(TARGET paintdesktop SUBDIR suppl)
