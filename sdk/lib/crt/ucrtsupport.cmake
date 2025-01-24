
list(APPEND UCRTSUPP_SOURCE
    ${CRT_FLOAT_SOURCE}
    ${CRT_MATH_SOURCE}
    wine/undname.c
)

list(APPEND UCRTSUPP_ASM_SOURCE
    ${CRT_EXCEPT_ASM_SOURCE}
    ${CRT_FLOAT_ASM_SOURCE}
    ${CRT_MATH_ASM_SOURCE}
    ${CRT_SETJMP_ASM_SOURCE}
)

add_asm_files(ucrtsupp_asm ${UCRTSUPP_ASM_SOURCE})

add_library(ucrtsupport ${UCRTSUPP_SOURCE} ${ucrtsupp_asm})
target_link_libraries(ucrtsupport chkstk ${PSEH_LIB})
target_compile_definitions(ucrtsupport PRIVATE
    CRTDLL
    _MSVCRT_LIB_
    _MSVCRT_
    _MT
    USE_MSVCRT_PREFIX
    __MINGW_IMPORT=extern
    __fma3_lib_init=__acrt_initialize_fma3
)
#add_pch(crt precomp.h)
add_dependencies(ucrtsupport psdk asm)
