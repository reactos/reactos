
list(APPEND LIBCNTPR_SOURCE
    ${LIBCNTPR_EXCEPT_SOURCE}
    ${LIBCNTPR_FLOAT_SOURCE}
    ${LIBCNTPR_MATH_SOURCE}
    ${LIBCNTPR_MBSTRING_SOURCE}
    ${LIBCNTPR_MEM_SOURCE}
    ${LIBCNTPT_MISC_SOURCE}
    ${LIBCNTPR_PRINTF_SOURCE}
    ${LIBCNTPR_SEARCH_SOURCE}
    ${LIBCNTPR_STDLIB_SOURCE}
    ${LIBCNTPR_STRING_SOURCE}
    ${LIBCNTPR_WSTRING_SOURCE}
)

list(APPEND LIBCNTPR_ASM_SOURCE
    ${LIBCNTPR_EXCEPT_ASM_SOURCE}
    ${LIBCNTPR_FLOAT_ASM_SOURCE}
    ${LIBCNTPR_MATH_ASM_SOURCE}
    ${LIBCNTPR_MEM_ASM_SOURCE}
    ${LIBCNTPR_SETJMP_ASM_SOURCE}
    ${LIBCNTPR_STRING_ASM_SOURCE}
)

if(ARCH STREQUAL "arm")
    list(APPEND LIBCNTPR_SOURCE
        math/arm/__rt_sdiv.c
        math/arm/__rt_sdiv64_worker.c
        math/arm/__rt_udiv.c
        math/arm/__rt_udiv64_worker.c
        math/arm/__rt_div_worker.h
        math/arm/__dtoi64.c
        math/arm/__dtou64.c
        math/arm/__stoi64.c
        math/arm/__stou64.c
        math/arm/__fto64.h
        math/arm/__i64tod.c
        math/arm/__u64tod.c
        math/arm/__i64tos.c
        math/arm/__u64tos.c
        math/arm/__64tof.h
    )
    list(APPEND LIBCNTPR_ASM_SOURCE
        except/arm/chkstk_asm.s
        math/arm/__rt_sdiv64.s
        math/arm/__rt_srsh.s
        math/arm/__rt_udiv64.s
    )
endif()

set_source_files_properties(${LIBCNTPR_ASM_SOURCE} PROPERTIES COMPILE_DEFINITIONS "NO_RTL_INLINES;_NTSYSTEM_;_NTDLLBUILD_;_LIBCNT_;__CRT__NO_INLINE;CRTDLL")
add_asm_files(libcntpr_asm ${LIBCNTPR_ASM_SOURCE})

add_library(libcntpr STATIC ${LIBCNTPR_SOURCE} ${libcntpr_asm})
target_compile_definitions(libcntpr
 PRIVATE    NO_RTL_INLINES
    _NTSYSTEM_
    _NTDLLBUILD_
    _LIBCNT_
    __CRT__NO_INLINE
    CRTDLL)
add_dependencies(libcntpr psdk asm)
