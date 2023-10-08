
list(APPEND SOURCE
    kd64/kdapi.c
    kd64/kdbreak.c
    kd64/kddata.c
    kd64/kdinit.c
    kd64/kdlock.c
    kd64/kdprint.c
    kd64/kdtrap.c
    )

if(ARCH STREQUAL "i386")
    list(APPEND SOURCE
        kd64/i386/kdx86.c
    )
elseif(ARCH STREQUAL "amd64")
    list(APPEND SOURCE
        kd64/amd64/kdx64.c
    )
elseif(ARCH STREQUAL "arm")
    list(APPEND SOURCE
        kd64/arm/kdarm.c
    )
endif()
