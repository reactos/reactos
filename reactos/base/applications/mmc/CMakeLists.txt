
list(APPEND SOURCE
    console.c
    misc.c
    mmc.c
    precomp.h)

add_rc_deps(mmc.rc ${CMAKE_CURRENT_SOURCE_DIR}/resources/mmc.ico)
add_executable(mmc ${SOURCE} mmc.rc)
set_module_type(mmc win32gui UNICODE)
add_importlibs(mmc user32 gdi32 comdlg32 advapi32 shell32 comctl32 msvcrt kernel32 ntdll)
add_pch(mmc precomp.h SOURCE)
add_cd_file(TARGET mmc DESTINATION reactos/system32 FOR all)
