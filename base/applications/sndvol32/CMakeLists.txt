
list(APPEND SOURCE
    dialog.c
    misc.c
    mixer.c
    sndvol32.c
    sndvol32.h)

add_rc_deps(sndvol32.rc ${CMAKE_CURRENT_SOURCE_DIR}/resources/sndvol32.ico)
add_executable(sndvol32 ${SOURCE} sndvol32.rc)
set_module_type(sndvol32 win32gui UNICODE)
add_importlibs(sndvol32 user32 advapi32 gdi32 comctl32 shell32 winmm msvcrt kernel32 ntdll)
add_pch(sndvol32 sndvol32.h SOURCE)
add_cd_file(TARGET sndvol32 DESTINATION reactos/system32 FOR all)
