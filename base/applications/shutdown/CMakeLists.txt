
list(APPEND SOURCE
    gui.c
    misc.c
    shutdown.c
    precomp.h)

add_executable(shutdown ${SOURCE} shutdown.rc)
set_module_type(shutdown win32cui UNICODE)
add_importlibs(shutdown advapi32 user32 powrprof msvcrt kernel32)
add_pch(shutdown precomp.h SOURCE)
add_cd_file(TARGET shutdown DESTINATION reactos/system32 FOR all)
