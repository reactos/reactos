
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

add_executable(tcpsvcs ${CMAKE_CURRENT_BINARY_DIR}/tcpsvcs_tcpsvcs.h.gch ${SOURCE})

add_pch(tcpsvcs ${CMAKE_CURRENT_SOURCE_DIR}/tcpsvcs.h ${SOURCE})
set_module_type(tcpsvcs win32cui)
add_importlibs(tcpsvcs ws2_32 advapi32 msvcrt)

