
add_executable(hh 
    main.c
    hh.rc)

set_unicode(hh no)

set_module_type(hh win32gui)

add_importlibs(hh gdi32 user32 msvcrt)