
list(APPEND LIBCNTPR_FLOAT_SOURCE
    float/isnan.c
)

list(APPEND CRT_FLOAT_SOURCE
    ${LIBCNTPR_FLOAT_SOURCE}
    float/chgsign.c
    float/copysign.c
    float/fpclass.c
    float/fpecode.c
    float/nafter.c
    float/scalb.c
)

if(ARCH STREQUAL "i386")
    list(APPEND CRT_FLOAT_SOURCE
        float/i386/clearfp.c
        float/i386/cntrlfp.c
        float/i386/fpreset.c
        float/i386/logb.c
        float/i386/statfp.c
    )
elseif(ARCH STREQUAL "amd64")
    list(APPEND CRT_FLOAT_SOURCE
        float/i386/cntrlfp.c
        float/i386/statfp.c
    )
    list(APPEND CRT_FLOAT_ASM_SOURCE
        float/amd64/clearfp.S
        float/amd64/getsetfpcw.S
        float/amd64/fpreset.S
        float/amd64/logb.S
    )
elseif(ARCH STREQUAL "arm")
    list(APPEND CRT_FLOAT_SOURCE
        float/arm/_clearfp.c
        float/arm/_controlfp.c
        float/arm/_fpreset.c
        float/arm/_statusfp.c
    )
    list(APPEND CRT_FLOAT_ASM_SOURCE
        float/arm/__getfp.s
        float/arm/__setfp.s
    )
endif()
