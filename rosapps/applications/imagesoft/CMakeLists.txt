
list(APPEND SOURCE
    about.c
    adjust.c
    brightness.c
    contrast.c
    custcombo.c
    floatwindow.c
    font.c
    imagesoft.c
    imgedwnd.c
    mainwnd.c
    misc.c
    opensave.c
    tooldock.c
    precomp.h
    )

add_executable(imagesoft ${SOURCE} imagesoft.rc)
add_pch(imagesoft precomp.h SOURCE)
set_module_type(imagesoft win32gui UNICODE)
add_importlibs(imagesoft gdi32 user32 advapi32 version comctl32 shell32 comdlg32 msvcrt kernel32)
add_cd_file(TARGET imagesoft DESTINATION reactos/system32 FOR all)
