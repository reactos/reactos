/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM64 NEON intrinsics for MSVC
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#if !defined(_M_ARM64)
#error This header is for ARM64 only
#endif

#include <stdint.h>

#ifndef DUMMYNEONSTRUCT
#define DUMMYNEONSTRUCT s
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef unsigned __int8  poly8_t;
typedef unsigned __int16 poly16_t;
typedef unsigned __int32 poly32_t;
typedef unsigned __int64 poly64_t;
typedef float float32_t;
typedef double float64_t;

typedef union __declspec(intrin_type) __n8
{
    unsigned __int8 n8_u8[1];
    __int8 n8_i8[1];
    poly8_t n8_p8[1];
} __n8;

typedef union __declspec(intrin_type) __declspec(align(2)) __n16
{
    unsigned __int16 n16_u16[1];
    unsigned __int8 n16_u8[2];
    __int16 n16_i16[1];
    __int8 n16_i8[2];
    poly16_t n16_p16[1];
    poly8_t n16_p8[2];
} __n16;

typedef union __declspec(intrin_type) __declspec(align(4)) __n32
{
    unsigned __int32 n32_u32[1];
    unsigned __int16 n32_u16[2];
    unsigned __int8 n32_u8[4];
    __int32 n32_i32[1];
    __int16 n32_i16[2];
    __int8 n32_i8[4];
    poly32_t n32_p32[1];
    poly16_t n32_p16[2];
    poly8_t n32_p8[4];
    float n32_f32[1];
} __n32;

typedef union __declspec(intrin_type) __declspec(align(8)) __n64
{
    unsigned __int64 n64_u64[1];
    unsigned __int32 n64_u32[2];
    unsigned __int16 n64_u16[4];
    unsigned __int8 n64_u8[8];
    __int64 n64_i64[1];
    __int32 n64_i32[2];
    __int16 n64_i16[4];
    __int8 n64_i8[8];
    poly64_t n64_p64[1];
    poly32_t n64_p32[2];
    poly16_t n64_p16[4];
    poly8_t n64_p8[8];
    float n64_f32[2];
    double n64_f64[1];
} __n64;

typedef union __declspec(intrin_type) __declspec(align(16)) __n128
{
    unsigned __int64 n128_u64[2];
    unsigned __int32 n128_u32[4];
    unsigned __int16 n128_u16[8];
    unsigned __int8 n128_u8[16];
    __int64 n128_i64[2];
    __int32 n128_i32[4];
    __int16 n128_i16[8];
    __int8 n128_i8[16];
    poly64_t n128_p64[2];
    poly32_t n128_p32[4];
    poly16_t n128_p16[8];
    poly8_t n128_p8[16];
    float n128_f32[4];
    double n128_f64[2];
    struct
    {
        __n64 low64;
        __n64 high64;
    } DUMMYNEONSTRUCT;
} __n128;

typedef __n128 poly128_t;

typedef __n64 int8x8_t;
typedef __n64 int16x4_t;
typedef __n64 int32x2_t;
typedef __n64 int64x1_t;
typedef __n64 uint8x8_t;
typedef __n64 uint16x4_t;
typedef __n64 uint32x2_t;
typedef __n64 uint64x1_t;
typedef __n64 float32x2_t;
typedef __n64 float64x1_t;
typedef __n64 poly8x8_t;
typedef __n64 poly16x4_t;
typedef __n64 poly64x1_t;
typedef __n128 int8x16_t;
typedef __n128 int16x8_t;
typedef __n128 int32x4_t;
typedef __n128 int64x2_t;
typedef __n128 uint8x16_t;
typedef __n128 uint16x8_t;
typedef __n128 uint32x4_t;
typedef __n128 uint64x2_t;
typedef __n128 float32x4_t;
typedef __n128 float64x2_t;
typedef __n128 poly8x16_t;
typedef __n128 poly16x8_t;
typedef __n128 poly64x2_t;

__int32 _CopyInt32FromFloat(float);
#pragma intrinsic(_CopyInt32FromFloat)

