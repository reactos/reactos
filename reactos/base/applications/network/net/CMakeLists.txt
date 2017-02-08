
add_definitions(-D__USE_W32_SOCKETS)

list(APPEND SOURCE
    main.c
    cmdstart.c
    cmdStop.c
    cmdHelpMsg.c
    cmdPause.c
    cmdContinue.c
    help.c)

add_executable(net ${SOURCE})

set_module_type(net win32cui UNICODE)
add_pch(net net.h)
add_importlibs(net advapi32 msvcrt kernel32)
add_cd_file(TARGET net DESTINATION reactos/system32 FOR all)
