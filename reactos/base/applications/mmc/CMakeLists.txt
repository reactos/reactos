
list(APPEND SOURCE
    console.c
    misc.c
    mmc.c
    precomp.h)

add_rc_deps(mmc.rc ${CMAKE_CURRENT_SOURCE_DIR}/resources/mmc.ico)
add_executable(mmcclient ${SOURCE} mmc.rc)
set_module_type(mmcclient win32gui UNICODE)
add_importlibs(mmcclient user32 gdi32 comdlg32 advapi32 shell32 comctl32 msvcrt kernel32)
add_pch(mmcclient precomp.h SOURCE)
