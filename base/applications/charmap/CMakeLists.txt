
set_rc_compiler()

list(APPEND SOURCE
    about.c
    charmap.c
    lrgcell.c
    map.c
    settings.c
    charmap.rc)

add_executable(charmap ${SOURCE})

set_module_type(charmap win32gui UNICODE)

add_importlibs(charmap advapi32 user32 gdi32 comctl32 msvcrt kernel32)

add_pch(charmap precomp.h)

add_cd_file(TARGET charmap DESTINATION reactos/system32 FOR all)