__n64 neon_add16(__n64, __n64);
__n64 neon_add32(__n64, __n64);
__n64 neon_add8(__n64, __n64);
__n64 neon_addp32(__n64, __n64);
__n64 neon_addps64(__n128);
__n128 neon_addq16(__n128, __n128);
__n128 neon_addq32(__n128, __n128);
__n128 neon_addq64(__n128, __n128);
__n128 neon_addq8(__n128, __n128);
__n64 neon_adds64(__n64, __n64);
__n16 neon_addv16(__n64);
__n8 neon_addv8(__n64);
__n16 neon_addvq16(__n128);
float neon_addvq32(__n128);
__n8 neon_addvq8(__n128);
__n128 neon_aesd(__n128, __n128);
__n128 neon_aese(__n128, __n128);
__n128 neon_aesimc(__n128);
__n128 neon_aesmc(__n128);
__n64 neon_and(__n64, __n64);
__n128 neon_andq(__n128, __n128);
__n64 neon_bic(__n64, __n64);
__n128 neon_bicq(__n128, __n128);
__n64 neon_cmeq16(__n64, __n64);
__n64 neon_cmeq32(__n64, __n64);
__n64 neon_cmeq64(__n64, __n64);
__n64 neon_cmeq8(__n64, __n64);
__n128 neon_cmeqq16(__n128, __n128);
__n128 neon_cmeqq32(__n128, __n128);
__n128 neon_cmeqq64(__n128, __n128);
__n128 neon_cmeqq8(__n128, __n128);
__n64 neon_cmeqz16(__n64);
__n64 neon_cmeqz32(__n64);
__n64 neon_cmeqz64(__n64);
__n64 neon_cmeqz8(__n64);
__n128 neon_cmeqzq16(__n128);
__n128 neon_cmeqzq32(__n128);
__n128 neon_cmeqzq64(__n128);
__n128 neon_cmeqzq8(__n128);
__n64 neon_cmge16(__n64, __n64);
__n64 neon_cmge32(__n64, __n64);
__n64 neon_cmge64(__n64, __n64);
__n64 neon_cmge8(__n64, __n64);
__n128 neon_cmgeq16(__n128, __n128);
__n128 neon_cmgeq32(__n128, __n128);
__n128 neon_cmgeq64(__n128, __n128);
__n128 neon_cmgeq8(__n128, __n128);
__n64 neon_cmgez16(__n64);
__n64 neon_cmgez32(__n64);
__n64 neon_cmgez64(__n64);
__n64 neon_cmgez8(__n64);
__n128 neon_cmgezq16(__n128);
__n128 neon_cmgezq32(__n128);
__n128 neon_cmgezq64(__n128);
__n128 neon_cmgezq8(__n128);
__n64 neon_cmgt16(__n64, __n64);
__n64 neon_cmgt32(__n64, __n64);
__n64 neon_cmgt64(__n64, __n64);
__n64 neon_cmgt8(__n64, __n64);
__n128 neon_cmgtq16(__n128, __n128);
__n128 neon_cmgtq32(__n128, __n128);
__n128 neon_cmgtq64(__n128, __n128);
__n128 neon_cmgtq8(__n128, __n128);
__n64 neon_cmgtz16(__n64);
__n64 neon_cmgtz32(__n64);
__n64 neon_cmgtz64(__n64);
__n64 neon_cmgtz8(__n64);
__n128 neon_cmgtzq16(__n128);
__n128 neon_cmgtzq32(__n128);
__n128 neon_cmgtzq64(__n128);
__n128 neon_cmgtzq8(__n128);
__n64 neon_cmhi16(__n64, __n64);
__n64 neon_cmhi32(__n64, __n64);
__n64 neon_cmhi64(__n64, __n64);
__n64 neon_cmhi8(__n64, __n64);
__n128 neon_cmhiq16(__n128, __n128);
__n128 neon_cmhiq32(__n128, __n128);
__n128 neon_cmhiq64(__n128, __n128);
__n128 neon_cmhiq8(__n128, __n128);
__n64 neon_cmhs16(__n64, __n64);
__n64 neon_cmhs32(__n64, __n64);
__n64 neon_cmhs64(__n64, __n64);
__n64 neon_cmhs8(__n64, __n64);
__n128 neon_cmhsq16(__n128, __n128);
__n128 neon_cmhsq32(__n128, __n128);
__n128 neon_cmhsq64(__n128, __n128);
__n128 neon_cmhsq8(__n128, __n128);
__n64 neon_cmlez16(__n64);
__n64 neon_cmlez32(__n64);
__n64 neon_cmlez64(__n64);
__n64 neon_cmlez8(__n64);
__n128 neon_cmlezq16(__n128);
__n128 neon_cmlezq32(__n128);
__n128 neon_cmlezq64(__n128);
__n128 neon_cmlezq8(__n128);
__n64 neon_cmltz16(__n64);
__n64 neon_cmltz32(__n64);
__n64 neon_cmltz64(__n64);
__n64 neon_cmltz8(__n64);
__n128 neon_cmltzq16(__n128);
__n128 neon_cmltzq32(__n128);
__n128 neon_cmltzq64(__n128);
__n128 neon_cmltzq8(__n128);
__n128 neon_combine(__n64, __n64);
__n128 neon_dupqr16(__int32);
__n128 neon_dupqr32(__int32);
__n128 neon_dupqr64(__int64);
__n128 neon_dupqr8(__int32);
__n128 neon_dupqrf32(float);
__n128 neon_dupqrf64(double);
__n64 neon_dupr16(__int32);
__n64 neon_dupr32(__int32);
__n64 neon_dupr64(__int64);
__n64 neon_dupr8(__int32);
__n64 neon_duprf32(float);
__n64 neon_duprf64(double);
float neon_dups32(__n64, const __int32);
float neon_dups32q(__n128, const __int32);
__n64 neon_dups64(__n64, const __int32);
__n64 neon_dups64q(__n128, const __int32);
__n64 neon_eor(__n64, __n64);
__n128 neon_eorq(__n128, __n128);
__n64 neon_ext16(__n64, __n64, const __int32);
__n64 neon_ext32(__n64, __n64, const __int32);
__n64 neon_ext64(__n64, __n64, const __int32);
__n64 neon_ext8(__n64, __n64, const __int32);
__n128 neon_extq16(__n128, __n128, const __int32);
__n128 neon_extq32(__n128, __n128, const __int32);
__n128 neon_extq64(__n128, __n128, const __int32);
__n128 neon_extq8(__n128, __n128, const __int32);
__n64 neon_fadd32(__n64, __n64);
__n64 neon_fadd64(__n64, __n64);
float neon_faddps32(__n64);
float neon_faddpsq32(__n128, __n128);
__n64 neon_faddpsq64(__n128);
__n128 neon_faddq32(__n128, __n128);
__n128 neon_faddq64(__n128, __n128);
__n64 neon_fcmeq32(__n64, __n64);
__n64 neon_fcmeq64(__n64, __n64);
__n128 neon_fcmeqq32(__n128, __n128);
__n128 neon_fcmeqq64(__n128, __n128);
__n64 neon_fcmeqz32(__n64);
__n64 neon_fcmeqz64(__n64);
__n128 neon_fcmeqzq32(__n128);
__n128 neon_fcmeqzq64(__n128);
__n64 neon_fcmge32(__n64, __n64);
__n64 neon_fcmge64(__n64, __n64);
__n128 neon_fcmgeq32(__n128, __n128);
__n128 neon_fcmgeq64(__n128, __n128);
__n64 neon_fcmgez32(__n64);
__n64 neon_fcmgez64(__n64);
__n128 neon_fcmgezq32(__n128);
__n128 neon_fcmgezq64(__n128);
__n64 neon_fcmgt32(__n64, __n64);
__n64 neon_fcmgt64(__n64, __n64);
__n128 neon_fcmgtq32(__n128, __n128);
__n128 neon_fcmgtq64(__n128, __n128);
__n64 neon_fcmgtz32(__n64);
__n64 neon_fcmgtz64(__n64);
__n128 neon_fcmgtzq32(__n128);
__n128 neon_fcmgtzq64(__n128);
__n64 neon_fcmlez32(__n64);
__n64 neon_fcmlez64(__n64);
__n128 neon_fcmlezq32(__n128);
__n128 neon_fcmlezq64(__n128);
__n64 neon_fcmltz32(__n64);
__n64 neon_fcmltz64(__n64);
__n128 neon_fcmltzq32(__n128);
__n128 neon_fcmltzq64(__n128);
__n64 neon_fmax32(__n64, __n64);
__n64 neon_fmax64(__n64, __n64);
float neon_fmaxps32(__n64);
double neon_fmaxps64(__n128);
__n128 neon_fmaxq32(__n128, __n128);
__n128 neon_fmaxq64(__n128, __n128);
float neon_fmaxv(__n128);
__n64 neon_fmin32(__n64, __n64);
__n64 neon_fmin64(__n64, __n64);
float neon_fminps32(__n64);
double neon_fminps64(__n128);
__n128 neon_fminq32(__n128, __n128);
__n128 neon_fminq64(__n128, __n128);
float neon_fminv(__n128);
__n64 neon_fmul32(__n64, __n64);
__n64 neon_fmul64(__n64, __n64);
__n128 neon_fmulq32(__n128, __n128);
__n128 neon_fmulq64(__n128, __n128);
__n64 neon_fsub32(__n64, __n64);
__n64 neon_fsub64(__n64, __n64);
__n128 neon_fsubq32(__n128, __n128);
__n128 neon_fsubq64(__n128, __n128);
__n128 neon_insqr16(__n128, const __int32, __int32);
__n128 neon_insqr32(__n128, const __int32, __int32);
__n128 neon_insqr64(__n128, const __int32, __int64);
__n128 neon_insqr8(__n128, const __int32, __int32);
__n128 neon_insqrf32(__n128, const __int32, float);
__n128 neon_insqrf64(__n128, const __int32, double);
__n64 neon_insr16(__n64, const __int32, __int32);
__n64 neon_insr32(__n64, const __int32, __int32);
__n64 neon_insr64(__n64, const __int32, __int64);
__n64 neon_insr8(__n64, const __int32, __int32);
__n64 neon_insrf32(__n64, const __int32, float);
__n64 neon_insrf64(__n64, const __int32, double);
__n64 neon_ld1m_16(const __int16 *);
__n64 neon_ld1m_32(const __int32 *);
__n64 neon_ld1m_64(const __int64 *);
__n64 neon_ld1m_8(const __int8 *);
__n128 neon_ld1m_q16(const __int16 *);
__n128 neon_ld1m_q32(const __int32 *);
__n128 neon_ld1m_q64(const __int64 *);
__n128 neon_ld1m_q8(const __int8 *);
__n64 neon_ld1r_16(const __int16 *);
__n64 neon_ld1r_32(const __int32 *);
__n64 neon_ld1r_64(const __int64 *);
__n64 neon_ld1r_8(const __int8 *);
__n128 neon_ld1r_q16(const __int16 *);
__n128 neon_ld1r_q32(const __int32 *);
__n128 neon_ld1r_q64(const __int64 *);
__n128 neon_ld1r_q8(const __int8 *);
__n64 neon_ld1s_16(const __int16 *, __n64, const __int32);
__n64 neon_ld1s_32(const __int32 *, __n64, const __int32);
__n64 neon_ld1s_64(const __int64 *, __n64, const __int32);
__n64 neon_ld1s_8(const __int8 *, __n64, const __int32);
__n128 neon_ld1s_q16(const __int16 *, __n128, const __int32);
__n128 neon_ld1s_q32(const __int32 *, __n128, const __int32);
__n128 neon_ld1s_q64(const __int64 *, __n128, const __int32);
__n128 neon_ld1s_q8(const __int8 *, __n128, const __int32);
__n64 neon_mla16(__n64, __n64, __n64);
__n64 neon_mla32(__n64, __n64, __n64);
__n64 neon_mla8(__n64, __n64, __n64);
__n128 neon_mlaq16(__n128, __n128, __n128);
__n128 neon_mlaq32(__n128, __n128, __n128);
__n128 neon_mlaq8(__n128, __n128, __n128);
__n64 neon_mls16(__n64, __n64, __n64);
__n64 neon_mls32(__n64, __n64, __n64);
__n64 neon_mls8(__n64, __n64, __n64);
__n128 neon_mlsq16(__n128, __n128, __n128);
__n128 neon_mlsq32(__n128, __n128, __n128);
__n128 neon_mlsq8(__n128, __n128, __n128);
__n64 neon_mul16(__n64, __n64);
__n64 neon_mul32(__n64, __n64);
__n64 neon_mul8(__n64, __n64);
__n128 neon_mulq16(__n128, __n128);
__n128 neon_mulq32(__n128, __n128);
__n128 neon_mulq8(__n128, __n128);
__n64 neon_not(__n64);
__n128 neon_notq(__n128);
__n64 neon_orn(__n64, __n64);
__n128 neon_ornq(__n128, __n128);
__n64 neon_orr(__n64, __n64);
__n128 neon_orrq(__n128, __n128);
__n128 neon_pmull2_64(__n128, __n128);
__n128 neon_pmull2_8(__n128, __n128);
__n128 neon_pmull_64(__n64, __n64);
__n128 neon_pmull_8(__n64, __n64);
__n64 neon_rev16(__n64);
__n128 neon_rev16q(__n128);
__n64 neon_rev32_16(__n64);
__n64 neon_rev32_8(__n64);
__n128 neon_rev32q_16(__n128);
__n128 neon_rev32q_8(__n128);
__n64 neon_rev64_16(__n64);
__n64 neon_rev64_32(__n64);
__n64 neon_rev64_8(__n64);
__n128 neon_rev64q_16(__n128);
__n128 neon_rev64q_32(__n128);
__n128 neon_rev64q_8(__n128);
__n128 neon_sha1cui(__n128, unsigned __int32, __n128);
unsigned __int32 neon_sha1hui(unsigned __int32);
__n128 neon_sha1mui(__n128, unsigned __int32, __n128);
__n128 neon_sha1pui(__n128, unsigned __int32, __n128);
__n128 neon_sha1su0(__n128, __n128, __n128);
__n128 neon_sha1su1(__n128, __n128);
__n128 neon_sha256h(__n128, __n128, __n128);
__n128 neon_sha256h2(__n128, __n128, __n128);
__n128 neon_sha256su0(__n128, __n128);
__n128 neon_sha256su1(__n128, __n128, __n128);
__n128 neon_sha512h(__n128, __n128, __n128);
__n128 neon_sha512h2(__n128, __n128, __n128);
__n128 neon_sha512su0(__n128, __n128);
__n128 neon_sha512su1(__n128, __n128, __n128);
__n64 neon_shli16(__n64, const __int32);
__n64 neon_shli32(__n64, const __int32);
__n64 neon_shli8(__n64, const __int32);
__n128 neon_shliq16(__n128, const __int32);
__n128 neon_shliq32(__n128, const __int32);
__n128 neon_shliq64(__n128, const __int32);
__n128 neon_shliq8(__n128, const __int32);
__n64 neon_shlis64(__n64, const __int32);
__n64 neon_smax16(__n64, __n64);
__n64 neon_smax32(__n64, __n64);
__n64 neon_smax8(__n64, __n64);
__n64 neon_smaxp32(__n64, __n64);
__n128 neon_smaxq16(__n128, __n128);
__n128 neon_smaxq32(__n128, __n128);
__n128 neon_smaxq8(__n128, __n128);
__n16 neon_smaxv16(__n64);
__n8 neon_smaxv8(__n64);
__n16 neon_smaxvq16(__n128);
float neon_smaxvq32(__n128);
__n8 neon_smaxvq8(__n128);
__n64 neon_smin16(__n64, __n64);
__n64 neon_smin32(__n64, __n64);
__n64 neon_smin8(__n64, __n64);
__n64 neon_sminp32(__n64, __n64);
__n128 neon_sminq16(__n128, __n128);
__n128 neon_sminq32(__n128, __n128);
__n128 neon_sminq8(__n128, __n128);
__n16 neon_sminv16(__n64);
__n8 neon_sminv8(__n64);
__n16 neon_sminvq16(__n128);
float neon_sminvq32(__n128);
__n8 neon_sminvq8(__n128);
__n128 neon_smlal2_16(__n128, __n128, __n128);
__n128 neon_smlal2_32(__n128, __n128, __n128);
__n128 neon_smlal2_8(__n128, __n128, __n128);
__n128 neon_smlal_16(__n128, __n64, __n64);
__n128 neon_smlal_32(__n128, __n64, __n64);
__n128 neon_smlal_8(__n128, __n64, __n64);
__n128 neon_smlsl2_16(__n128, __n128, __n128);
__n128 neon_smlsl2_32(__n128, __n128, __n128);
__n128 neon_smlsl2_8(__n128, __n128, __n128);
__n128 neon_smlsl_16(__n128, __n64, __n64);
__n128 neon_smlsl_32(__n128, __n64, __n64);
__n128 neon_smlsl_8(__n128, __n64, __n64);
__int16 neon_smov16(__n64, const __int32);
__int32 neon_smov32(__n64, const __int32);
__int64 neon_smov64(__n64, const __int32);
__int8 neon_smov8(__n64, const __int32);
__int16 neon_smovq16(__n128, const __int32);
__int32 neon_smovq32(__n128, const __int32);
__int64 neon_smovq64(__n128, const __int32);
__int8 neon_smovq8(__n128, const __int32);
__n128 neon_smull2_16(__n128, __n128);
__n128 neon_smull2_32(__n128, __n128);
__n128 neon_smull2_8(__n128, __n128);
__n128 neon_smull_16(__n64, __n64);
__n128 neon_smull_32(__n64, __n64);
__n128 neon_smull_8(__n64, __n64);
__n64 neon_sshri16(__n64, const __int32);
__n64 neon_sshri32(__n64, const __int32);
__n64 neon_sshri8(__n64, const __int32);
__n128 neon_sshriq16(__n128, const __int32);
__n128 neon_sshriq32(__n128, const __int32);
__n128 neon_sshriq64(__n128, const __int32);
__n128 neon_sshriq8(__n128, const __int32);
__n64 neon_sshris64(__n64, const __int32);
void neon_st1m_16(__int16 *, __n64);
void neon_st1m_32(__int32 *, __n64);
void neon_st1m_64(__int64 *, __n64);
void neon_st1m_8(__int8 *, __n64);
void neon_st1m_q16(__int16 *, __n128);
void neon_st1m_q32(__int32 *, __n128);
void neon_st1m_q64(__int64 *, __n128);
void neon_st1m_q8(__int8 *, __n128);
void neon_st1s_16(__int16 *, __n64, const __int32);
void neon_st1s_32(__int32 *, __n64, const __int32);
void neon_st1s_64(__int64 *, __n64, const __int32);
void neon_st1s_8(__int8 *, __n64, const __int32);
void neon_st1s_q16(__int16 *, __n128, const __int32);
void neon_st1s_q32(__int32 *, __n128, const __int32);
void neon_st1s_q64(__int64 *, __n128, const __int32);
void neon_st1s_q8(__int8 *, __n128, const __int32);
__n64 neon_sub16(__n64, __n64);
__n64 neon_sub32(__n64, __n64);
__n64 neon_sub8(__n64, __n64);
__n128 neon_subq16(__n128, __n128);
__n128 neon_subq32(__n128, __n128);
__n128 neon_subq64(__n128, __n128);
__n128 neon_subq8(__n128, __n128);
__n64 neon_subs64(__n64, __n64);
__n64 neon_tbl1_8(__n64, __n64);
__n64 neon_tbl1_q8(__n128, __n64);
__n128 neon_tbl1_qq8(__n128, __n128);
__n64 neon_trn1_16(__n64, __n64);
__n64 neon_trn1_32(__n64, __n64);
__n64 neon_trn1_8(__n64, __n64);
__n128 neon_trn1_q16(__n128, __n128);
__n128 neon_trn1_q32(__n128, __n128);
__n128 neon_trn1_q64(__n128, __n128);
__n128 neon_trn1_q8(__n128, __n128);
__n64 neon_trn2_16(__n64, __n64);
__n64 neon_trn2_32(__n64, __n64);
__n64 neon_trn2_8(__n64, __n64);
__n128 neon_trn2_q16(__n128, __n128);
__n128 neon_trn2_q32(__n128, __n128);
__n128 neon_trn2_q64(__n128, __n128);
__n128 neon_trn2_q8(__n128, __n128);
__n64 neon_umax16(__n64, __n64);
__n64 neon_umax32(__n64, __n64);
__n64 neon_umax8(__n64, __n64);
__n64 neon_umaxp32(__n64, __n64);
__n128 neon_umaxq16(__n128, __n128);
__n128 neon_umaxq32(__n128, __n128);
__n128 neon_umaxq8(__n128, __n128);
__n16 neon_umaxv16(__n64);
__n8 neon_umaxv8(__n64);
__n16 neon_umaxvq16(__n128);
float neon_umaxvq32(__n128);
__n8 neon_umaxvq8(__n128);
__n64 neon_umin16(__n64, __n64);
__n64 neon_umin32(__n64, __n64);
__n64 neon_umin8(__n64, __n64);
__n64 neon_uminp32(__n64, __n64);
__n128 neon_uminq16(__n128, __n128);
__n128 neon_uminq32(__n128, __n128);
__n128 neon_uminq8(__n128, __n128);
__n16 neon_uminv16(__n64);
__n8 neon_uminv8(__n64);
__n16 neon_uminvq16(__n128);
float neon_uminvq32(__n128);
__n8 neon_uminvq8(__n128);
__n128 neon_umlal2_16(__n128, __n128, __n128);
__n128 neon_umlal2_32(__n128, __n128, __n128);
__n128 neon_umlal2_8(__n128, __n128, __n128);
__n128 neon_umlal_16(__n128, __n64, __n64);
__n128 neon_umlal_32(__n128, __n64, __n64);
__n128 neon_umlal_8(__n128, __n64, __n64);
__n128 neon_umlsl2_16(__n128, __n128, __n128);
__n128 neon_umlsl2_32(__n128, __n128, __n128);
__n128 neon_umlsl2_8(__n128, __n128, __n128);
__n128 neon_umlsl_16(__n128, __n64, __n64);
__n128 neon_umlsl_32(__n128, __n64, __n64);
__n128 neon_umlsl_8(__n128, __n64, __n64);
unsigned __int16 neon_umov16(__n64, const __int32);
unsigned __int32 neon_umov32(__n64, const __int32);
unsigned __int64 neon_umov64(__n64, const __int32);
unsigned __int8 neon_umov8(__n64, const __int32);
unsigned __int16 neon_umovq16(__n128, const __int32);
unsigned __int32 neon_umovq32(__n128, const __int32);
unsigned __int64 neon_umovq64(__n128, const __int32);
unsigned __int8 neon_umovq8(__n128, const __int32);
__n128 neon_umull2_16(__n128, __n128);
__n128 neon_umull2_32(__n128, __n128);
__n128 neon_umull2_8(__n128, __n128);
__n128 neon_umull_16(__n64, __n64);
__n128 neon_umull_32(__n64, __n64);
__n128 neon_umull_8(__n64, __n64);
__n64 neon_ushri16(__n64, const __int32);
__n64 neon_ushri32(__n64, const __int32);
__n64 neon_ushri8(__n64, const __int32);
__n128 neon_ushriq16(__n128, const __int32);
__n128 neon_ushriq32(__n128, const __int32);
__n128 neon_ushriq64(__n128, const __int32);
__n128 neon_ushriq8(__n128, const __int32);
__n64 neon_ushris64(__n64, const __int32);
__n64 neon_uzp1_16(__n64, __n64);
__n64 neon_uzp1_32(__n64, __n64);
__n64 neon_uzp1_8(__n64, __n64);
__n128 neon_uzp1_q16(__n128, __n128);
__n128 neon_uzp1_q32(__n128, __n128);
__n128 neon_uzp1_q64(__n128, __n128);
__n128 neon_uzp1_q8(__n128, __n128);
__n64 neon_uzp2_16(__n64, __n64);
__n64 neon_uzp2_32(__n64, __n64);
__n64 neon_uzp2_8(__n64, __n64);
__n128 neon_uzp2_q16(__n128, __n128);
__n128 neon_uzp2_q32(__n128, __n128);
__n128 neon_uzp2_q64(__n128, __n128);
__n128 neon_uzp2_q8(__n128, __n128);
__n64 neon_zip1_16(__n64, __n64);
__n64 neon_zip1_32(__n64, __n64);
__n64 neon_zip1_8(__n64, __n64);
__n128 neon_zip1_q16(__n128, __n128);
__n128 neon_zip1_q32(__n128, __n128);
__n128 neon_zip1_q64(__n128, __n128);
__n128 neon_zip1_q8(__n128, __n128);
__n64 neon_zip2_16(__n64, __n64);
__n64 neon_zip2_32(__n64, __n64);
__n64 neon_zip2_8(__n64, __n64);
__n128 neon_zip2_q16(__n128, __n128);
__n128 neon_zip2_q32(__n128, __n128);
__n128 neon_zip2_q64(__n128, __n128);
__n128 neon_zip2_q8(__n128, __n128);

