
list(APPEND SOURCE
    about.c
    charmap.c
    lrgcell.c
    map.c
    settings.c
    precomp.h)

add_rc_deps(charmap.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/charmap.ico)
add_executable(charmap ${SOURCE} charmap.rc)
set_module_type(charmap win32gui UNICODE)
add_importlibs(charmap advapi32 user32 gdi32 comctl32 msvcrt kernel32)
add_pch(charmap precomp.h SOURCE)
add_cd_file(TARGET charmap DESTINATION reactos/system32 FOR all)
