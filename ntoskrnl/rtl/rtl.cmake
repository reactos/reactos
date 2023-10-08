
list(APPEND SOURCE
    rtl/libsupp.c
    rtl/misc.c
    )

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE
        rtl/i386/stack.S)
elseif(ARCH STREQUAL "amd64")
elseif(ARCH STREQUAL "arm")
    list(APPEND SOURCE
        rtl/arm/rtlexcpt.c)
endif()
