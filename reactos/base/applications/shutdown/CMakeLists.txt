
list(APPEND SOURCE
    gui.c
    misc.c
    shutdown.c
    shutdown.rc)

add_executable(shutdown ${SOURCE})
set_module_type(shutdown win32cui UNICODE)
add_importlibs(shutdown advapi32 user32 msvcrt powrprof kernel32)
add_pch(shutdown precomp.h)
add_cd_file(TARGET shutdown DESTINATION reactos/system32 FOR all)
