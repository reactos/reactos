
if(ARCH STREQUAL "i386")
    list(APPEND LIBCNTPR_SETJMP_ASM_SOURCE
        setjmp/i386/setjmp.s
    )
elseif(ARCH STREQUAL "amd64")
    list(APPEND LIBCNTPR_SETJMP_ASM_SOURCE
        setjmp/amd64/setjmp.s
    )
elseif(ARCH STREQUAL "arm")
    list(APPEND LIBCNTPR_SETJMP_ASM_SOURCE
        setjmp/arm/setjmp.s
    )
endif()

list(APPEND CRT_SETJMP_ASM_SOURCE
    ${LIBCNTPR_SETJMP_ASM_SOURCE}
)
