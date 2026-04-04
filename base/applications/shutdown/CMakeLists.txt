
list(APPEND SOURCE
    gui.c
    misc.c
    shutdown.c
    precomp.h)

add_executable(shutdown ${SOURCE} shutdown.rc)
add_pch(shutdown precomp.h SOURCE)
set_module_type(shutdown win32cui UNICODE)
target_link_libraries(shutdown conutils)
add_importlibs(shutdown advapi32 powrprof user32 msvcrt kernel32)
add_cd_file(TARGET shutdown DESTINATION reactos/system32 FOR all)