#define vld1_s8(p) neon_ld1m_8((const __int8 *)(p))
#define vst1_s8(p, v) neon_st1m_8((__int8 *)(p), (v))
#define vld1_dup_s8(p) neon_ld1r_8((const __int8 *)(p))
#define vld1_lane_s8(p, v, lane) neon_ld1s_8((const __int8 *)(p), (v), (lane))
#define vst1_lane_s8(p, v, lane) neon_st1s_8((__int8 *)(p), (v), (lane))
#define vld1q_s8(p) neon_ld1m_q8((const __int8 *)(p))
#define vst1q_s8(p, v) neon_st1m_q8((__int8 *)(p), (v))
#define vld1q_dup_s8(p) neon_ld1r_q8((const __int8 *)(p))
#define vld1q_lane_s8(p, v, lane) neon_ld1s_q8((const __int8 *)(p), (v), (lane))
#define vst1q_lane_s8(p, v, lane) neon_st1s_q8((__int8 *)(p), (v), (lane))
#define vld1_s16(p) neon_ld1m_16((const __int16 *)(p))
#define vst1_s16(p, v) neon_st1m_16((__int16 *)(p), (v))
#define vld1_dup_s16(p) neon_ld1r_16((const __int16 *)(p))
#define vld1_lane_s16(p, v, lane) neon_ld1s_16((const __int16 *)(p), (v), (lane))
#define vst1_lane_s16(p, v, lane) neon_st1s_16((__int16 *)(p), (v), (lane))
#define vld1q_s16(p) neon_ld1m_q16((const __int16 *)(p))
#define vst1q_s16(p, v) neon_st1m_q16((__int16 *)(p), (v))
#define vld1q_dup_s16(p) neon_ld1r_q16((const __int16 *)(p))
#define vld1q_lane_s16(p, v, lane) neon_ld1s_q16((const __int16 *)(p), (v), (lane))
#define vst1q_lane_s16(p, v, lane) neon_st1s_q16((__int16 *)(p), (v), (lane))
#define vld1_s32(p) neon_ld1m_32((const __int32 *)(p))
#define vst1_s32(p, v) neon_st1m_32((__int32 *)(p), (v))
#define vld1_dup_s32(p) neon_ld1r_32((const __int32 *)(p))
#define vld1_lane_s32(p, v, lane) neon_ld1s_32((const __int32 *)(p), (v), (lane))
#define vst1_lane_s32(p, v, lane) neon_st1s_32((__int32 *)(p), (v), (lane))
#define vld1q_s32(p) neon_ld1m_q32((const __int32 *)(p))
#define vst1q_s32(p, v) neon_st1m_q32((__int32 *)(p), (v))
#define vld1q_dup_s32(p) neon_ld1r_q32((const __int32 *)(p))
#define vld1q_lane_s32(p, v, lane) neon_ld1s_q32((const __int32 *)(p), (v), (lane))
#define vst1q_lane_s32(p, v, lane) neon_st1s_q32((__int32 *)(p), (v), (lane))
#define vld1_s64(p) neon_ld1m_64((const __int64 *)(p))
#define vst1_s64(p, v) neon_st1m_64((__int64 *)(p), (v))
#define vld1_dup_s64(p) neon_ld1r_64((const __int64 *)(p))
#define vld1_lane_s64(p, v, lane) neon_ld1s_64((const __int64 *)(p), (v), (lane))
#define vst1_lane_s64(p, v, lane) neon_st1s_64((__int64 *)(p), (v), (lane))
#define vld1q_s64(p) neon_ld1m_q64((const __int64 *)(p))
#define vst1q_s64(p, v) neon_st1m_q64((__int64 *)(p), (v))
#define vld1q_dup_s64(p) neon_ld1r_q64((const __int64 *)(p))
#define vld1q_lane_s64(p, v, lane) neon_ld1s_q64((const __int64 *)(p), (v), (lane))
#define vst1q_lane_s64(p, v, lane) neon_st1s_q64((__int64 *)(p), (v), (lane))
#define vld1_u8(p) neon_ld1m_8((const __int8 *)(p))
#define vst1_u8(p, v) neon_st1m_8((__int8 *)(p), (v))
#define vld1_dup_u8(p) neon_ld1r_8((const __int8 *)(p))
#define vld1_lane_u8(p, v, lane) neon_ld1s_8((const __int8 *)(p), (v), (lane))
#define vst1_lane_u8(p, v, lane) neon_st1s_8((__int8 *)(p), (v), (lane))
#define vld1q_u8(p) neon_ld1m_q8((const __int8 *)(p))
#define vst1q_u8(p, v) neon_st1m_q8((__int8 *)(p), (v))
#define vld1q_dup_u8(p) neon_ld1r_q8((const __int8 *)(p))
#define vld1q_lane_u8(p, v, lane) neon_ld1s_q8((const __int8 *)(p), (v), (lane))
#define vst1q_lane_u8(p, v, lane) neon_st1s_q8((__int8 *)(p), (v), (lane))
#define vld1_u16(p) neon_ld1m_16((const __int16 *)(p))
#define vst1_u16(p, v) neon_st1m_16((__int16 *)(p), (v))
#define vld1_dup_u16(p) neon_ld1r_16((const __int16 *)(p))
#define vld1_lane_u16(p, v, lane) neon_ld1s_16((const __int16 *)(p), (v), (lane))
#define vst1_lane_u16(p, v, lane) neon_st1s_16((__int16 *)(p), (v), (lane))
#define vld1q_u16(p) neon_ld1m_q16((const __int16 *)(p))
#define vst1q_u16(p, v) neon_st1m_q16((__int16 *)(p), (v))
#define vld1q_dup_u16(p) neon_ld1r_q16((const __int16 *)(p))
#define vld1q_lane_u16(p, v, lane) neon_ld1s_q16((const __int16 *)(p), (v), (lane))
#define vst1q_lane_u16(p, v, lane) neon_st1s_q16((__int16 *)(p), (v), (lane))
#define vld1_u32(p) neon_ld1m_32((const __int32 *)(p))
#define vst1_u32(p, v) neon_st1m_32((__int32 *)(p), (v))
#define vld1_dup_u32(p) neon_ld1r_32((const __int32 *)(p))
#define vld1_lane_u32(p, v, lane) neon_ld1s_32((const __int32 *)(p), (v), (lane))
#define vst1_lane_u32(p, v, lane) neon_st1s_32((__int32 *)(p), (v), (lane))
#define vld1q_u32(p) neon_ld1m_q32((const __int32 *)(p))
#define vst1q_u32(p, v) neon_st1m_q32((__int32 *)(p), (v))
#define vld1q_dup_u32(p) neon_ld1r_q32((const __int32 *)(p))
#define vld1q_lane_u32(p, v, lane) neon_ld1s_q32((const __int32 *)(p), (v), (lane))
#define vst1q_lane_u32(p, v, lane) neon_st1s_q32((__int32 *)(p), (v), (lane))
#define vld1_u64(p) neon_ld1m_64((const __int64 *)(p))
#define vst1_u64(p, v) neon_st1m_64((__int64 *)(p), (v))
#define vld1_dup_u64(p) neon_ld1r_64((const __int64 *)(p))
#define vld1_lane_u64(p, v, lane) neon_ld1s_64((const __int64 *)(p), (v), (lane))
#define vst1_lane_u64(p, v, lane) neon_st1s_64((__int64 *)(p), (v), (lane))
#define vld1q_u64(p) neon_ld1m_q64((const __int64 *)(p))
#define vst1q_u64(p, v) neon_st1m_q64((__int64 *)(p), (v))
#define vld1q_dup_u64(p) neon_ld1r_q64((const __int64 *)(p))
#define vld1q_lane_u64(p, v, lane) neon_ld1s_q64((const __int64 *)(p), (v), (lane))
#define vst1q_lane_u64(p, v, lane) neon_st1s_q64((__int64 *)(p), (v), (lane))
#define vld1_f32(p) neon_ld1m_32((const __int32 *)(p))
#define vst1_f32(p, v) neon_st1m_32((__int32 *)(p), (v))
#define vld1_dup_f32(p) neon_ld1r_32((const __int32 *)(p))
#define vld1_lane_f32(p, v, lane) neon_ld1s_32((const __int32 *)(p), (v), (lane))
#define vst1_lane_f32(p, v, lane) neon_st1s_32((__int32 *)(p), (v), (lane))
#define vld1q_f32(p) neon_ld1m_q32((const __int32 *)(p))
#define vst1q_f32(p, v) neon_st1m_q32((__int32 *)(p), (v))
#define vld1q_dup_f32(p) neon_ld1r_q32((const __int32 *)(p))
#define vld1q_lane_f32(p, v, lane) neon_ld1s_q32((const __int32 *)(p), (v), (lane))
#define vst1q_lane_f32(p, v, lane) neon_st1s_q32((__int32 *)(p), (v), (lane))
#define vld1_f64(p) neon_ld1m_64((const __int64 *)(p))
#define vst1_f64(p, v) neon_st1m_64((__int64 *)(p), (v))
#define vld1_dup_f64(p) neon_ld1r_64((const __int64 *)(p))
#define vld1_lane_f64(p, v, lane) neon_ld1s_64((const __int64 *)(p), (v), (lane))
#define vst1_lane_f64(p, v, lane) neon_st1s_64((__int64 *)(p), (v), (lane))
#define vld1q_f64(p) neon_ld1m_q64((const __int64 *)(p))
#define vst1q_f64(p, v) neon_st1m_q64((__int64 *)(p), (v))
#define vld1q_dup_f64(p) neon_ld1r_q64((const __int64 *)(p))
#define vld1q_lane_f64(p, v, lane) neon_ld1s_q64((const __int64 *)(p), (v), (lane))
#define vst1q_lane_f64(p, v, lane) neon_st1s_q64((__int64 *)(p), (v), (lane))
#define vld1_p8(p) neon_ld1m_8((const __int8 *)(p))
#define vst1_p8(p, v) neon_st1m_8((__int8 *)(p), (v))
#define vld1_dup_p8(p) neon_ld1r_8((const __int8 *)(p))
#define vld1_lane_p8(p, v, lane) neon_ld1s_8((const __int8 *)(p), (v), (lane))
#define vst1_lane_p8(p, v, lane) neon_st1s_8((__int8 *)(p), (v), (lane))
#define vld1q_p8(p) neon_ld1m_q8((const __int8 *)(p))
#define vst1q_p8(p, v) neon_st1m_q8((__int8 *)(p), (v))
#define vld1q_dup_p8(p) neon_ld1r_q8((const __int8 *)(p))
#define vld1q_lane_p8(p, v, lane) neon_ld1s_q8((const __int8 *)(p), (v), (lane))
#define vst1q_lane_p8(p, v, lane) neon_st1s_q8((__int8 *)(p), (v), (lane))
#define vld1_p16(p) neon_ld1m_16((const __int16 *)(p))
#define vst1_p16(p, v) neon_st1m_16((__int16 *)(p), (v))
#define vld1_dup_p16(p) neon_ld1r_16((const __int16 *)(p))
#define vld1_lane_p16(p, v, lane) neon_ld1s_16((const __int16 *)(p), (v), (lane))
#define vst1_lane_p16(p, v, lane) neon_st1s_16((__int16 *)(p), (v), (lane))
#define vld1q_p16(p) neon_ld1m_q16((const __int16 *)(p))
#define vst1q_p16(p, v) neon_st1m_q16((__int16 *)(p), (v))
#define vld1q_dup_p16(p) neon_ld1r_q16((const __int16 *)(p))
#define vld1q_lane_p16(p, v, lane) neon_ld1s_q16((const __int16 *)(p), (v), (lane))
#define vst1q_lane_p16(p, v, lane) neon_st1s_q16((__int16 *)(p), (v), (lane))
#define vld1_p64(p) neon_ld1m_64((const __int64 *)(p))
#define vst1_p64(p, v) neon_st1m_64((__int64 *)(p), (v))
#define vld1_dup_p64(p) neon_ld1r_64((const __int64 *)(p))
#define vld1_lane_p64(p, v, lane) neon_ld1s_64((const __int64 *)(p), (v), (lane))
#define vst1_lane_p64(p, v, lane) neon_st1s_64((__int64 *)(p), (v), (lane))
#define vld1q_p64(p) neon_ld1m_q64((const __int64 *)(p))
#define vst1q_p64(p, v) neon_st1m_q64((__int64 *)(p), (v))
#define vld1q_dup_p64(p) neon_ld1r_q64((const __int64 *)(p))
#define vld1q_lane_p64(p, v, lane) neon_ld1s_q64((const __int64 *)(p), (v), (lane))
#define vst1q_lane_p64(p, v, lane) neon_st1s_q64((__int64 *)(p), (v), (lane))
#define vadd_s8(a, b) neon_add8((a), (b))
#define vaddq_s8(a, b) neon_addq8((a), (b))
#define vadd_s16(a, b) neon_add16((a), (b))
#define vaddq_s16(a, b) neon_addq16((a), (b))
#define vadd_s32(a, b) neon_add32((a), (b))
#define vaddq_s32(a, b) neon_addq32((a), (b))
#define vadd_s64(a, b) neon_adds64((a), (b))
#define vaddq_s64(a, b) neon_addq64((a), (b))
#define vadd_u8(a, b) neon_add8((a), (b))
#define vaddq_u8(a, b) neon_addq8((a), (b))
#define vadd_u16(a, b) neon_add16((a), (b))
#define vaddq_u16(a, b) neon_addq16((a), (b))
#define vadd_u32(a, b) neon_add32((a), (b))
#define vaddq_u32(a, b) neon_addq32((a), (b))
#define vadd_u64(a, b) neon_adds64((a), (b))
#define vaddq_u64(a, b) neon_addq64((a), (b))
#define vadd_f32(a, b) neon_fadd32((a), (b))
#define vaddq_f32(a, b) neon_faddq32((a), (b))
#define vadd_f64(a, b) neon_fadd64((a), (b))
#define vaddq_f64(a, b) neon_faddq64((a), (b))
#define vsub_s8(a, b) neon_sub8((a), (b))
#define vsubq_s8(a, b) neon_subq8((a), (b))
#define vsub_s16(a, b) neon_sub16((a), (b))
#define vsubq_s16(a, b) neon_subq16((a), (b))
#define vsub_s32(a, b) neon_sub32((a), (b))
#define vsubq_s32(a, b) neon_subq32((a), (b))
#define vsub_s64(a, b) neon_subs64((a), (b))
#define vsubq_s64(a, b) neon_subq64((a), (b))
#define vsub_u8(a, b) neon_sub8((a), (b))
#define vsubq_u8(a, b) neon_subq8((a), (b))
#define vsub_u16(a, b) neon_sub16((a), (b))
#define vsubq_u16(a, b) neon_subq16((a), (b))
#define vsub_u32(a, b) neon_sub32((a), (b))
#define vsubq_u32(a, b) neon_subq32((a), (b))
#define vsub_u64(a, b) neon_subs64((a), (b))
#define vsubq_u64(a, b) neon_subq64((a), (b))
#define vsub_f32(a, b) neon_fsub32((a), (b))
#define vsubq_f32(a, b) neon_fsubq32((a), (b))
#define vsub_f64(a, b) neon_fsub64((a), (b))
#define vsubq_f64(a, b) neon_fsubq64((a), (b))
#define vmul_s8(a, b) neon_mul8((a), (b))
#define vmulq_s8(a, b) neon_mulq8((a), (b))
#define vmul_s16(a, b) neon_mul16((a), (b))
#define vmulq_s16(a, b) neon_mulq16((a), (b))
#define vmul_s32(a, b) neon_mul32((a), (b))
#define vmulq_s32(a, b) neon_mulq32((a), (b))
#define vmul_u8(a, b) neon_mul8((a), (b))
#define vmulq_u8(a, b) neon_mulq8((a), (b))
#define vmul_u16(a, b) neon_mul16((a), (b))
#define vmulq_u16(a, b) neon_mulq16((a), (b))
#define vmul_u32(a, b) neon_mul32((a), (b))
#define vmulq_u32(a, b) neon_mulq32((a), (b))
#define vmul_f32(a, b) neon_fmul32((a), (b))
#define vmulq_f32(a, b) neon_fmulq32((a), (b))
#define vmul_f64(a, b) neon_fmul64((a), (b))
#define vmulq_f64(a, b) neon_fmulq64((a), (b))
#define vmla_s8(a, b, c) neon_mla8((a), (b), (c))
#define vmlaq_s8(a, b, c) neon_mlaq8((a), (b), (c))
#define vmla_s16(a, b, c) neon_mla16((a), (b), (c))
#define vmlaq_s16(a, b, c) neon_mlaq16((a), (b), (c))
#define vmla_s32(a, b, c) neon_mla32((a), (b), (c))
#define vmlaq_s32(a, b, c) neon_mlaq32((a), (b), (c))
#define vmla_u8(a, b, c) neon_mla8((a), (b), (c))
#define vmlaq_u8(a, b, c) neon_mlaq8((a), (b), (c))
#define vmla_u16(a, b, c) neon_mla16((a), (b), (c))
#define vmlaq_u16(a, b, c) neon_mlaq16((a), (b), (c))
#define vmla_u32(a, b, c) neon_mla32((a), (b), (c))
#define vmlaq_u32(a, b, c) neon_mlaq32((a), (b), (c))
#define vmls_s8(a, b, c) neon_mls8((a), (b), (c))
#define vmlsq_s8(a, b, c) neon_mlsq8((a), (b), (c))
#define vmls_s16(a, b, c) neon_mls16((a), (b), (c))
#define vmlsq_s16(a, b, c) neon_mlsq16((a), (b), (c))
#define vmls_s32(a, b, c) neon_mls32((a), (b), (c))
#define vmlsq_s32(a, b, c) neon_mlsq32((a), (b), (c))
#define vmls_u8(a, b, c) neon_mls8((a), (b), (c))
#define vmlsq_u8(a, b, c) neon_mlsq8((a), (b), (c))
#define vmls_u16(a, b, c) neon_mls16((a), (b), (c))
#define vmlsq_u16(a, b, c) neon_mlsq16((a), (b), (c))
#define vmls_u32(a, b, c) neon_mls32((a), (b), (c))
#define vmlsq_u32(a, b, c) neon_mlsq32((a), (b), (c))
#define vmull_s8(a, b) neon_smull_8((a), (b))
#define vmull_high_s8(a, b) neon_smull2_8((a), (b))
#define vmull_s16(a, b) neon_smull_16((a), (b))
#define vmull_high_s16(a, b) neon_smull2_16((a), (b))
#define vmull_s32(a, b) neon_smull_32((a), (b))
#define vmull_high_s32(a, b) neon_smull2_32((a), (b))
#define vmull_u8(a, b) neon_umull_8((a), (b))
#define vmull_high_u8(a, b) neon_umull2_8((a), (b))
#define vmull_u16(a, b) neon_umull_16((a), (b))
#define vmull_high_u16(a, b) neon_umull2_16((a), (b))
#define vmull_u32(a, b) neon_umull_32((a), (b))
#define vmull_high_u32(a, b) neon_umull2_32((a), (b))
#define vmull_p8(a, b) neon_pmull_8((a), (b))
#define vmull_high_p8(a, b) neon_pmull2_8((a), (b))
#define vmull_p64(a, b) neon_pmull_64((a), (b))
#define vmull_high_p64(a, b) neon_pmull2_64((a), (b))
#define vmlal_s8(a, b, c) neon_smlal_8((a), (b), (c))
#define vmlal_high_s8(a, b, c) neon_smlal2_8((a), (b), (c))
#define vmlal_s16(a, b, c) neon_smlal_16((a), (b), (c))
#define vmlal_high_s16(a, b, c) neon_smlal2_16((a), (b), (c))
#define vmlal_s32(a, b, c) neon_smlal_32((a), (b), (c))
#define vmlal_high_s32(a, b, c) neon_smlal2_32((a), (b), (c))
#define vmlal_u8(a, b, c) neon_umlal_8((a), (b), (c))
#define vmlal_high_u8(a, b, c) neon_umlal2_8((a), (b), (c))
#define vmlal_u16(a, b, c) neon_umlal_16((a), (b), (c))
#define vmlal_high_u16(a, b, c) neon_umlal2_16((a), (b), (c))
#define vmlal_u32(a, b, c) neon_umlal_32((a), (b), (c))
#define vmlal_high_u32(a, b, c) neon_umlal2_32((a), (b), (c))
#define vmlsl_s8(a, b, c) neon_smlsl_8((a), (b), (c))
#define vmlsl_high_s8(a, b, c) neon_smlsl2_8((a), (b), (c))
#define vmlsl_s16(a, b, c) neon_smlsl_16((a), (b), (c))
#define vmlsl_high_s16(a, b, c) neon_smlsl2_16((a), (b), (c))
#define vmlsl_s32(a, b, c) neon_smlsl_32((a), (b), (c))
#define vmlsl_high_s32(a, b, c) neon_smlsl2_32((a), (b), (c))
#define vmlsl_u8(a, b, c) neon_umlsl_8((a), (b), (c))
#define vmlsl_high_u8(a, b, c) neon_umlsl2_8((a), (b), (c))
#define vmlsl_u16(a, b, c) neon_umlsl_16((a), (b), (c))
#define vmlsl_high_u16(a, b, c) neon_umlsl2_16((a), (b), (c))
#define vmlsl_u32(a, b, c) neon_umlsl_32((a), (b), (c))
#define vmlsl_high_u32(a, b, c) neon_umlsl2_32((a), (b), (c))
#define vand_s8(a, b) neon_and((a), (b))
#define vandq_s8(a, b) neon_andq((a), (b))
#define vand_s16(a, b) neon_and((a), (b))
#define vandq_s16(a, b) neon_andq((a), (b))
#define vand_s32(a, b) neon_and((a), (b))
#define vandq_s32(a, b) neon_andq((a), (b))
#define vand_s64(a, b) neon_and((a), (b))
#define vandq_s64(a, b) neon_andq((a), (b))
#define vand_u8(a, b) neon_and((a), (b))
#define vandq_u8(a, b) neon_andq((a), (b))
#define vand_u16(a, b) neon_and((a), (b))
#define vandq_u16(a, b) neon_andq((a), (b))
#define vand_u32(a, b) neon_and((a), (b))
#define vandq_u32(a, b) neon_andq((a), (b))
#define vand_u64(a, b) neon_and((a), (b))
#define vandq_u64(a, b) neon_andq((a), (b))
#define vorr_s8(a, b) neon_orr((a), (b))
#define vorrq_s8(a, b) neon_orrq((a), (b))
#define vorr_s16(a, b) neon_orr((a), (b))
#define vorrq_s16(a, b) neon_orrq((a), (b))
#define vorr_s32(a, b) neon_orr((a), (b))
#define vorrq_s32(a, b) neon_orrq((a), (b))
#define vorr_s64(a, b) neon_orr((a), (b))
#define vorrq_s64(a, b) neon_orrq((a), (b))
#define vorr_u8(a, b) neon_orr((a), (b))
#define vorrq_u8(a, b) neon_orrq((a), (b))
#define vorr_u16(a, b) neon_orr((a), (b))
#define vorrq_u16(a, b) neon_orrq((a), (b))
#define vorr_u32(a, b) neon_orr((a), (b))
#define vorrq_u32(a, b) neon_orrq((a), (b))
#define vorr_u64(a, b) neon_orr((a), (b))
#define vorrq_u64(a, b) neon_orrq((a), (b))
#define veor_s8(a, b) neon_eor((a), (b))
#define veorq_s8(a, b) neon_eorq((a), (b))
#define veor_s16(a, b) neon_eor((a), (b))
#define veorq_s16(a, b) neon_eorq((a), (b))
#define veor_s32(a, b) neon_eor((a), (b))
#define veorq_s32(a, b) neon_eorq((a), (b))
#define veor_s64(a, b) neon_eor((a), (b))
#define veorq_s64(a, b) neon_eorq((a), (b))
#define veor_u8(a, b) neon_eor((a), (b))
#define veorq_u8(a, b) neon_eorq((a), (b))
#define veor_u16(a, b) neon_eor((a), (b))
#define veorq_u16(a, b) neon_eorq((a), (b))
#define veor_u32(a, b) neon_eor((a), (b))
#define veorq_u32(a, b) neon_eorq((a), (b))
#define veor_u64(a, b) neon_eor((a), (b))
#define veorq_u64(a, b) neon_eorq((a), (b))
#define vbic_s8(a, b) neon_bic((a), (b))
#define vbicq_s8(a, b) neon_bicq((a), (b))
#define vbic_s16(a, b) neon_bic((a), (b))
#define vbicq_s16(a, b) neon_bicq((a), (b))
#define vbic_s32(a, b) neon_bic((a), (b))
#define vbicq_s32(a, b) neon_bicq((a), (b))
#define vbic_s64(a, b) neon_bic((a), (b))
#define vbicq_s64(a, b) neon_bicq((a), (b))
#define vbic_u8(a, b) neon_bic((a), (b))
#define vbicq_u8(a, b) neon_bicq((a), (b))
#define vbic_u16(a, b) neon_bic((a), (b))
#define vbicq_u16(a, b) neon_bicq((a), (b))
#define vbic_u32(a, b) neon_bic((a), (b))
#define vbicq_u32(a, b) neon_bicq((a), (b))
#define vbic_u64(a, b) neon_bic((a), (b))
#define vbicq_u64(a, b) neon_bicq((a), (b))
#define vorn_s8(a, b) neon_orn((a), (b))
#define vornq_s8(a, b) neon_ornq((a), (b))
#define vorn_s16(a, b) neon_orn((a), (b))
#define vornq_s16(a, b) neon_ornq((a), (b))
#define vorn_s32(a, b) neon_orn((a), (b))
#define vornq_s32(a, b) neon_ornq((a), (b))
#define vorn_s64(a, b) neon_orn((a), (b))
#define vornq_s64(a, b) neon_ornq((a), (b))
#define vorn_u8(a, b) neon_orn((a), (b))
#define vornq_u8(a, b) neon_ornq((a), (b))
#define vorn_u16(a, b) neon_orn((a), (b))
#define vornq_u16(a, b) neon_ornq((a), (b))
#define vorn_u32(a, b) neon_orn((a), (b))
#define vornq_u32(a, b) neon_ornq((a), (b))
#define vorn_u64(a, b) neon_orn((a), (b))
#define vornq_u64(a, b) neon_ornq((a), (b))
#define vmvn_s8(a) neon_not((a))
#define vmvnq_s8(a) neon_notq((a))
#define vmvn_s16(a) neon_not((a))
#define vmvnq_s16(a) neon_notq((a))
#define vmvn_s32(a) neon_not((a))
#define vmvnq_s32(a) neon_notq((a))
#define vmvn_u8(a) neon_not((a))
#define vmvnq_u8(a) neon_notq((a))
#define vmvn_u16(a) neon_not((a))
#define vmvnq_u16(a) neon_notq((a))
#define vmvn_u32(a) neon_not((a))
#define vmvnq_u32(a) neon_notq((a))
#define vmvn_p8(a) neon_not((a))
#define vmvnq_p8(a) neon_notq((a))
#define vceq_s8(a, b) neon_cmeq8((a), (b))
#define vceqq_s8(a, b) neon_cmeqq8((a), (b))
#define vceq_s16(a, b) neon_cmeq16((a), (b))
#define vceqq_s16(a, b) neon_cmeqq16((a), (b))
#define vceq_s32(a, b) neon_cmeq32((a), (b))
#define vceqq_s32(a, b) neon_cmeqq32((a), (b))
#define vceq_s64(a, b) neon_cmeq64((a), (b))
#define vceqq_s64(a, b) neon_cmeqq64((a), (b))
#define vceq_u8(a, b) neon_cmeq8((a), (b))
#define vceqq_u8(a, b) neon_cmeqq8((a), (b))
#define vceq_u16(a, b) neon_cmeq16((a), (b))
#define vceqq_u16(a, b) neon_cmeqq16((a), (b))
#define vceq_u32(a, b) neon_cmeq32((a), (b))
#define vceqq_u32(a, b) neon_cmeqq32((a), (b))
#define vceq_u64(a, b) neon_cmeq64((a), (b))
#define vceqq_u64(a, b) neon_cmeqq64((a), (b))
#define vceq_f32(a, b) neon_fcmeq32((a), (b))
#define vceqq_f32(a, b) neon_fcmeqq32((a), (b))
#define vceq_f64(a, b) neon_fcmeq64((a), (b))
#define vceqq_f64(a, b) neon_fcmeqq64((a), (b))
#define vcgt_s8(a, b) neon_cmgt8((a), (b))
#define vcgtq_s8(a, b) neon_cmgtq8((a), (b))
#define vcgt_s16(a, b) neon_cmgt16((a), (b))
#define vcgtq_s16(a, b) neon_cmgtq16((a), (b))
#define vcgt_s32(a, b) neon_cmgt32((a), (b))
#define vcgtq_s32(a, b) neon_cmgtq32((a), (b))
#define vcgt_s64(a, b) neon_cmgt64((a), (b))
#define vcgtq_s64(a, b) neon_cmgtq64((a), (b))
#define vcgt_u8(a, b) neon_cmhi8((a), (b))
#define vcgtq_u8(a, b) neon_cmhiq8((a), (b))
#define vcgt_u16(a, b) neon_cmhi16((a), (b))
#define vcgtq_u16(a, b) neon_cmhiq16((a), (b))
#define vcgt_u32(a, b) neon_cmhi32((a), (b))
#define vcgtq_u32(a, b) neon_cmhiq32((a), (b))
#define vcgt_u64(a, b) neon_cmhi64((a), (b))
#define vcgtq_u64(a, b) neon_cmhiq64((a), (b))
#define vcgt_f32(a, b) neon_fcmgt32((a), (b))
#define vcgtq_f32(a, b) neon_fcmgtq32((a), (b))
#define vcgt_f64(a, b) neon_fcmgt64((a), (b))
#define vcgtq_f64(a, b) neon_fcmgtq64((a), (b))
#define vcge_s8(a, b) neon_cmge8((a), (b))
#define vcgeq_s8(a, b) neon_cmgeq8((a), (b))
#define vcge_s16(a, b) neon_cmge16((a), (b))
#define vcgeq_s16(a, b) neon_cmgeq16((a), (b))
#define vcge_s32(a, b) neon_cmge32((a), (b))
#define vcgeq_s32(a, b) neon_cmgeq32((a), (b))
#define vcge_s64(a, b) neon_cmge64((a), (b))
#define vcgeq_s64(a, b) neon_cmgeq64((a), (b))
#define vcge_u8(a, b) neon_cmhs8((a), (b))
#define vcgeq_u8(a, b) neon_cmhsq8((a), (b))
#define vcge_u16(a, b) neon_cmhs16((a), (b))
#define vcgeq_u16(a, b) neon_cmhsq16((a), (b))
#define vcge_u32(a, b) neon_cmhs32((a), (b))
#define vcgeq_u32(a, b) neon_cmhsq32((a), (b))
#define vcge_u64(a, b) neon_cmhs64((a), (b))
#define vcgeq_u64(a, b) neon_cmhsq64((a), (b))
#define vcge_f32(a, b) neon_fcmge32((a), (b))
#define vcgeq_f32(a, b) neon_fcmgeq32((a), (b))
#define vcge_f64(a, b) neon_fcmge64((a), (b))
#define vcgeq_f64(a, b) neon_fcmgeq64((a), (b))
#define vclt_s8(a, b) neon_cmgt8((b), (a))
#define vcltq_s8(a, b) neon_cmgtq8((b), (a))
#define vclt_s16(a, b) neon_cmgt16((b), (a))
#define vcltq_s16(a, b) neon_cmgtq16((b), (a))
#define vclt_s32(a, b) neon_cmgt32((b), (a))
#define vcltq_s32(a, b) neon_cmgtq32((b), (a))
#define vclt_s64(a, b) neon_cmgt64((b), (a))
#define vcltq_s64(a, b) neon_cmgtq64((b), (a))
#define vclt_u8(a, b) neon_cmhi8((b), (a))
#define vcltq_u8(a, b) neon_cmhiq8((b), (a))
#define vclt_u16(a, b) neon_cmhi16((b), (a))
#define vcltq_u16(a, b) neon_cmhiq16((b), (a))
#define vclt_u32(a, b) neon_cmhi32((b), (a))
#define vcltq_u32(a, b) neon_cmhiq32((b), (a))
#define vclt_u64(a, b) neon_cmhi64((b), (a))
#define vcltq_u64(a, b) neon_cmhiq64((b), (a))
#define vclt_f32(a, b) neon_fcmgt32((b), (a))
#define vcltq_f32(a, b) neon_fcmgtq32((b), (a))
#define vclt_f64(a, b) neon_fcmgt64((b), (a))
#define vcltq_f64(a, b) neon_fcmgtq64((b), (a))
#define vcle_s8(a, b) neon_cmge8((b), (a))
#define vcleq_s8(a, b) neon_cmgeq8((b), (a))
#define vcle_s16(a, b) neon_cmge16((b), (a))
#define vcleq_s16(a, b) neon_cmgeq16((b), (a))
#define vcle_s32(a, b) neon_cmge32((b), (a))
#define vcleq_s32(a, b) neon_cmgeq32((b), (a))
#define vcle_s64(a, b) neon_cmge64((b), (a))
#define vcleq_s64(a, b) neon_cmgeq64((b), (a))
#define vcle_u8(a, b) neon_cmhs8((b), (a))
#define vcleq_u8(a, b) neon_cmhsq8((b), (a))
#define vcle_u16(a, b) neon_cmhs16((b), (a))
#define vcleq_u16(a, b) neon_cmhsq16((b), (a))
#define vcle_u32(a, b) neon_cmhs32((b), (a))
#define vcleq_u32(a, b) neon_cmhsq32((b), (a))
#define vcle_u64(a, b) neon_cmhs64((b), (a))
#define vcleq_u64(a, b) neon_cmhsq64((b), (a))
#define vcle_f32(a, b) neon_fcmge32((b), (a))
#define vcleq_f32(a, b) neon_fcmgeq32((b), (a))
#define vcle_f64(a, b) neon_fcmge64((b), (a))
#define vcleq_f64(a, b) neon_fcmgeq64((b), (a))
#define vceqz_s8(a) neon_cmeqz8((a))
#define vceqzq_s8(a) neon_cmeqzq8((a))
#define vceqz_s16(a) neon_cmeqz16((a))
#define vceqzq_s16(a) neon_cmeqzq16((a))
#define vceqz_s32(a) neon_cmeqz32((a))
#define vceqzq_s32(a) neon_cmeqzq32((a))
#define vceqz_s64(a) neon_cmeqz64((a))
#define vceqzq_s64(a) neon_cmeqzq64((a))
#define vceqz_f32(a) neon_fcmeqz32((a))
#define vceqzq_f32(a) neon_fcmeqzq32((a))
#define vceqz_f64(a) neon_fcmeqz64((a))
#define vceqzq_f64(a) neon_fcmeqzq64((a))
#define vceqz_u8(a) neon_cmeqz8((a))
#define vceqzq_u8(a) neon_cmeqzq8((a))
#define vceqz_u16(a) neon_cmeqz16((a))
#define vceqzq_u16(a) neon_cmeqzq16((a))
#define vceqz_u32(a) neon_cmeqz32((a))
#define vceqzq_u32(a) neon_cmeqzq32((a))
#define vceqz_u64(a) neon_cmeqz64((a))
#define vceqzq_u64(a) neon_cmeqzq64((a))
#define vcgtz_s8(a) neon_cmgtz8((a))
#define vcgtzq_s8(a) neon_cmgtzq8((a))
#define vcgtz_s16(a) neon_cmgtz16((a))
#define vcgtzq_s16(a) neon_cmgtzq16((a))
#define vcgtz_s32(a) neon_cmgtz32((a))
#define vcgtzq_s32(a) neon_cmgtzq32((a))
#define vcgtz_s64(a) neon_cmgtz64((a))
#define vcgtzq_s64(a) neon_cmgtzq64((a))
#define vcgtz_f32(a) neon_fcmgtz32((a))
#define vcgtzq_f32(a) neon_fcmgtzq32((a))
#define vcgtz_f64(a) neon_fcmgtz64((a))
#define vcgtzq_f64(a) neon_fcmgtzq64((a))
#define vcgez_s8(a) neon_cmgez8((a))
#define vcgezq_s8(a) neon_cmgezq8((a))
#define vcgez_s16(a) neon_cmgez16((a))
#define vcgezq_s16(a) neon_cmgezq16((a))
#define vcgez_s32(a) neon_cmgez32((a))
#define vcgezq_s32(a) neon_cmgezq32((a))
#define vcgez_s64(a) neon_cmgez64((a))
#define vcgezq_s64(a) neon_cmgezq64((a))
#define vcgez_f32(a) neon_fcmgez32((a))
#define vcgezq_f32(a) neon_fcmgezq32((a))
#define vcgez_f64(a) neon_fcmgez64((a))
#define vcgezq_f64(a) neon_fcmgezq64((a))
#define vcltz_s8(a) neon_cmltz8((a))
#define vcltzq_s8(a) neon_cmltzq8((a))
#define vcltz_s16(a) neon_cmltz16((a))
#define vcltzq_s16(a) neon_cmltzq16((a))
#define vcltz_s32(a) neon_cmltz32((a))
#define vcltzq_s32(a) neon_cmltzq32((a))
#define vcltz_s64(a) neon_cmltz64((a))
#define vcltzq_s64(a) neon_cmltzq64((a))
#define vcltz_f32(a) neon_fcmltz32((a))
#define vcltzq_f32(a) neon_fcmltzq32((a))
#define vcltz_f64(a) neon_fcmltz64((a))
#define vcltzq_f64(a) neon_fcmltzq64((a))
#define vclez_s8(a) neon_cmlez8((a))
#define vclezq_s8(a) neon_cmlezq8((a))
#define vclez_s16(a) neon_cmlez16((a))
#define vclezq_s16(a) neon_cmlezq16((a))
#define vclez_s32(a) neon_cmlez32((a))
#define vclezq_s32(a) neon_cmlezq32((a))
#define vclez_s64(a) neon_cmlez64((a))
#define vclezq_s64(a) neon_cmlezq64((a))
#define vclez_f32(a) neon_fcmlez32((a))
#define vclezq_f32(a) neon_fcmlezq32((a))
#define vclez_f64(a) neon_fcmlez64((a))
#define vclezq_f64(a) neon_fcmlezq64((a))
#define vshl_n_s8(a, n) neon_shli8((a), (n))
#define vshr_n_s8(a, n) neon_sshri8((a), (n))
#define vshlq_n_s8(a, n) neon_shliq8((a), (n))
#define vshrq_n_s8(a, n) neon_sshriq8((a), (n))
#define vshl_n_s16(a, n) neon_shli16((a), (n))
#define vshr_n_s16(a, n) neon_sshri16((a), (n))
#define vshlq_n_s16(a, n) neon_shliq16((a), (n))
#define vshrq_n_s16(a, n) neon_sshriq16((a), (n))
#define vshl_n_s32(a, n) neon_shli32((a), (n))
#define vshr_n_s32(a, n) neon_sshri32((a), (n))
#define vshlq_n_s32(a, n) neon_shliq32((a), (n))
#define vshrq_n_s32(a, n) neon_sshriq32((a), (n))
#define vshl_n_s64(a, n) neon_shlis64((a), (n))
#define vshr_n_s64(a, n) neon_sshris64((a), (n))
#define vshlq_n_s64(a, n) neon_shliq64((a), (n))
#define vshrq_n_s64(a, n) neon_sshriq64((a), (n))
#define vshl_n_u8(a, n) neon_shli8((a), (n))
#define vshr_n_u8(a, n) neon_ushri8((a), (n))
#define vshlq_n_u8(a, n) neon_shliq8((a), (n))
#define vshrq_n_u8(a, n) neon_ushriq8((a), (n))
#define vshl_n_u16(a, n) neon_shli16((a), (n))
#define vshr_n_u16(a, n) neon_ushri16((a), (n))
#define vshlq_n_u16(a, n) neon_shliq16((a), (n))
#define vshrq_n_u16(a, n) neon_ushriq16((a), (n))
#define vshl_n_u32(a, n) neon_shli32((a), (n))
#define vshr_n_u32(a, n) neon_ushri32((a), (n))
#define vshlq_n_u32(a, n) neon_shliq32((a), (n))
#define vshrq_n_u32(a, n) neon_ushriq32((a), (n))
#define vshl_n_u64(a, n) neon_shlis64((a), (n))
#define vshr_n_u64(a, n) neon_ushris64((a), (n))
#define vshlq_n_u64(a, n) neon_shliq64((a), (n))
#define vshrq_n_u64(a, n) neon_ushriq64((a), (n))
#define vdup_n_s8(x) neon_dupr8((x))
#define vmov_n_s8(x) neon_dupr8((x))
#define vdupq_n_s8(x) neon_dupqr8((x))
#define vmovq_n_s8(x) neon_dupqr8((x))
#define vget_low_s8(a) neon_dups64q((a), 0)
#define vget_high_s8(a) neon_dups64q((a), 1)
#define vcombine_s8(lo, hi) neon_combine((lo), (hi))
#define vget_lane_s8(a, lane) neon_smov8((a), (lane))
#define vset_lane_s8(x, a, lane) neon_insr8((a), (lane), (x))
#define vext_s8(a, b, n) neon_ext8((a), (b), (n))
#define vgetq_lane_s8(a, lane) neon_smovq8((a), (lane))
#define vsetq_lane_s8(x, a, lane) neon_insqr8((a), (lane), (x))
#define vextq_s8(a, b, n) neon_extq8((a), (b), (n))
#define vdup_n_s16(x) neon_dupr16((x))
#define vmov_n_s16(x) neon_dupr16((x))
#define vdupq_n_s16(x) neon_dupqr16((x))
#define vmovq_n_s16(x) neon_dupqr16((x))
#define vget_low_s16(a) neon_dups64q((a), 0)
#define vget_high_s16(a) neon_dups64q((a), 1)
#define vcombine_s16(lo, hi) neon_combine((lo), (hi))
#define vget_lane_s16(a, lane) neon_smov16((a), (lane))
#define vset_lane_s16(x, a, lane) neon_insr16((a), (lane), (x))
#define vext_s16(a, b, n) neon_ext16((a), (b), (n))
#define vgetq_lane_s16(a, lane) neon_smovq16((a), (lane))
#define vsetq_lane_s16(x, a, lane) neon_insqr16((a), (lane), (x))
#define vextq_s16(a, b, n) neon_extq16((a), (b), (n))
#define vdup_n_s32(x) neon_dupr32((x))
#define vmov_n_s32(x) neon_dupr32((x))
#define vdupq_n_s32(x) neon_dupqr32((x))
#define vmovq_n_s32(x) neon_dupqr32((x))
#define vget_low_s32(a) neon_dups64q((a), 0)
#define vget_high_s32(a) neon_dups64q((a), 1)
#define vcombine_s32(lo, hi) neon_combine((lo), (hi))
#define vget_lane_s32(a, lane) neon_smov32((a), (lane))
#define vset_lane_s32(x, a, lane) neon_insr32((a), (lane), (x))
#define vext_s32(a, b, n) neon_ext32((a), (b), (n))
#define vgetq_lane_s32(a, lane) neon_smovq32((a), (lane))
#define vsetq_lane_s32(x, a, lane) neon_insqr32((a), (lane), (x))
#define vextq_s32(a, b, n) neon_extq32((a), (b), (n))
#define vdup_n_s64(x) neon_dupr64((x))
#define vmov_n_s64(x) neon_dupr64((x))
#define vdupq_n_s64(x) neon_dupqr64((x))
#define vmovq_n_s64(x) neon_dupqr64((x))
#define vget_low_s64(a) neon_dups64q((a), 0)
#define vget_high_s64(a) neon_dups64q((a), 1)
#define vcombine_s64(lo, hi) neon_combine((lo), (hi))
#define vget_lane_s64(a, lane) neon_smov64((a), (lane))
#define vset_lane_s64(x, a, lane) neon_insr64((a), (lane), (x))
#define vext_s64(a, b, n) neon_ext64((a), (b), (n))
#define vgetq_lane_s64(a, lane) neon_smovq64((a), (lane))
#define vsetq_lane_s64(x, a, lane) neon_insqr64((a), (lane), (x))
#define vextq_s64(a, b, n) neon_extq64((a), (b), (n))
#define vdup_n_u8(x) neon_dupr8((x))
#define vmov_n_u8(x) neon_dupr8((x))
#define vdupq_n_u8(x) neon_dupqr8((x))
#define vmovq_n_u8(x) neon_dupqr8((x))
#define vget_low_u8(a) neon_dups64q((a), 0)
#define vget_high_u8(a) neon_dups64q((a), 1)
#define vcombine_u8(lo, hi) neon_combine((lo), (hi))
#define vget_lane_u8(a, lane) neon_umov8((a), (lane))
#define vset_lane_u8(x, a, lane) neon_insr8((a), (lane), (x))
#define vext_u8(a, b, n) neon_ext8((a), (b), (n))
#define vgetq_lane_u8(a, lane) neon_umovq8((a), (lane))
#define vsetq_lane_u8(x, a, lane) neon_insqr8((a), (lane), (x))
#define vextq_u8(a, b, n) neon_extq8((a), (b), (n))
#define vdup_n_u16(x) neon_dupr16((x))
#define vmov_n_u16(x) neon_dupr16((x))
#define vdupq_n_u16(x) neon_dupqr16((x))
#define vmovq_n_u16(x) neon_dupqr16((x))
#define vget_low_u16(a) neon_dups64q((a), 0)
#define vget_high_u16(a) neon_dups64q((a), 1)
#define vcombine_u16(lo, hi) neon_combine((lo), (hi))
#define vget_lane_u16(a, lane) neon_umov16((a), (lane))
#define vset_lane_u16(x, a, lane) neon_insr16((a), (lane), (x))
#define vext_u16(a, b, n) neon_ext16((a), (b), (n))
#define vgetq_lane_u16(a, lane) neon_umovq16((a), (lane))
#define vsetq_lane_u16(x, a, lane) neon_insqr16((a), (lane), (x))
#define vextq_u16(a, b, n) neon_extq16((a), (b), (n))
#define vdup_n_u32(x) neon_dupr32((x))
#define vmov_n_u32(x) neon_dupr32((x))
#define vdupq_n_u32(x) neon_dupqr32((x))
#define vmovq_n_u32(x) neon_dupqr32((x))
#define vget_low_u32(a) neon_dups64q((a), 0)
#define vget_high_u32(a) neon_dups64q((a), 1)
#define vcombine_u32(lo, hi) neon_combine((lo), (hi))
#define vget_lane_u32(a, lane) neon_umov32((a), (lane))
#define vset_lane_u32(x, a, lane) neon_insr32((a), (lane), (x))
#define vext_u32(a, b, n) neon_ext32((a), (b), (n))
#define vgetq_lane_u32(a, lane) neon_umovq32((a), (lane))
#define vsetq_lane_u32(x, a, lane) neon_insqr32((a), (lane), (x))
#define vextq_u32(a, b, n) neon_extq32((a), (b), (n))
#define vdup_n_u64(x) neon_dupr64((x))
#define vmov_n_u64(x) neon_dupr64((x))
#define vdupq_n_u64(x) neon_dupqr64((x))
#define vmovq_n_u64(x) neon_dupqr64((x))
#define vget_low_u64(a) neon_dups64q((a), 0)
#define vget_high_u64(a) neon_dups64q((a), 1)
#define vcombine_u64(lo, hi) neon_combine((lo), (hi))
#define vget_lane_u64(a, lane) neon_umov64((a), (lane))
#define vset_lane_u64(x, a, lane) neon_insr64((a), (lane), (x))
#define vext_u64(a, b, n) neon_ext64((a), (b), (n))
#define vgetq_lane_u64(a, lane) neon_umovq64((a), (lane))
#define vsetq_lane_u64(x, a, lane) neon_insqr64((a), (lane), (x))
#define vextq_u64(a, b, n) neon_extq64((a), (b), (n))
#define vdup_n_f32(x) neon_duprf32((x))
#define vmov_n_f32(x) neon_duprf32((x))
#define vdupq_n_f32(x) neon_dupqrf32((x))
#define vmovq_n_f32(x) neon_dupqrf32((x))
#define vget_low_f32(a) neon_dups64q((a), 0)
#define vget_high_f32(a) neon_dups64q((a), 1)
#define vcombine_f32(lo, hi) neon_combine((lo), (hi))
#define vget_lane_f32(a, lane) neon_dups32((a), (lane))
#define vset_lane_f32(x, a, lane) neon_insrf32((a), (lane), (x))
#define vext_f32(a, b, n) neon_ext32((a), (b), (n))
#define vgetq_lane_f32(a, lane) neon_dups32q((a), (lane))
#define vsetq_lane_f32(x, a, lane) neon_insqrf32((a), (lane), (x))
#define vextq_f32(a, b, n) neon_extq32((a), (b), (n))
#define vdup_n_f64(x) neon_duprf64((x))
#define vmov_n_f64(x) neon_duprf64((x))
#define vdupq_n_f64(x) neon_dupqrf64((x))
#define vmovq_n_f64(x) neon_dupqrf64((x))
#define vget_low_f64(a) neon_dups64q((a), 0)
#define vget_high_f64(a) neon_dups64q((a), 1)
#define vcombine_f64(lo, hi) neon_combine((lo), (hi))
#define vget_lane_f64(a, lane) neon_dups64((a), (lane)).n64_f64[0]
#define vset_lane_f64(x, a, lane) neon_insrf64((a), (lane), (x))
#define vext_f64(a, b, n) neon_ext64((a), (b), (n))
#define vgetq_lane_f64(a, lane) neon_dups64q((a), (lane)).n64_f64[0]
#define vsetq_lane_f64(x, a, lane) neon_insqrf64((a), (lane), (x))
#define vextq_f64(a, b, n) neon_extq64((a), (b), (n))
#define vdup_n_p8(x) neon_dupr8((x))
#define vmov_n_p8(x) neon_dupr8((x))
#define vdupq_n_p8(x) neon_dupqr8((x))
#define vmovq_n_p8(x) neon_dupqr8((x))
#define vget_low_p8(a) neon_dups64q((a), 0)
#define vget_high_p8(a) neon_dups64q((a), 1)
#define vcombine_p8(lo, hi) neon_combine((lo), (hi))
#define vget_lane_p8(a, lane) neon_umov8((a), (lane))
#define vset_lane_p8(x, a, lane) neon_insr8((a), (lane), (x))
#define vext_p8(a, b, n) neon_ext8((a), (b), (n))
#define vgetq_lane_p8(a, lane) neon_umovq8((a), (lane))
#define vsetq_lane_p8(x, a, lane) neon_insqr8((a), (lane), (x))
#define vextq_p8(a, b, n) neon_extq8((a), (b), (n))
#define vdup_n_p16(x) neon_dupr16((x))
#define vmov_n_p16(x) neon_dupr16((x))
#define vdupq_n_p16(x) neon_dupqr16((x))
#define vmovq_n_p16(x) neon_dupqr16((x))
#define vget_low_p16(a) neon_dups64q((a), 0)
#define vget_high_p16(a) neon_dups64q((a), 1)
#define vcombine_p16(lo, hi) neon_combine((lo), (hi))
#define vget_lane_p16(a, lane) neon_umov16((a), (lane))
#define vset_lane_p16(x, a, lane) neon_insr16((a), (lane), (x))
#define vext_p16(a, b, n) neon_ext16((a), (b), (n))
#define vgetq_lane_p16(a, lane) neon_umovq16((a), (lane))
#define vsetq_lane_p16(x, a, lane) neon_insqr16((a), (lane), (x))
#define vextq_p16(a, b, n) neon_extq16((a), (b), (n))
#define vdup_n_p64(x) neon_dupr64((x))
#define vdupq_n_p64(x) neon_dupqr64((x))
#define vget_low_p64(a) neon_dups64q((a), 0)
#define vget_high_p64(a) neon_dups64q((a), 1)
#define vcombine_p64(lo, hi) neon_combine((lo), (hi))
#define vget_lane_p64(a, lane) neon_umov64((a), (lane))
#define vset_lane_p64(x, a, lane) neon_insr64((a), (lane), (x))
#define vext_p64(a, b, n) neon_ext64((a), (b), (n))
#define vgetq_lane_p64(a, lane) neon_umovq64((a), (lane))
#define vsetq_lane_p64(x, a, lane) neon_insqr64((a), (lane), (x))
#define vextq_p64(a, b, n) neon_extq64((a), (b), (n))
#define vmax_s8(a, b) neon_smax8((a), (b))
#define vmaxq_s8(a, b) neon_smaxq8((a), (b))
#define vmax_s16(a, b) neon_smax16((a), (b))
#define vmaxq_s16(a, b) neon_smaxq16((a), (b))
#define vmax_s32(a, b) neon_smax32((a), (b))
#define vmaxq_s32(a, b) neon_smaxq32((a), (b))
#define vmax_u8(a, b) neon_umax8((a), (b))
#define vmaxq_u8(a, b) neon_umaxq8((a), (b))
#define vmax_u16(a, b) neon_umax16((a), (b))
#define vmaxq_u16(a, b) neon_umaxq16((a), (b))
#define vmax_u32(a, b) neon_umax32((a), (b))
#define vmaxq_u32(a, b) neon_umaxq32((a), (b))
#define vmax_f32(a, b) neon_fmax32((a), (b))
#define vmaxq_f32(a, b) neon_fmaxq32((a), (b))
#define vmax_f64(a, b) neon_fmax64((a), (b))
#define vmaxq_f64(a, b) neon_fmaxq64((a), (b))
#define vmin_s8(a, b) neon_smin8((a), (b))
#define vminq_s8(a, b) neon_sminq8((a), (b))
#define vmin_s16(a, b) neon_smin16((a), (b))
#define vminq_s16(a, b) neon_sminq16((a), (b))
#define vmin_s32(a, b) neon_smin32((a), (b))
#define vminq_s32(a, b) neon_sminq32((a), (b))
#define vmin_u8(a, b) neon_umin8((a), (b))
#define vminq_u8(a, b) neon_uminq8((a), (b))
#define vmin_u16(a, b) neon_umin16((a), (b))
#define vminq_u16(a, b) neon_uminq16((a), (b))
#define vmin_u32(a, b) neon_umin32((a), (b))
#define vminq_u32(a, b) neon_uminq32((a), (b))
#define vmin_f32(a, b) neon_fmin32((a), (b))
#define vminq_f32(a, b) neon_fminq32((a), (b))
#define vmin_f64(a, b) neon_fmin64((a), (b))
#define vminq_f64(a, b) neon_fminq64((a), (b))
#define vmaxv_s8(a) neon_smaxv8((a)).n8_i8[0]
#define vmaxvq_s8(a) neon_smaxvq8((a)).n8_i8[0]
#define vmaxv_s16(a) neon_smaxv16((a)).n16_i16[0]
#define vmaxvq_s16(a) neon_smaxvq16((a)).n16_i16[0]
#define vmaxv_s32(a) neon_smaxp32((a), (a)).n64_i32[0]
#define vmaxvq_s32(a) _CopyInt32FromFloat(neon_smaxvq32((a)))
#define vmaxv_u8(a) neon_umaxv8((a)).n8_u8[0]
#define vmaxvq_u8(a) neon_umaxvq8((a)).n8_u8[0]
#define vmaxv_u16(a) neon_umaxv16((a)).n16_u16[0]
#define vmaxvq_u16(a) neon_umaxvq16((a)).n16_u16[0]
#define vmaxv_u32(a) neon_umaxp32((a), (a)).n64_u32[0]
#define vmaxvq_u32(a) (unsigned __int32)_CopyInt32FromFloat(neon_umaxvq32((a)))
#define vminv_s8(a) neon_sminv8((a)).n8_i8[0]
#define vminvq_s8(a) neon_sminvq8((a)).n8_i8[0]
#define vminv_s16(a) neon_sminv16((a)).n16_i16[0]
#define vminvq_s16(a) neon_sminvq16((a)).n16_i16[0]
#define vminv_s32(a) neon_sminp32((a), (a)).n64_i32[0]
#define vminvq_s32(a) _CopyInt32FromFloat(neon_sminvq32((a)))
#define vminv_u8(a) neon_uminv8((a)).n8_u8[0]
#define vminvq_u8(a) neon_uminvq8((a)).n8_u8[0]
#define vminv_u16(a) neon_uminv16((a)).n16_u16[0]
#define vminvq_u16(a) neon_uminvq16((a)).n16_u16[0]
#define vminv_u32(a) neon_uminp32((a), (a)).n64_u32[0]
#define vminvq_u32(a) (unsigned __int32)_CopyInt32FromFloat(neon_uminvq32((a)))
#define vaddv_s8(a) neon_addv8((a)).n8_i8[0]
#define vaddvq_s8(a) neon_addvq8((a)).n8_i8[0]
#define vaddv_s16(a) neon_addv16((a)).n16_i16[0]
#define vaddvq_s16(a) neon_addvq16((a)).n16_i16[0]
#define vaddv_s32(a) neon_addp32((a), (a)).n64_i32[0]
#define vaddvq_s32(a) _CopyInt32FromFloat(neon_addvq32((a)))
#define vaddv_u8(a) neon_addv8((a)).n8_u8[0]
#define vaddvq_u8(a) neon_addvq8((a)).n8_u8[0]
#define vaddv_u16(a) neon_addv16((a)).n16_u16[0]
#define vaddvq_u16(a) neon_addvq16((a)).n16_u16[0]
#define vaddv_u32(a) neon_addp32((a), (a)).n64_u32[0]
#define vaddvq_u32(a) (unsigned __int32)_CopyInt32FromFloat(neon_addvq32((a)))
#define vmaxv_f32(a) neon_fmaxps32((a))
#define vmaxvq_f32(a) neon_fmaxv((a))
#define vmaxvq_f64(a) neon_fmaxps64((a))
#define vminv_f32(a) neon_fminps32((a))
#define vminvq_f32(a) neon_fminv((a))
#define vminvq_f64(a) neon_fminps64((a))
#define vaddv_f32(a) neon_faddps32((a))
#define vaddvq_f32(a) neon_faddpsq32((a), (a))
#define vaddvq_f64(a) neon_faddpsq64((a)).n64_f64[0]
#define vaddvq_s64(a) neon_addps64((a)).n64_i64[0]
#define vaddvq_u64(a) neon_addps64((a)).n64_u64[0]
#define vrev64_s8(a) neon_rev64_8((a))
#define vrev64q_s8(a) neon_rev64q_8((a))
#define vrev64_s16(a) neon_rev64_16((a))
#define vrev64q_s16(a) neon_rev64q_16((a))
#define vrev64_s32(a) neon_rev64_32((a))
#define vrev64q_s32(a) neon_rev64q_32((a))
#define vrev64_u8(a) neon_rev64_8((a))
#define vrev64q_u8(a) neon_rev64q_8((a))
#define vrev64_u16(a) neon_rev64_16((a))
#define vrev64q_u16(a) neon_rev64q_16((a))
#define vrev64_u32(a) neon_rev64_32((a))
#define vrev64q_u32(a) neon_rev64q_32((a))
#define vrev64_f32(a) neon_rev64_32((a))
#define vrev64q_f32(a) neon_rev64q_32((a))
#define vrev64_p8(a) neon_rev64_8((a))
#define vrev64q_p8(a) neon_rev64q_8((a))
#define vrev64_p16(a) neon_rev64_16((a))
#define vrev64q_p16(a) neon_rev64q_16((a))
#define vrev32_s8(a) neon_rev32_8((a))
#define vrev32q_s8(a) neon_rev32q_8((a))
#define vrev32_s16(a) neon_rev32_16((a))
#define vrev32q_s16(a) neon_rev32q_16((a))
#define vrev32_u8(a) neon_rev32_8((a))
#define vrev32q_u8(a) neon_rev32q_8((a))
#define vrev32_u16(a) neon_rev32_16((a))
#define vrev32q_u16(a) neon_rev32q_16((a))
#define vrev32_p8(a) neon_rev32_8((a))
#define vrev32q_p8(a) neon_rev32q_8((a))
#define vrev32_p16(a) neon_rev32_16((a))
#define vrev32q_p16(a) neon_rev32q_16((a))
#define vrev16_s8(a) neon_rev16((a))
#define vrev16q_s8(a) neon_rev16q((a))
#define vrev16_u8(a) neon_rev16((a))
#define vrev16q_u8(a) neon_rev16q((a))
#define vrev16_p8(a) neon_rev16((a))
#define vrev16q_p8(a) neon_rev16q((a))
#define vzip1_s8(a, b) neon_zip1_8((a), (b))
#define vzip1q_s8(a, b) neon_zip1_q8((a), (b))
#define vzip1_s16(a, b) neon_zip1_16((a), (b))
#define vzip1q_s16(a, b) neon_zip1_q16((a), (b))
#define vzip1_s32(a, b) neon_zip1_32((a), (b))
#define vzip1q_s32(a, b) neon_zip1_q32((a), (b))
#define vzip1q_s64(a, b) neon_zip1_q64((a), (b))
#define vzip1_u8(a, b) neon_zip1_8((a), (b))
#define vzip1q_u8(a, b) neon_zip1_q8((a), (b))
#define vzip1_u16(a, b) neon_zip1_16((a), (b))
#define vzip1q_u16(a, b) neon_zip1_q16((a), (b))
#define vzip1_u32(a, b) neon_zip1_32((a), (b))
#define vzip1q_u32(a, b) neon_zip1_q32((a), (b))
#define vzip1q_u64(a, b) neon_zip1_q64((a), (b))
#define vzip1_f32(a, b) neon_zip1_32((a), (b))
#define vzip1q_f32(a, b) neon_zip1_q32((a), (b))
#define vzip1q_f64(a, b) neon_zip1_q64((a), (b))
#define vzip1_p8(a, b) neon_zip1_8((a), (b))
#define vzip1q_p8(a, b) neon_zip1_q8((a), (b))
#define vzip1_p16(a, b) neon_zip1_16((a), (b))
#define vzip1q_p16(a, b) neon_zip1_q16((a), (b))
#define vzip1q_p64(a, b) neon_zip1_q64((a), (b))
#define vzip2_s8(a, b) neon_zip2_8((a), (b))
#define vzip2q_s8(a, b) neon_zip2_q8((a), (b))
#define vzip2_s16(a, b) neon_zip2_16((a), (b))
#define vzip2q_s16(a, b) neon_zip2_q16((a), (b))
#define vzip2_s32(a, b) neon_zip2_32((a), (b))
#define vzip2q_s32(a, b) neon_zip2_q32((a), (b))
#define vzip2q_s64(a, b) neon_zip2_q64((a), (b))
#define vzip2_u8(a, b) neon_zip2_8((a), (b))
#define vzip2q_u8(a, b) neon_zip2_q8((a), (b))
#define vzip2_u16(a, b) neon_zip2_16((a), (b))
#define vzip2q_u16(a, b) neon_zip2_q16((a), (b))
#define vzip2_u32(a, b) neon_zip2_32((a), (b))
#define vzip2q_u32(a, b) neon_zip2_q32((a), (b))
#define vzip2q_u64(a, b) neon_zip2_q64((a), (b))
#define vzip2_f32(a, b) neon_zip2_32((a), (b))
#define vzip2q_f32(a, b) neon_zip2_q32((a), (b))
#define vzip2q_f64(a, b) neon_zip2_q64((a), (b))
#define vzip2_p8(a, b) neon_zip2_8((a), (b))
#define vzip2q_p8(a, b) neon_zip2_q8((a), (b))
#define vzip2_p16(a, b) neon_zip2_16((a), (b))
#define vzip2q_p16(a, b) neon_zip2_q16((a), (b))
#define vzip2q_p64(a, b) neon_zip2_q64((a), (b))
#define vuzp1_s8(a, b) neon_uzp1_8((a), (b))
#define vuzp1q_s8(a, b) neon_uzp1_q8((a), (b))
#define vuzp1_s16(a, b) neon_uzp1_16((a), (b))
#define vuzp1q_s16(a, b) neon_uzp1_q16((a), (b))
#define vuzp1_s32(a, b) neon_uzp1_32((a), (b))
#define vuzp1q_s32(a, b) neon_uzp1_q32((a), (b))
#define vuzp1q_s64(a, b) neon_uzp1_q64((a), (b))
#define vuzp1_u8(a, b) neon_uzp1_8((a), (b))
#define vuzp1q_u8(a, b) neon_uzp1_q8((a), (b))
#define vuzp1_u16(a, b) neon_uzp1_16((a), (b))
#define vuzp1q_u16(a, b) neon_uzp1_q16((a), (b))
#define vuzp1_u32(a, b) neon_uzp1_32((a), (b))
#define vuzp1q_u32(a, b) neon_uzp1_q32((a), (b))
#define vuzp1q_u64(a, b) neon_uzp1_q64((a), (b))
#define vuzp1_f32(a, b) neon_uzp1_32((a), (b))
#define vuzp1q_f32(a, b) neon_uzp1_q32((a), (b))
#define vuzp1q_f64(a, b) neon_uzp1_q64((a), (b))
#define vuzp1_p8(a, b) neon_uzp1_8((a), (b))
#define vuzp1q_p8(a, b) neon_uzp1_q8((a), (b))
#define vuzp1_p16(a, b) neon_uzp1_16((a), (b))
#define vuzp1q_p16(a, b) neon_uzp1_q16((a), (b))
#define vuzp1q_p64(a, b) neon_uzp1_q64((a), (b))
#define vuzp2_s8(a, b) neon_uzp2_8((a), (b))
#define vuzp2q_s8(a, b) neon_uzp2_q8((a), (b))
#define vuzp2_s16(a, b) neon_uzp2_16((a), (b))
#define vuzp2q_s16(a, b) neon_uzp2_q16((a), (b))
#define vuzp2_s32(a, b) neon_uzp2_32((a), (b))
#define vuzp2q_s32(a, b) neon_uzp2_q32((a), (b))
#define vuzp2q_s64(a, b) neon_uzp2_q64((a), (b))
#define vuzp2_u8(a, b) neon_uzp2_8((a), (b))
#define vuzp2q_u8(a, b) neon_uzp2_q8((a), (b))
#define vuzp2_u16(a, b) neon_uzp2_16((a), (b))
#define vuzp2q_u16(a, b) neon_uzp2_q16((a), (b))
#define vuzp2_u32(a, b) neon_uzp2_32((a), (b))
#define vuzp2q_u32(a, b) neon_uzp2_q32((a), (b))
#define vuzp2q_u64(a, b) neon_uzp2_q64((a), (b))
#define vuzp2_f32(a, b) neon_uzp2_32((a), (b))
#define vuzp2q_f32(a, b) neon_uzp2_q32((a), (b))
#define vuzp2q_f64(a, b) neon_uzp2_q64((a), (b))
#define vuzp2_p8(a, b) neon_uzp2_8((a), (b))
#define vuzp2q_p8(a, b) neon_uzp2_q8((a), (b))
#define vuzp2_p16(a, b) neon_uzp2_16((a), (b))
#define vuzp2q_p16(a, b) neon_uzp2_q16((a), (b))
#define vuzp2q_p64(a, b) neon_uzp2_q64((a), (b))
#define vtrn1_s8(a, b) neon_trn1_8((a), (b))
#define vtrn1q_s8(a, b) neon_trn1_q8((a), (b))
#define vtrn1_s16(a, b) neon_trn1_16((a), (b))
#define vtrn1q_s16(a, b) neon_trn1_q16((a), (b))
#define vtrn1_s32(a, b) neon_trn1_32((a), (b))
#define vtrn1q_s32(a, b) neon_trn1_q32((a), (b))
#define vtrn1q_s64(a, b) neon_trn1_q64((a), (b))
#define vtrn1_u8(a, b) neon_trn1_8((a), (b))
#define vtrn1q_u8(a, b) neon_trn1_q8((a), (b))
#define vtrn1_u16(a, b) neon_trn1_16((a), (b))
#define vtrn1q_u16(a, b) neon_trn1_q16((a), (b))
#define vtrn1_u32(a, b) neon_trn1_32((a), (b))
#define vtrn1q_u32(a, b) neon_trn1_q32((a), (b))
#define vtrn1q_u64(a, b) neon_trn1_q64((a), (b))
#define vtrn1_f32(a, b) neon_trn1_32((a), (b))
#define vtrn1q_f32(a, b) neon_trn1_q32((a), (b))
#define vtrn1q_f64(a, b) neon_trn1_q64((a), (b))
#define vtrn1_p8(a, b) neon_trn1_8((a), (b))
#define vtrn1q_p8(a, b) neon_trn1_q8((a), (b))
#define vtrn1_p16(a, b) neon_trn1_16((a), (b))
#define vtrn1q_p16(a, b) neon_trn1_q16((a), (b))
#define vtrn1q_p64(a, b) neon_trn1_q64((a), (b))
#define vtrn2_s8(a, b) neon_trn2_8((a), (b))
#define vtrn2q_s8(a, b) neon_trn2_q8((a), (b))
#define vtrn2_s16(a, b) neon_trn2_16((a), (b))
#define vtrn2q_s16(a, b) neon_trn2_q16((a), (b))
#define vtrn2_s32(a, b) neon_trn2_32((a), (b))
#define vtrn2q_s32(a, b) neon_trn2_q32((a), (b))
#define vtrn2q_s64(a, b) neon_trn2_q64((a), (b))
#define vtrn2_u8(a, b) neon_trn2_8((a), (b))
#define vtrn2q_u8(a, b) neon_trn2_q8((a), (b))
#define vtrn2_u16(a, b) neon_trn2_16((a), (b))
#define vtrn2q_u16(a, b) neon_trn2_q16((a), (b))
#define vtrn2_u32(a, b) neon_trn2_32((a), (b))
#define vtrn2q_u32(a, b) neon_trn2_q32((a), (b))
#define vtrn2q_u64(a, b) neon_trn2_q64((a), (b))
#define vtrn2_f32(a, b) neon_trn2_32((a), (b))
#define vtrn2q_f32(a, b) neon_trn2_q32((a), (b))
#define vtrn2q_f64(a, b) neon_trn2_q64((a), (b))
#define vtrn2_p8(a, b) neon_trn2_8((a), (b))
#define vtrn2q_p8(a, b) neon_trn2_q8((a), (b))
#define vtrn2_p16(a, b) neon_trn2_16((a), (b))
#define vtrn2q_p16(a, b) neon_trn2_q16((a), (b))
#define vtrn2q_p64(a, b) neon_trn2_q64((a), (b))
#define vtbl1_s8(a, b) neon_tbl1_8((a), (b))
#define vqtbl1_s8(a, b) neon_tbl1_q8((a), (b))
#define vqtbl1q_s8(a, b) neon_tbl1_qq8((a), (b))
#define vtbl1_u8(a, b) neon_tbl1_8((a), (b))
#define vqtbl1_u8(a, b) neon_tbl1_q8((a), (b))
#define vqtbl1q_u8(a, b) neon_tbl1_qq8((a), (b))
#define vtbl1_p8(a, b) neon_tbl1_8((a), (b))
#define vqtbl1_p8(a, b) neon_tbl1_q8((a), (b))
#define vqtbl1q_p8(a, b) neon_tbl1_qq8((a), (b))
#define vaeseq_u8(a, b) neon_aese((a), (b))
#define vaesdq_u8(a, b) neon_aesd((a), (b))
#define vaesmcq_u8(a) neon_aesmc((a))
#define vaesimcq_u8(a) neon_aesimc((a))
#define vsha1cq_u32(abcd, e, wk) neon_sha1cui((abcd), (e), (wk))
#define vsha1pq_u32(abcd, e, wk) neon_sha1pui((abcd), (e), (wk))
#define vsha1mq_u32(abcd, e, wk) neon_sha1mui((abcd), (e), (wk))
#define vsha1h_u32(e) neon_sha1hui((e))
#define vsha1su0q_u32(a, b, c) neon_sha1su0((a), (b), (c))
#define vsha1su1q_u32(a, b) neon_sha1su1((a), (b))
#define vsha256hq_u32(a, b, c) neon_sha256h((a), (b), (c))
#define vsha256h2q_u32(a, b, c) neon_sha256h2((a), (b), (c))
#define vsha256su1q_u32(a, b, c) neon_sha256su1((a), (b), (c))
#define vsha256su0q_u32(a, b) neon_sha256su0((a), (b))
#define vsha512hq_u64(a, b, c) neon_sha512h((a), (b), (c))
#define vsha512h2q_u64(a, b, c) neon_sha512h2((a), (b), (c))
#define vsha512su1q_u64(a, b, c) neon_sha512su1((a), (b), (c))
#define vsha512su0q_u64(a, b) neon_sha512su0((a), (b))
#define vreinterpret_s8_s16(a) (a)
#define vreinterpret_s8_s32(a) (a)
#define vreinterpret_s8_s64(a) (a)
#define vreinterpret_s8_u8(a) (a)
#define vreinterpret_s8_u16(a) (a)
#define vreinterpret_s8_u32(a) (a)
#define vreinterpret_s8_u64(a) (a)
#define vreinterpret_s8_f32(a) (a)
#define vreinterpret_s8_f64(a) (a)
#define vreinterpret_s8_p8(a) (a)
#define vreinterpret_s8_p16(a) (a)
#define vreinterpret_s8_p64(a) (a)
#define vreinterpret_s16_s8(a) (a)
#define vreinterpret_s16_s32(a) (a)
#define vreinterpret_s16_s64(a) (a)
#define vreinterpret_s16_u8(a) (a)
#define vreinterpret_s16_u16(a) (a)
#define vreinterpret_s16_u32(a) (a)
#define vreinterpret_s16_u64(a) (a)
#define vreinterpret_s16_f32(a) (a)
#define vreinterpret_s16_f64(a) (a)
#define vreinterpret_s16_p8(a) (a)
#define vreinterpret_s16_p16(a) (a)
#define vreinterpret_s16_p64(a) (a)
#define vreinterpret_s32_s8(a) (a)
#define vreinterpret_s32_s16(a) (a)
#define vreinterpret_s32_s64(a) (a)
#define vreinterpret_s32_u8(a) (a)
#define vreinterpret_s32_u16(a) (a)
#define vreinterpret_s32_u32(a) (a)
#define vreinterpret_s32_u64(a) (a)
#define vreinterpret_s32_f32(a) (a)
#define vreinterpret_s32_f64(a) (a)
#define vreinterpret_s32_p8(a) (a)
#define vreinterpret_s32_p16(a) (a)
#define vreinterpret_s32_p64(a) (a)
#define vreinterpret_s64_s8(a) (a)
#define vreinterpret_s64_s16(a) (a)
#define vreinterpret_s64_s32(a) (a)
#define vreinterpret_s64_u8(a) (a)
#define vreinterpret_s64_u16(a) (a)
#define vreinterpret_s64_u32(a) (a)
#define vreinterpret_s64_u64(a) (a)
#define vreinterpret_s64_f32(a) (a)
#define vreinterpret_s64_f64(a) (a)
#define vreinterpret_s64_p8(a) (a)
#define vreinterpret_s64_p16(a) (a)
#define vreinterpret_s64_p64(a) (a)
#define vreinterpret_u8_s8(a) (a)
#define vreinterpret_u8_s16(a) (a)
#define vreinterpret_u8_s32(a) (a)
#define vreinterpret_u8_s64(a) (a)
#define vreinterpret_u8_u16(a) (a)
#define vreinterpret_u8_u32(a) (a)
#define vreinterpret_u8_u64(a) (a)
#define vreinterpret_u8_f32(a) (a)
#define vreinterpret_u8_f64(a) (a)
#define vreinterpret_u8_p8(a) (a)
#define vreinterpret_u8_p16(a) (a)
#define vreinterpret_u8_p64(a) (a)
#define vreinterpret_u16_s8(a) (a)
#define vreinterpret_u16_s16(a) (a)
#define vreinterpret_u16_s32(a) (a)
#define vreinterpret_u16_s64(a) (a)
#define vreinterpret_u16_u8(a) (a)
#define vreinterpret_u16_u32(a) (a)
#define vreinterpret_u16_u64(a) (a)
#define vreinterpret_u16_f32(a) (a)
#define vreinterpret_u16_f64(a) (a)
#define vreinterpret_u16_p8(a) (a)
#define vreinterpret_u16_p16(a) (a)
#define vreinterpret_u16_p64(a) (a)
#define vreinterpret_u32_s8(a) (a)
#define vreinterpret_u32_s16(a) (a)
#define vreinterpret_u32_s32(a) (a)
#define vreinterpret_u32_s64(a) (a)
#define vreinterpret_u32_u8(a) (a)
#define vreinterpret_u32_u16(a) (a)
#define vreinterpret_u32_u64(a) (a)
#define vreinterpret_u32_f32(a) (a)
#define vreinterpret_u32_f64(a) (a)
#define vreinterpret_u32_p8(a) (a)
#define vreinterpret_u32_p16(a) (a)
#define vreinterpret_u32_p64(a) (a)
#define vreinterpret_u64_s8(a) (a)
#define vreinterpret_u64_s16(a) (a)
#define vreinterpret_u64_s32(a) (a)
#define vreinterpret_u64_s64(a) (a)
#define vreinterpret_u64_u8(a) (a)
#define vreinterpret_u64_u16(a) (a)
#define vreinterpret_u64_u32(a) (a)
#define vreinterpret_u64_f32(a) (a)
#define vreinterpret_u64_f64(a) (a)
#define vreinterpret_u64_p8(a) (a)
#define vreinterpret_u64_p16(a) (a)
#define vreinterpret_u64_p64(a) (a)
#define vreinterpret_f32_s8(a) (a)
#define vreinterpret_f32_s16(a) (a)
#define vreinterpret_f32_s32(a) (a)
#define vreinterpret_f32_s64(a) (a)
#define vreinterpret_f32_u8(a) (a)
#define vreinterpret_f32_u16(a) (a)
#define vreinterpret_f32_u32(a) (a)
#define vreinterpret_f32_u64(a) (a)
#define vreinterpret_f32_f64(a) (a)
#define vreinterpret_f32_p8(a) (a)
#define vreinterpret_f32_p16(a) (a)
#define vreinterpret_f32_p64(a) (a)
#define vreinterpret_f64_s8(a) (a)
#define vreinterpret_f64_s16(a) (a)
#define vreinterpret_f64_s32(a) (a)
#define vreinterpret_f64_s64(a) (a)
#define vreinterpret_f64_u8(a) (a)
#define vreinterpret_f64_u16(a) (a)
#define vreinterpret_f64_u32(a) (a)
#define vreinterpret_f64_u64(a) (a)
#define vreinterpret_f64_f32(a) (a)
#define vreinterpret_f64_p8(a) (a)
#define vreinterpret_f64_p16(a) (a)
#define vreinterpret_f64_p64(a) (a)
#define vreinterpret_p8_s8(a) (a)
#define vreinterpret_p8_s16(a) (a)
#define vreinterpret_p8_s32(a) (a)
#define vreinterpret_p8_s64(a) (a)
#define vreinterpret_p8_u8(a) (a)
#define vreinterpret_p8_u16(a) (a)
#define vreinterpret_p8_u32(a) (a)
#define vreinterpret_p8_u64(a) (a)
#define vreinterpret_p8_f32(a) (a)
#define vreinterpret_p8_f64(a) (a)
#define vreinterpret_p8_p16(a) (a)
#define vreinterpret_p8_p64(a) (a)
#define vreinterpret_p16_s8(a) (a)
#define vreinterpret_p16_s16(a) (a)
#define vreinterpret_p16_s32(a) (a)
#define vreinterpret_p16_s64(a) (a)
#define vreinterpret_p16_u8(a) (a)
#define vreinterpret_p16_u16(a) (a)
#define vreinterpret_p16_u32(a) (a)
#define vreinterpret_p16_u64(a) (a)
#define vreinterpret_p16_f32(a) (a)
#define vreinterpret_p16_f64(a) (a)
#define vreinterpret_p16_p8(a) (a)
#define vreinterpret_p16_p64(a) (a)
#define vreinterpret_p64_s8(a) (a)
#define vreinterpret_p64_s16(a) (a)
#define vreinterpret_p64_s32(a) (a)
#define vreinterpret_p64_s64(a) (a)
#define vreinterpret_p64_u8(a) (a)
#define vreinterpret_p64_u16(a) (a)
#define vreinterpret_p64_u32(a) (a)
#define vreinterpret_p64_u64(a) (a)
#define vreinterpret_p64_f32(a) (a)
#define vreinterpret_p64_f64(a) (a)
#define vreinterpret_p64_p8(a) (a)
#define vreinterpret_p64_p16(a) (a)
#define vreinterpretq_s8_s16(a) (a)
#define vreinterpretq_s8_s32(a) (a)
#define vreinterpretq_s8_s64(a) (a)
#define vreinterpretq_s8_u8(a) (a)
#define vreinterpretq_s8_u16(a) (a)
#define vreinterpretq_s8_u32(a) (a)
#define vreinterpretq_s8_u64(a) (a)
#define vreinterpretq_s8_f32(a) (a)
#define vreinterpretq_s8_f64(a) (a)
#define vreinterpretq_s8_p8(a) (a)
#define vreinterpretq_s8_p16(a) (a)
#define vreinterpretq_s8_p64(a) (a)
#define vreinterpretq_s16_s8(a) (a)
#define vreinterpretq_s16_s32(a) (a)
#define vreinterpretq_s16_s64(a) (a)
#define vreinterpretq_s16_u8(a) (a)
#define vreinterpretq_s16_u16(a) (a)
#define vreinterpretq_s16_u32(a) (a)
#define vreinterpretq_s16_u64(a) (a)
#define vreinterpretq_s16_f32(a) (a)
#define vreinterpretq_s16_f64(a) (a)
#define vreinterpretq_s16_p8(a) (a)
#define vreinterpretq_s16_p16(a) (a)
#define vreinterpretq_s16_p64(a) (a)
#define vreinterpretq_s32_s8(a) (a)
#define vreinterpretq_s32_s16(a) (a)
#define vreinterpretq_s32_s64(a) (a)
#define vreinterpretq_s32_u8(a) (a)
#define vreinterpretq_s32_u16(a) (a)
#define vreinterpretq_s32_u32(a) (a)
#define vreinterpretq_s32_u64(a) (a)
#define vreinterpretq_s32_f32(a) (a)
#define vreinterpretq_s32_f64(a) (a)
#define vreinterpretq_s32_p8(a) (a)
#define vreinterpretq_s32_p16(a) (a)
#define vreinterpretq_s32_p64(a) (a)
#define vreinterpretq_s64_s8(a) (a)
#define vreinterpretq_s64_s16(a) (a)
#define vreinterpretq_s64_s32(a) (a)
#define vreinterpretq_s64_u8(a) (a)
#define vreinterpretq_s64_u16(a) (a)
#define vreinterpretq_s64_u32(a) (a)
#define vreinterpretq_s64_u64(a) (a)
#define vreinterpretq_s64_f32(a) (a)
#define vreinterpretq_s64_f64(a) (a)
#define vreinterpretq_s64_p8(a) (a)
#define vreinterpretq_s64_p16(a) (a)
#define vreinterpretq_s64_p64(a) (a)
#define vreinterpretq_u8_s8(a) (a)
#define vreinterpretq_u8_s16(a) (a)
#define vreinterpretq_u8_s32(a) (a)
#define vreinterpretq_u8_s64(a) (a)
#define vreinterpretq_u8_u16(a) (a)
#define vreinterpretq_u8_u32(a) (a)
#define vreinterpretq_u8_u64(a) (a)
#define vreinterpretq_u8_f32(a) (a)
#define vreinterpretq_u8_f64(a) (a)
#define vreinterpretq_u8_p8(a) (a)
#define vreinterpretq_u8_p16(a) (a)
#define vreinterpretq_u8_p64(a) (a)
#define vreinterpretq_u16_s8(a) (a)
#define vreinterpretq_u16_s16(a) (a)
#define vreinterpretq_u16_s32(a) (a)
#define vreinterpretq_u16_s64(a) (a)
#define vreinterpretq_u16_u8(a) (a)
#define vreinterpretq_u16_u32(a) (a)
#define vreinterpretq_u16_u64(a) (a)
#define vreinterpretq_u16_f32(a) (a)
#define vreinterpretq_u16_f64(a) (a)
#define vreinterpretq_u16_p8(a) (a)
#define vreinterpretq_u16_p16(a) (a)
#define vreinterpretq_u16_p64(a) (a)
#define vreinterpretq_u32_s8(a) (a)
#define vreinterpretq_u32_s16(a) (a)
#define vreinterpretq_u32_s32(a) (a)
#define vreinterpretq_u32_s64(a) (a)
#define vreinterpretq_u32_u8(a) (a)
#define vreinterpretq_u32_u16(a) (a)
#define vreinterpretq_u32_u64(a) (a)
#define vreinterpretq_u32_f32(a) (a)
#define vreinterpretq_u32_f64(a) (a)
#define vreinterpretq_u32_p8(a) (a)
#define vreinterpretq_u32_p16(a) (a)
#define vreinterpretq_u32_p64(a) (a)
#define vreinterpretq_u64_s8(a) (a)
#define vreinterpretq_u64_s16(a) (a)
#define vreinterpretq_u64_s32(a) (a)
#define vreinterpretq_u64_s64(a) (a)
#define vreinterpretq_u64_u8(a) (a)
#define vreinterpretq_u64_u16(a) (a)
#define vreinterpretq_u64_u32(a) (a)
#define vreinterpretq_u64_f32(a) (a)
#define vreinterpretq_u64_f64(a) (a)
#define vreinterpretq_u64_p8(a) (a)
#define vreinterpretq_u64_p16(a) (a)
#define vreinterpretq_u64_p64(a) (a)
#define vreinterpretq_f32_s8(a) (a)
#define vreinterpretq_f32_s16(a) (a)
#define vreinterpretq_f32_s32(a) (a)
#define vreinterpretq_f32_s64(a) (a)
#define vreinterpretq_f32_u8(a) (a)
#define vreinterpretq_f32_u16(a) (a)
#define vreinterpretq_f32_u32(a) (a)
#define vreinterpretq_f32_u64(a) (a)
#define vreinterpretq_f32_f64(a) (a)
#define vreinterpretq_f32_p8(a) (a)
#define vreinterpretq_f32_p16(a) (a)
#define vreinterpretq_f32_p64(a) (a)
#define vreinterpretq_f64_s8(a) (a)
#define vreinterpretq_f64_s16(a) (a)
#define vreinterpretq_f64_s32(a) (a)
#define vreinterpretq_f64_s64(a) (a)
#define vreinterpretq_f64_u8(a) (a)
#define vreinterpretq_f64_u16(a) (a)
#define vreinterpretq_f64_u32(a) (a)
#define vreinterpretq_f64_u64(a) (a)
#define vreinterpretq_f64_f32(a) (a)
#define vreinterpretq_f64_p8(a) (a)
#define vreinterpretq_f64_p16(a) (a)
#define vreinterpretq_f64_p64(a) (a)
#define vreinterpretq_p8_s8(a) (a)
#define vreinterpretq_p8_s16(a) (a)
#define vreinterpretq_p8_s32(a) (a)
#define vreinterpretq_p8_s64(a) (a)
#define vreinterpretq_p8_u8(a) (a)
#define vreinterpretq_p8_u16(a) (a)
#define vreinterpretq_p8_u32(a) (a)
#define vreinterpretq_p8_u64(a) (a)
#define vreinterpretq_p8_f32(a) (a)
#define vreinterpretq_p8_f64(a) (a)
#define vreinterpretq_p8_p16(a) (a)
#define vreinterpretq_p8_p64(a) (a)
#define vreinterpretq_p16_s8(a) (a)
#define vreinterpretq_p16_s16(a) (a)
#define vreinterpretq_p16_s32(a) (a)
#define vreinterpretq_p16_s64(a) (a)
#define vreinterpretq_p16_u8(a) (a)
#define vreinterpretq_p16_u16(a) (a)
#define vreinterpretq_p16_u32(a) (a)
#define vreinterpretq_p16_u64(a) (a)
#define vreinterpretq_p16_f32(a) (a)
#define vreinterpretq_p16_f64(a) (a)
#define vreinterpretq_p16_p8(a) (a)
#define vreinterpretq_p16_p64(a) (a)
#define vreinterpretq_p64_s8(a) (a)
#define vreinterpretq_p64_s16(a) (a)
#define vreinterpretq_p64_s32(a) (a)
#define vreinterpretq_p64_s64(a) (a)
#define vreinterpretq_p64_u8(a) (a)
#define vreinterpretq_p64_u16(a) (a)
#define vreinterpretq_p64_u32(a) (a)
#define vreinterpretq_p64_u64(a) (a)
#define vreinterpretq_p64_f32(a) (a)
#define vreinterpretq_p64_f64(a) (a)
#define vreinterpretq_p64_p8(a) (a)
#define vreinterpretq_p64_p16(a) (a)

#if defined(__cplusplus)
} // extern "C"
#endif
