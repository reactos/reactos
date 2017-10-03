
PROJECT(progman)

list(APPEND SOURCE
    dialog.c
    group.c
    grpfile.c
    main.c
    program.c
    string.c
    progman.h)

add_executable(progman ${SOURCE} progman.rc)
set_module_type(progman win32gui UNICODE)
add_importlibs(progman advapi32 user32 gdi32 comctl32 comdlg32 shell32 msvcrt kernel32)
add_pch(progman progman.h SOURCE)
add_cd_file(TARGET progman DESTINATION reactos/system32 FOR all)
