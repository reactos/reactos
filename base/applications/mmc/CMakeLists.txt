
list(APPEND SOURCE
    console.c
    misc.c
    mmc.c
    mscfile.c
    precomp.h)

add_rc_deps(mmc.rc ${CMAKE_CURRENT_SOURCE_DIR}/resources/mmc.ico)
add_executable(mmc ${SOURCE} mmc.rc)
set_module_type(mmc win32gui UNICODE)
target_link_libraries(mmc uuid)
add_importlibs(mmc user32 gdi32 comdlg32 advapi32 shell32 comctl32 ole32 oleaut32 shlwapi msvcrt kernel32 ntdll)
add_pch(mmc precomp.h SOURCE)
add_cd_file(TARGET mmc DESTINATION reactos/system32 FOR all)
