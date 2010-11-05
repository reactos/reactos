
set_unicode()

add_executable(write write.c rsrc.rc)

set_module_type(write win32gui)

add_importlibs(write user32 gdi32 msvcrt kernel32)

add_cab_target(write 1)