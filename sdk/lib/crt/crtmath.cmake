
list(APPEND CRTMATH_SOURCE
    ${CRT_FLOAT_SOURCE}
    ${CRT_MATH_SOURCE}
)

list(APPEND CRTMATH_ASM_SOURCE
    ${CRT_FLOAT_ASM_SOURCE}
    ${CRT_MATH_ASM_SOURCE}
)

add_asm_files(crtmath_asm ${CRTMATH_ASM_SOURCE})

add_library(crtmath ${CRTMATH_SOURCE} ${crtmath_asm})
target_compile_definitions(crtmath PRIVATE
    USE_MSVCRT_PREFIX
    __fma3_lib_init=__acrt_initialize_fma3
)
add_dependencies(crtmath psdk asm)
