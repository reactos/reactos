
list(APPEND SOURCE
    dialogs.c
    editwnd.c
    mainwnd.c
    misc.c
    opensave.c
    wordpad.c)

add_executable(old_wordpad ${SOURCE} rsrc.rc)
set_module_type(old_wordpad win32gui UNICODE)
add_importlibs(old_wordpad user32 gdi32 comdlg32 advapi32 shell32 comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET old_wordpad DESTINATION reactos/system32 FOR all)
