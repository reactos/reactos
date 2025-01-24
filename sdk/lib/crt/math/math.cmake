
include_directories(libm_sse2)

list(APPEND LIBCNTPR_MATH_SOURCE
    math/_chgsignf.c
    math/_invoke_matherr.c
    math/abs.c
    math/div.c
    math/exp2f.c
    math/labs.c
    math/sincos.c
)

if(ARCH STREQUAL "i386")
    list(APPEND LIBCNTPR_MATH_SOURCE
        math/fabs.c
        math/fabsf.c
        math/i386/ci.c
        math/i386/cicos.c
        math/i386/cilog.c
        math/i386/cipow.c
        math/i386/cisin.c
        math/i386/cisqrt.c
        math/i386/ldexp.c
    )
    list(APPEND LIBCNTPR_MATH_ASM_SOURCE
        math/i386/alldiv_asm.s
        math/i386/alldvrm_asm.s
        math/i386/allmul_asm.s
        math/i386/allrem_asm.s
        math/i386/allshl_asm.s
        math/i386/allshr_asm.s
        math/i386/atan_asm.s
        math/i386/atan2_asm.s
        math/i386/aulldiv_asm.s
        math/i386/aulldvrm_asm.s
        math/i386/aullrem_asm.s
        math/i386/aullshr_asm.s
        math/i386/ceil_asm.s
        math/i386/cos_asm.s
        # math/i386/fabs_asm.s # FIXME
        math/i386/floor_asm.s
        math/i386/ftol_asm.s
        math/i386/ftol2_asm.s
        math/i386/ftoul2_legacy_asm.s
        math/i386/log_asm.s
        math/i386/log10_asm.s
        math/i386/pow_asm.s
        math/i386/sin_asm.s
        math/i386/sqrt_asm.s
        math/i386/tan_asm.s
    )
    list(APPEND CRT_MATH_ASM_SOURCE
        math/i386/ceilf.S
        math/i386/floorf.S
        math/i386/exp_asm.s
        math/i386/fmod_asm.s
        math/i386/fmodf_asm.s
    )
    list(APPEND CRT_MATH_SOURCE
        math/_hypotf.c
    )
elseif(ARCH STREQUAL "amd64")
    list(APPEND LIBCNTPR_MATH_SOURCE
        math/amd64/_set_statfp.c
        # math/libm_sse2/_chgsign.c
        # math/libm_sse2/_chgsignf.c
        # math/libm_sse2/_copysign.c
        # math/libm_sse2/_copysignf.c
        # math/libm_sse2/_finite.c
        # math/libm_sse2/_finitef.c
        math/libm_sse2/_handle_error.c
        math/libm_sse2/acos.c
        math/libm_sse2/acosf.c
        math/libm_sse2/asin.c
        math/libm_sse2/asinf.c
        math/libm_sse2/atan.c
        math/libm_sse2/atan2.c
        math/libm_sse2/atan2f.c
        math/libm_sse2/atanf.c
        # math/libm_sse2/cabs.c
        # math/libm_sse2/cabsf.c
        math/libm_sse2/ceil.c
        math/libm_sse2/ceilf.c
        math/libm_sse2/cosh.c
        math/libm_sse2/coshf.c
        math/libm_sse2/exp_special.c
        math/libm_sse2/exp2.c
        math/libm_sse2/floor.c
        math/libm_sse2/floorf.c
        math/libm_sse2/fma3_available.c
        math/libm_sse2/hypot.c
        math/libm_sse2/hypotf.c
        math/libm_sse2/L2_by_pi_bits.c
        math/libm_sse2/ldexp.c
        # math/libm_sse2/ldexpf.c
        math/libm_sse2/log_128_lead_tail_table.c
        math/libm_sse2/log_256_lead_tail_table.c
        math/libm_sse2/log_F_inv_dword_table.c
        math/libm_sse2/log_F_inv_qword_table.c
        math/libm_sse2/log_special.c
        math/libm_sse2/log10_128_lead_tail_table.c
        math/libm_sse2/log10_256_lead_tail_table.c
        math/libm_sse2/logb.c
        math/libm_sse2/logbf.c
        math/libm_sse2/Lsincos_array.c
        math/libm_sse2/Lsincosf_array.c
        math/libm_sse2/modf.c
        math/libm_sse2/modff.c
        math/libm_sse2/pow_special.c
        # math/libm_sse2/remainder.c
        math/libm_sse2/remainder_piby2.c
        # math/libm_sse2/remainder_piby2f.c
        # math/libm_sse2/remainderf.c
        math/libm_sse2/sincos_special.c
        math/libm_sse2/sinh.c
        math/libm_sse2/sinhf.c
        math/libm_sse2/sqrt.c
        math/libm_sse2/sqrtf.c
        math/libm_sse2/tan.c
        math/libm_sse2/tanf.c
        math/libm_sse2/tanh.c
        math/libm_sse2/tanhf.c
        math/libm_sse2/two_to_jby64_head_tail_table.c
        math/libm_sse2/two_to_jby64_table.c
    )
    list(APPEND LIBCNTPR_MATH_ASM_SOURCE
        math/libm_sse2/fm.inc
        math/libm_sse2/cos.asm
        math/libm_sse2/cosf.asm
        math/libm_sse2/exp.asm
        math/libm_sse2/expf.asm
        math/amd64/fabs.S
        math/amd64/fabsf.S
        math/libm_sse2/fmod.asm
        math/libm_sse2/fmodf.asm
        math/libm_sse2/log.asm
        math/libm_sse2/log10.asm
        math/libm_sse2/pow.asm
        math/libm_sse2/remainder_piby2_forAsm.asm
        math/libm_sse2/remainder_piby2_forFMA3.asm
        math/libm_sse2/remainder_piby2f_forAsm.asm
        math/libm_sse2/remainder_piby2f_forC.asm
        math/libm_sse2/sin.asm
        math/libm_sse2/sinf.asm
    )
