
list(APPEND SOURCE
    dialog.c
    process.c
    about.c
    registry.c
    precomp.h)

add_rc_deps(utilman.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/utilman.ico)
add_executable(utilman ${SOURCE} utilman.rc)
set_module_type(utilman win32gui UNICODE)
add_importlibs(utilman comdlg32 user32 gdi32 advapi32 shell32 comctl32 msvcrt kernel32 ntdll)
add_pch(utilman precomp.h SOURCE)
add_cd_file(TARGET utilman DESTINATION reactos/system32 FOR all)
