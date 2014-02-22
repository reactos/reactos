
list(APPEND SOURCE
    main.c
    pnp_list_manager.c
    pnp_list_lock.c
    pnp.c
    services.c
    debug.c
    audiosrv.h)

add_executable(audiosrv ${SOURCE} audiosrv.rc)
set_module_type(audiosrv win32cui UNICODE)
add_importlibs(audiosrv advapi32 user32 setupapi msvcrt kernel32)
add_pch(audiosrv audiosrv.h SOURCE)
add_cd_file(TARGET audiosrv DESTINATION reactos/system32 FOR all)
