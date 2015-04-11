
list(APPEND SOURCE
    dialogs.c
    dib.c
    drawing.c
    history.c
    main.c
    mouse.c
    palette.c
    registry.c
    scrollbox.c
    selection.c
    sizebox.c
    textedit.c
    toolsettings.c
    winproc.c
    precomp.h)

add_executable(mspaint ${SOURCE} rsrc.rc)
set_module_type(mspaint win32gui UNICODE)
add_importlibs(mspaint hhctrl comdlg32 shell32 user32 gdi32 advapi32 comctl32 msvcrt kernel32)
add_pch(mspaint precomp.h SOURCE)
add_cd_file(TARGET mspaint DESTINATION reactos/system32 FOR all)
