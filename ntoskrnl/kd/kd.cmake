
if(ARCH STREQUAL "i386")
    list(APPEND SOURCE kd/i386/kdserial.c)
elseif(ARCH STREQUAL "amd64")
    list(APPEND SOURCE kd/i386/kdserial.c)
elseif(ARCH STREQUAL "arm")
    list(APPEND SOURCE kd/arm/kdserial.c)
endif()

list(APPEND SOURCE
    kd/kdio.c
    kd/kdmain.c
    kd/kdprompt.c
    kd/kdps2kbd.c
    kd/kdserial.c
    kd/kdterminal.c)
