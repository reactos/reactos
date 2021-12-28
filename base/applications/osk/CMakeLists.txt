
list(APPEND SOURCE
    keyboard.c
    main.c
    settings.c
    precomp.h)

file(GLOB osk_rc_deps res/*.*)
add_rc_deps(osk.rc ${osk_rc_deps})
add_executable(osk ${SOURCE} osk.rc)
set_module_type(osk win32gui UNICODE)
add_importlibs(osk comdlg32 winmm shell32 user32 gdi32 advapi32 comctl32 msvcrt kernel32 ntdll)
add_pch(osk precomp.h SOURCE)
add_cd_file(TARGET osk DESTINATION reactos/system32 FOR all)
