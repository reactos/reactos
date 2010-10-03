
set_unicode()

add_executable(paint
    dialogs.c
    dib.c
    drawing.c
    history.c
    main.c
    mouse.c
    palette.c
    registry.c
    selection.c
    sizebox.c
    toolsettings.c
    winproc.c
    rsrc.rc)

set_module_type(paint win32gui)

add_importlibs(paint comdlg32 shell32 user32 gdi32 advapi32 comctl32 msvcrt)
