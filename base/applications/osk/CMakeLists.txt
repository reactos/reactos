
file(GLOB osk_rc_deps res/*.*)
add_rc_deps(rsrc.rc ${osk_rc_deps})
add_executable(osk main.c settings.c rsrc.rc)
set_module_type(osk win32gui UNICODE)
add_importlibs(osk comdlg32 shell32 user32 gdi32 advapi32 comctl32 msvcrt kernel32)
add_cd_file(TARGET osk DESTINATION reactos/system32 FOR all)
