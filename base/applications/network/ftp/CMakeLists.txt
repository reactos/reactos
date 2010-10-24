add_executable(ftp
    cmds.c
    cmdtab.c
    domacro.c
    fake.c
    ftp.c
    main.c
    ruserpass.c
    ftp.rc)

set_module_type(ftp win32cui)

add_importlibs(ftp ws2_32 iphlpapi msvcrt kernel32)

add_livecd_target(ftp reactos/system32)