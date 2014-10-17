
list(APPEND SOURCE rtcapi.c)

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE i386/_RTC_CheckEsp.S)
endif()

add_asm_files(RunTmChk_asm ${ASM_SOURCE})
add_library(RunTmChk  ${SOURCE} ${RunTmChk_asm})
add_dependencies(RunTmChk asm)
