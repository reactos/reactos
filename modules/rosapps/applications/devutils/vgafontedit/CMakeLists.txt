
list(APPEND SOURCE
    aboutdlg.c
    editglyphdlg.c
    fontboxeswnd.c
    fontwnd.c
    main.c
    mainwnd.c
    misc.c
    opensave.c
    precomp.h)

add_executable(vgafontedit ${SOURCE} vgafontedit.rc)
add_pch(vgafontedit precomp.h SOURCE)
set_module_type(vgafontedit win32gui UNICODE)
add_importlibs(vgafontedit shell32 user32 gdi32 comdlg32 msvcrt kernel32)
add_cd_file(TARGET vgafontedit DESTINATION reactos/system32 FOR all)
