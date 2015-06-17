
list(APPEND SOURCE
    display.c
    fontview.c
    precomp.h)

add_rc_deps(fontview.rc ${CMAKE_CURRENT_SOURCE_DIR}/ttf.ico)
add_executable(fontview ${SOURCE} fontview.rc)
set_module_type(fontview win32gui)
add_importlibs(fontview comdlg32 gdi32 shell32 user32 msvcrt kernel32)
add_pch(fontview precomp.h SOURCE)
add_cd_file(TARGET fontview DESTINATION reactos/system32 FOR all)
