
add_definitions(-D_DLL -D__USE_CRTIMP)

list(APPEND SOURCE
    misc.c
    shutdown.c
    shutdown.rc)

add_executable(shutdown ${SOURCE})

add_pch(shutdown precomp.h)

set_module_type(shutdown win32cui)
add_importlibs(shutdown advapi32 user32 msvcrt kernel32)
add_cd_file(TARGET shutdown DESTINATION reactos/system32 FOR all)
