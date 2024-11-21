
list(APPEND CRT_SOURCE
    ${CRT_CONIO_SOURCE}
    ${CRT_DIRECT_SOURCE}
    ${CRT_EXCEPT_SOURCE}
    ${CRT_FLOAT_SOURCE}
    locale/locale.c
    ${CRT_MATH_SOURCE}
    ${CRT_MBSTRING_SOURCE}
    ${CRT_MEM_SOURCE}
    ${CRT_MISC_SOURCE}
    ${CRT_PRINTF_SOURCE}
    ${CRT_PROCESS_SOURCE}
    ${CRT_SEARCH_SOURCE}
    signal/signal.c
    ${CRT_STARTUP_SOURCE}
    ${CRT_STDIO_SOURCE}
    ${CRT_STDLIB_SOURCE}
    ${CRT_STRING_SOURCE}
    sys_stat/systime.c
    ${CRT_TIME_SOURCE}
    ${CRT_WINE_SOURCE}
    ${CRT_WSTRING_SOURCE}
)

list(APPEND CRT_ASM_SOURCE
    ${CRT_EXCEPT_ASM_SOURCE}
    ${CRT_FLOAT_ASM_SOURCE}
    ${CRT_MATH_ASM_SOURCE}
    ${CRT_SETJMP_ASM_SOURCE}
    ${CRT_STDLIB_ASM_SOURCE}
    ${CRT_STRING_ASM_SOURCE}
    ${CRT_WINE_ASM_SOURCE}
)

set_source_files_properties(${CRT_ASM_SOURCE} PROPERTIES COMPILE_DEFINITIONS "__MINGW_IMPORT=extern;USE_MSVCRT_PREFIX;_MSVCRT_LIB_;_MSVCRT_;_MT;CRTDLL")
add_asm_files(crt_asm ${CRT_ASM_SOURCE})

add_library(crt ${CRT_SOURCE} ${crt_asm})
target_link_libraries(crt chkstk ${PSEH_LIB})
target_compile_definitions(crt
 PRIVATE    __MINGW_IMPORT=extern
    USE_MSVCRT_PREFIX
    _MSVCRT_LIB_
    _MSVCRT_
    _MT
    CRTDLL)
#add_pch(crt precomp.h)
add_dependencies(crt psdk asm)