elseif(ARCH STREQUAL "arm")
    list(APPEND LIBCNTPR_MATH_SOURCE
        math/cos.c
        math/ceilf.c
        math/fabs.c
        math/fabsf.c
        math/floorf.c
        math/sin.c
        math/sqrt.c
        math/sqrtf.c
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
    list(APPEND CRT_MATH_SOURCE
        math/_hypotf.c
        math/acosf.c
        math/asinf.c
        math/atan2f.c
        math/atanf.c
        math/coshf.c
        math/expf.c
        math/fabsf.c
        math/fmodf.c
        math/modff.c
        math/sinf.c
        math/sinhf.c
        math/tanf.c
        math/tanhf.c
    )
    list(APPEND LIBCNTPR_MATH_ASM_SOURCE
        math/arm/atan.s
        math/arm/atan2.s
        math/arm/ceil.s
        math/arm/exp.s
        math/arm/fmod.s
        math/arm/floor.s
        math/arm/ldexp.s
        math/arm/log.s
        math/arm/log10.s
        math/arm/pow.s
        math/arm/tan.s
        math/arm/__rt_sdiv64.s
        math/arm/__rt_srsh.s
        math/arm/__rt_udiv64.s
    )
    list(APPEND CRT_MATH_ASM_SOURCE
        math/arm/_logb.s
    )
endif()

if(NOT ARCH STREQUAL "i386")
    list(APPEND CRT_MATH_SOURCE
        math/_copysignf.c
        math/log10f.c
        math/stubs.c
    )
endif()

if(NOT ARCH STREQUAL "amd64")
    list(APPEND CRT_MATH_SOURCE
        math/acos.c
        math/asin.c
        math/cosh.c
        math/cosf.c
        math/exp2.c
        math/hypot.c
        math/modf.c
        math/s_modf.c
        math/sinh.c
        math/tanh.c
    )
endif()

list(APPEND CRT_MATH_SOURCE
    ${LIBCNTPR_MATH_SOURCE}
    math/adjust.c
    math/cabs.c
    math/fdivbug.c
    math/frexp.c
    math/huge_val.c
    math/ieee754/j0_y0.c
    math/ieee754/j1_y1.c
    math/ieee754/jn_yn.c
    math/j0_y0.c
    math/j1_y1.c
    math/jn_yn.c
    math/ldiv.c
    math/logf.c
    math/powf.c
)

list(APPEND CRT_MATH_ASM_SOURCE
    ${LIBCNTPR_MATH_ASM_SOURCE}
)

if(ARCH STREQUAL "i386")
    list(APPEND ATAN2_ASM_SOURCE math/i386/atan2_asm.s)
elseif(ARCH STREQUAL "amd64")
    list(APPEND ATAN2_SOURCE math/_invoke_matherr.c math/amd64/_set_statfp.c math/libm_sse2/_handle_error.c math/libm_sse2/atan2.c)
elseif(ARCH STREQUAL "arm")
    list(APPEND ATAN2_ASM_SOURCE math/arm/atan2.s)
elseif(ARCH STREQUAL "arm64")
    list(APPEND ATAN2_ASM_SOURCE math/arm64/atan2.s)
endif()

add_asm_files(atan2_asm ${ATAN2_ASM_SOURCE})
add_library(atan2 ${ATAN2_SOURCE} ${atan2_asm})
set_target_properties(atan2 PROPERTIES LINKER_LANGUAGE "C")
add_dependencies(atan2 asm)
