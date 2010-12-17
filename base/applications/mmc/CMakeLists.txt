
set_unicode()

list(APPEND SOURCE
    console.c
    misc.c
    mmc.c
    mmc.rc)

add_executable(mmcclient ${CMAKE_CURRENT_BINARY_DIR}/mmcclient_precomp.h.gch ${SOURCE})

add_pch(mmcclient ${CMAKE_CURRENT_SOURCE_DIR}/precomp.h ${SOURCE})

set_module_type(mmcclient win32gui)
add_importlibs(mmcclient user32 gdi32 comdlg32 advapi32 shell32 comctl32 msvcrt kernel32)
