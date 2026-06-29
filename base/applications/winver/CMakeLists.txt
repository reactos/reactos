
list(APPEND SOURCE
    osinfo.c
    winver.c
    winver_p.h)

add_executable(winver ${SOURCE} winver.rc)
add_pch(winver winver_p.h SOURCE)
set_module_type(winver win32gui UNICODE)
add_importlibs(winver advapi32 user32 shell32 comctl32 msvcrt kernel32)
add_cd_file(TARGET winver DESTINATION reactos/system32 FOR all)
