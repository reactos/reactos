
list(APPEND SOURCE
    dialog.c
    main.c
    settings.c
    text.c
    rsrc.rc)

set_rc_compiler()

add_executable(notepad ${SOURCE})

set_module_type(notepad win32gui UNICODE)
add_importlibs(notepad user32 gdi32 comctl32 comdlg32 advapi32 shell32 msvcrt kernel32)
add_pch(notepad notepad.h)
add_cd_file(TARGET notepad DESTINATION reactos/system32 FOR all)
