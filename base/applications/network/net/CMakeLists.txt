
add_definitions(-D__USE_W32_SOCKETS)

list(APPEND SOURCE
    main.c
    cmdstart.c
    cmdStop.c
    help.c
    process.c)

add_executable(net ${SOURCE})

set_module_type(net win32cui)

add_importlibs(net ws2_32 msvcrt kernel32)

add_livecd_target(net reactos/system32)