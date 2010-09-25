add_executable(doskey
    doskey.c
    doskey.rc)

set_module_type(doskey win32cui)

set_unicode(doskey yes)

add_importlibs(doskey user32 msvcrt)