
list(APPEND SOURCE
    clipbrd.c
    cliputils.c
    winutils.c
    fileutils.c
    scrollutils.c
    precomp.h)

add_executable(clipbrd ${SOURCE} clipbrd.rc)
add_pch(clipbrd precomp.h SOURCE)
set_module_type(clipbrd win32gui UNICODE)
add_importlibs(clipbrd hhctrl user32 gdi32 comdlg32 shell32 msvcrt kernel32)
add_cd_file(TARGET clipbrd DESTINATION reactos/system32 FOR all)
