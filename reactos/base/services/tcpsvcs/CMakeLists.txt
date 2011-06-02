
set_unicode()

list(APPEND SOURCE
    tcpsvcs.c
    skelserver.c
    echo.c
    discard.c
    daytime.c
    qotd.c
    chargen.c
    tcpsvcs.rc
    log.c)

add_executable(tcpsvcs ${SOURCE})

add_pch(tcpsvcs tcpsvcs.h)
set_module_type(tcpsvcs win32cui)
add_importlibs(tcpsvcs ws2_32 advapi32 msvcrt kernel32 ntdll)


add_cd_file(TARGET tcpsvcs DESTINATION reactos/system32 FOR all)
add_cd_file(FILE ${CMAKE_CURRENT_SOURCE_DIR}/quotes DESTINATION reactos/system32/drivers/etc FOR all)