/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Simplified implementation of __libm_sse2_*
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <emmintrin.h>
#include <math.h>

#if defined(_MSC_VER) && !defined(__clang__)
#pragma function(acos,asin,atan,atan2,cos)
#pragma function(exp,log,log10,pow,sin,tan)
#define __ATTRIBUTE_SSE2__
#else
#define __ATTRIBUTE_SSE2__ __attribute__((__target__("sse2")))
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

__ATTRIBUTE_SSE2__ __m128d __libm_sse2_acos(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = acos(x);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128 __libm_sse2_acosf(__m128 Xmm0)
{
    __m128d Xmm0d = _mm_cvtss_sd(Xmm0d, Xmm0);
    double x = _mm_cvtsd_f64(Xmm0d);
    double result = acos(x);
    __m128d result128 = _mm_set_sd(result);
    return _mm_cvtpd_ps(result128);
}

__ATTRIBUTE_SSE2__ __m128d __libm_sse2_asin(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = asin(x);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128 __libm_sse2_asinf(__m128 Xmm0)
{
    __m128d Xmm0d = _mm_cvtss_sd(Xmm0d, Xmm0);
    double x = _mm_cvtsd_f64(Xmm0d);
    double result = asin(x);
    __m128d result128 = _mm_set_sd(result);
    return _mm_cvtpd_ps(result128);
}

__ATTRIBUTE_SSE2__ __m128d __libm_sse2_atan(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = atan(x);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128 __libm_sse2_atanf(__m128 Xmm0)
{
    __m128d Xmm0d = _mm_cvtss_sd(Xmm0d, Xmm0);
    double x = _mm_cvtsd_f64(Xmm0d);
    double result = atan(x);
    __m128d result128 = _mm_set_sd(result);
    return _mm_cvtpd_ps(result128);
}

__ATTRIBUTE_SSE2__ __m128d __libm_sse2_atan2(__m128d Xmm0, __m128d Xmm1)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double y = _mm_cvtsd_f64(Xmm1);
    double result = atan2(x, y);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128d __libm_sse2_cos(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = cos(x);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128 __libm_sse2_cosf(__m128 Xmm0)
{
    __m128d Xmm0d = _mm_cvtss_sd(Xmm0d, Xmm0);
    double x = _mm_cvtsd_f64(Xmm0d);
    double result = cos(x);
    __m128d result128 = _mm_set_sd(result);
    return _mm_cvtpd_ps(result128);
}

__ATTRIBUTE_SSE2__ __m128d __libm_sse2_exp(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = exp(x);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128 __libm_sse2_expf(__m128 Xmm0)
{
    __m128d Xmm0d = _mm_cvtss_sd(Xmm0d, Xmm0);
    double x = _mm_cvtsd_f64(Xmm0d);
    double result = exp(x);
    __m128d result128 = _mm_set_sd(result);
    return _mm_cvtpd_ps(result128);
}

__ATTRIBUTE_SSE2__ __m128d __libm_sse2_log(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = log(x);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128 __libm_sse2_logf(__m128 Xmm0)
{
    __m128d Xmm0d = _mm_cvtss_sd(Xmm0d, Xmm0);
    double x = _mm_cvtsd_f64(Xmm0d);
    double result = log(x);
    __m128d result128 = _mm_set_sd(result);
    return _mm_cvtpd_ps(result128);
}

__ATTRIBUTE_SSE2__ __m128d __libm_sse2_log10(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = log10(x);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128 __libm_sse2_log10f(__m128 Xmm0)
{
    __m128d Xmm0d = _mm_cvtss_sd(Xmm0d, Xmm0);
    double x = _mm_cvtsd_f64(Xmm0d);
    double result = log10(x);
    __m128d result128 = _mm_set_sd(result);
    return _mm_cvtpd_ps(result128);
}

__ATTRIBUTE_SSE2__ __m128d __libm_sse2_pow(__m128d Xmm0, __m128d Xmm1)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double y = _mm_cvtsd_f64(Xmm1);
    double result = pow(x, y);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128 __libm_sse2_powf(__m128 Xmm0, __m128 Xmm1)
{
    float x = _mm_cvtss_f32(Xmm0);
    float y = _mm_cvtss_f32(Xmm1);
    float result = powf(x, y);
    return _mm_set_ss(result);
}

__ATTRIBUTE_SSE2__ __m128d __libm_sse2_sin(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = sin(x);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128 __libm_sse2_sinf(__m128 Xmm0)
{
    __m128d Xmm0d = _mm_cvtss_sd(Xmm0d, Xmm0);
    double x = _mm_cvtsd_f64(Xmm0d);
    double result = sin(x);
    __m128d result128 = _mm_set_sd(result);
    return _mm_cvtpd_ps(result128);
}

__ATTRIBUTE_SSE2__ __m128d __libm_sse2_tan(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = tan(x);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128 __libm_sse2_tanf(__m128 Xmm0)
{
    __m128d Xmm0d = _mm_cvtss_sd(Xmm0d, Xmm0);
    double x = _mm_cvtsd_f64(Xmm0d);
    double result = tan(x);
    __m128d result128 = _mm_set_sd(result);
    return _mm_cvtpd_ps(result128);
}

__ATTRIBUTE_SSE2__ __m128d _libm_sse2_acos_precise(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = acos(x);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128d _libm_sse2_asin_precise(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = asin(x);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128d _libm_sse2_atan_precise(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = atan(x);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128d _libm_sse2_cos_precise(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = cos(x);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128d _libm_sse2_exp_precise(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = exp(x);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128d _libm_sse2_log_precise(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = log(x);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128d _libm_sse2_log10_precise(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = log10(x);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128d _libm_sse2_pow_precise(__m128d Xmm0, __m128d Xmm1)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double y = _mm_cvtsd_f64(Xmm1);
    double result = pow(x, y);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128d _libm_sse2_sin_precise(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = sin(x);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128d _libm_sse2_sqrt_precise(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = sqrt(x);
    return _mm_set_sd(result);
}

__ATTRIBUTE_SSE2__ __m128d _libm_sse2_tan_precise(__m128d Xmm0)
{
    double x = _mm_cvtsd_f64(Xmm0);
    double result = tan(x);
    return _mm_set_sd(result);
}
