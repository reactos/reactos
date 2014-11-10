
list(APPEND SOURCE
    magnifier.c
    settings.c
    magnifier.h)

add_executable(magnify ${SOURCE} magnify.rc)
set_module_type(magnify win32gui)
add_importlibs(magnify user32 gdi32 advapi32 shell32 msvcrt kernel32)
add_pch(magnify magnifier.h SOURCE)
add_cd_file(TARGET magnify DESTINATION reactos/system32 FOR all)
