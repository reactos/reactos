
list(APPEND SOURCE
    rtcapi.c
    rtcuserapi.c
)

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE i386/_RTC_CheckEsp.S)
endif()

add_asm_files(runtmchk_asm ${ASM_SOURCE})
add_library(runtmchk ${SOURCE} ${runtmchk_asm})
add_dependencies(runtmchk asm)
