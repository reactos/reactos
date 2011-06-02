
set_unicode()
set_rc_compiler()

list(APPEND SOURCE
    about.c
    charmap.c
    lrgcell.c
    map.c
    charmap.rc)

add_executable(charmap ${SOURCE})

add_pch(charmap precomp.h)

set_module_type(charmap win32gui)

add_importlibs(charmap msvcrt user32 gdi32 comctl32 kernel32)

add_cd_file(TARGET charmap DESTINATION reactos/system32 FOR all)
