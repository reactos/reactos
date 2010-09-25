add_executable(hostname
    hostname.c
    hostname.rc)

set_module_type(hostname win32cui)

set_unicode(hostname no)

add_importlibs(hostname msvcrt)