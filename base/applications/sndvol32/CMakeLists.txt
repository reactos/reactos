
set_unicode()

list(APPEND SOURCE
    misc.c
    mixer.c
    sndvol32.c
    sndvol32.rc)

add_executable(sndvol32 ${CMAKE_CURRENT_BINARY_DIR}/sndvol32_sndvol32.h.gch ${SOURCE})

add_pch(sndvol32 ${CMAKE_CURRENT_SOURCE_DIR}/sndvol32.h ${SOURCE})

set_module_type(sndvol32 win32gui)

add_importlibs(sndvol32 user32 advapi32 gdi32 comctl32 shell32 winmm msvcrt kernel32)
