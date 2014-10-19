
add_definitions(-D__USE_W32_SOCKETS)

list(APPEND SOURCE
    main.c
    cmdAccounts.c
    cmdContinue.c
    cmdHelpMsg.c
    cmdLocalGroup.c
    cmdPause.c
    cmdStart.c
    cmdStop.c
    help.c
    net.h)

add_executable(net ${SOURCE} net.rc)
set_module_type(net win32cui UNICODE)
add_importlibs(net advapi32 netapi32 msvcrt kernel32 user32 ntdll)
add_pch(net net.h SOURCE)
add_cd_file(TARGET net DESTINATION reactos/system32 FOR all)
