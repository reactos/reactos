
set_unicode()
set_rc_compiler()

add_executable(mspaint
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

set_module_type(mspaint win32gui)
add_pch(mspaint precomp.h)
add_importlibs(mspaint comdlg32 shell32 user32 gdi32 advapi32 comctl32 msvcrt kernel32)
add_cd_file(TARGET mspaint DESTINATION reactos/system32 FOR all)
