
set_unicode()

add_executable(winver winver.c)

set_module_type(winver win32gui)

add_importlibs(winver shell32 msvcrt kernel32)
