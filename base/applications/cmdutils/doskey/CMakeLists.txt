
set_unicode()

add_executable(doskey doskey.c doskey.rc)

set_module_type(doskey win32cui)
add_importlibs(doskey user32 msvcrt)
