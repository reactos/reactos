
set_unicode()

list(APPEND SOURCE
    dialog.c
    misc.c
    mixer.c
    sndvol32.c
    sndvol32.rc)

add_executable(sndvol32 ${SOURCE})

add_pch(sndvol32 sndvol32.h)

set_module_type(sndvol32 win32gui)
add_importlibs(sndvol32 user32 advapi32 gdi32 comctl32 shell32 winmm msvcrt kernel32 ntdll)
add_cd_file(TARGET sndvol32 DESTINATION reactos/system32 FOR all)
