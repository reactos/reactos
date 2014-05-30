
add_definitions(-D__USE_W32_SOCKETS)

list(APPEND SOURCE
    main.c
    cmdAccounts.c
    cmdStart.c
    cmdStop.c
    cmdHelpMsg.c
    cmdPause.c
    cmdContinue.c
    help.c
    net.h)

add_executable(net ${SOURCE})
set_module_type(net win32cui UNICODE)
add_importlibs(net advapi32 netapi32 msvcrt kernel32 ntdll)
add_pch(net net.h SOURCE)
add_cd_file(TARGET net DESTINATION reactos/system32 FOR all)
