
set_source_files_properties(${ARMRT_MATH_SOURCE} PROPERTIES COMPILE_DEFINITIONS "NO_RTL_INLINES;_NTSYSTEM_;_NTDLLBUILD_;_LIBCNT_;__CRT__NO_INLINE;CRTDLL")
add_asm_files(armrt_asm ${ARMRT_MATH_ASM_SOURCE})

add_library(armrt STATIC ${ARMRT_MATH_SOURCE} ${armrt_asm})
target_compile_definitions(armrt
 PRIVATE    NO_RTL_INLINES
    _NTSYSTEM_
    _NTDLLBUILD_
    _LIBCNT_
    __CRT__NO_INLINE
    CRTDLL)
add_dependencies(armrt psdk asm)
