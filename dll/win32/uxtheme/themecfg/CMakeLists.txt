
set_rc_compiler()

list(APPEND SOURCE
    theme.c
    themecfg.c
    themecfg.rc)

add_executable(themecfg ${SOURCE})

set_module_type(themecfg win32gui)

add_importlibs(themecfg msvcrt user32 gdi32 comctl32 kernel32 uxtheme shell32 ole32 comdlg32 shlwapi)

add_cd_file(TARGET themecfg DESTINATION reactos/system32 FOR all)
