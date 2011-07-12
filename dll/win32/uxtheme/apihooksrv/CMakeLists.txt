
set_rc_compiler()

list(APPEND SOURCE
    apihooksrv.c)

add_executable(apihooksrv ${SOURCE})

set_module_type(apihooksrv win32cui)

add_importlibs(apihooksrv msvcrt user32 kernel32)

add_cd_file(TARGET apihooksrv DESTINATION reactos/system32 FOR all)
