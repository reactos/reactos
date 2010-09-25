add_executable(more
    more.c
    more.rc)

set_module_type(more win32cui)

set_unicode(more no)

add_importlibs(more ntdll user32 msvcrt)