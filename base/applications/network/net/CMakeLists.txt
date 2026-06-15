
add_definitions(-D__USE_W32_SOCKETS)

list(APPEND SOURCE
    main.c
    cmdAccounts.c
    cmdComputer.c
    cmdConfig.c
    cmdContinue.c
    cmdGroup.c
    cmdHelp.c
    cmdHelpMsg.c
    cmdLocalGroup.c
    cmdPause.c
    cmdSession.c
    cmdShare.c
    cmdStart.c
    cmdStatistics.c
    cmdStop.c
    cmdUse.c
    cmdUser.c
    net.h)

add_executable(net ${SOURCE} net.rc)
add_pch(net net.h SOURCE)
set_module_type(net win32cui UNICODE)
target_link_libraries(net conutils)
add_dependencies(net net_msg)
add_importlibs(net advapi32 netapi32 mpr msvcrt kernel32 ntdll)
add_cd_file(TARGET net DESTINATION reactos/system32 FOR all)
