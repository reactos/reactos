
add_executable(winemine
    main.c
    dialog.c
    rsrc.rc)

set_unicode(winemine yes)

set_module_type(winemine win32gui)

add_importlibs(winemine user32 gdi32 advapi32 shell32 msvcrt)