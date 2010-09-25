add_executable(find
    find.c
    find.rc)

set_module_type(find win32cui)

set_unicode(find no)

add_importlibs(find user32 msvcrt)