//
// tgmath.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The type-generic math library.
//
#pragma once
#ifndef _TGMATH
#define _TGMATH

#include <corecrt.h>

#if (_CRT_HAS_CXX17 == 1) && !defined(_CRT_USE_C_TGMATH_H)

#include <ctgmath>

#else // ^^^^ /std:c++17 ^^^^ // vvvv _CRT_USE_C_TGMATH_H vvvv

#include <math.h>
#include <complex.h>

#if _CRT_HAS_C11 == 0

#ifndef _CRT_SILENCE_NONCONFORMING_TGMATH_H

#pragma message(_CRT_WARNING_MESSAGE("UCRT4000", \
    "This header does not conform to the C99 standard. " \
    "C99 functionality is available when compiling in C11 mode or higher (/std:c11). " \
    "Functionality equivalent to the type-generic functions provided by tgmath.h is available " \
    "in <ctgmath> when compiling as C++. " \
    "If compiling in C++17 mode or higher (/std:c++17), this header will automatically include <ctgmath> instead. " \
    "You can define _CRT_SILENCE_NONCONFORMING_TGMATH_H to acknowledge that you have received this warning."))

#endif // _CRT_SILENCE_NONCONFORMING_TGMATH_H

#else // ^^^^ Default C Support ^^^^ // vvvv C11 Support vvvv

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS
_CRT_BEGIN_C_HEADER

#define __tgmath_resolve_real_binary_op(X, Y) _Generic((X), \
    long double: 0.0l,                                      \
                                                            \
    default:     _Generic((Y),                              \
        long double: 0.0l,                                  \
        default:     0.0                                    \
        ),                                                  \
                                                            \
    float:       _Generic((Y),                              \
        long double: 0.0l,                                  \
        default:     0.0,                                   \
        float:       0.0f                                   \
        )                                                   \
    )

#define fabs(X) _Generic((X), \
    _Lcomplex:   cabsl,       \
    _Fcomplex:   cabsf,       \
    _Dcomplex:   cabs,        \
    long double: fabsl,       \
    float:       fabsf,       \
    default:     fabs         \
)(X)

#define exp(X) _Generic((X), \
    _Lcomplex:   cexpl,      \
    _Fcomplex:   cexpf,      \
    _Dcomplex:   cexp,       \
    long double: expl,       \
    float:       expf,       \
    default:     exp         \
)(X)

#define log(X) _Generic((X), \
    _Lcomplex:   clogl,      \
    _Fcomplex:   clogf,      \
    _Dcomplex:   clog,       \
    long double: logl,       \
    float:       logf,       \
    default:     log         \
)(X)

// C99 Complex types currently not supported. Complex types do not cast/promote implicitly - need inline helper functions.

inline _Lcomplex __cpowl_lc_dc(_Lcomplex const __lc, _Dcomplex const __dc)
{
    return cpowl(__lc, _LCbuild(__dc._Val[0], __dc._Val[1]));
}

inline _Lcomplex __cpowl_dc_lc(_Dcomplex const __dc, _Lcomplex const __lc)
{
    return cpowl(_LCbuild(__dc._Val[0], __dc._Val[1]), __lc);
}

inline _Lcomplex __cpowl_lc_fc(_Lcomplex const __lc, _Fcomplex const __fc)
{
    return cpowl(__lc, _LCbuild(__fc._Val[0], __fc._Val[1]));
}

inline _Lcomplex __cpowl_fc_lc(_Fcomplex const __fc, _Lcomplex const __lc)
{
    return cpowl(_LCbuild(__fc._Val[0], __fc._Val[1]), __lc);
}

inline _Lcomplex __cpowl_lc_l(_Lcomplex const __lc, long double const __l)
{
    return cpowl(__lc, _LCbuild(__l, 0.0));
}

inline _Lcomplex __cpowl_l_lc(long double const __l, _Lcomplex const __lc)
{
    return cpowl(_LCbuild(__l, 0.0), __lc);
}

inline _Lcomplex __cpowl_lc_d(_Lcomplex const __lc, double const __d)
{
    return cpowl(__lc, _LCbuild(__d, 0.0));
}

inline _Lcomplex __cpowl_d_lc(double const __d, _Lcomplex const __lc)
{
    return cpowl(_LCbuild(__d, 0.0), __lc);
}

inline _Lcomplex __cpowl_lc_f(_Lcomplex const __lc, float const __f)
{
    return cpowl(__lc, _LCbuild(__f, 0.0));
}

inline _Lcomplex __cpowl_f_lc(float const __f, _Lcomplex const __lc)
{
    return cpowl(_LCbuild(__f, 0.0), __lc);
}

inline _Lcomplex __cpowl_dc_l(_Dcomplex const __dc, long double const __l)
{
    return cpowl(_LCbuild(__dc._Val[0], __dc._Val[1]), _LCbuild(__l, 0.0));
}

inline _Lcomplex __cpowl_l_dc(long double const __l, _Dcomplex const __dc)
{
    return cpowl(_LCbuild(__l, 0.0), _LCbuild(__dc._Val[0], __dc._Val[1]));
}

inline _Lcomplex __cpowl_fc_l(_Fcomplex const __fc, long double const __l)
{
    return cpowl(_LCbuild(__fc._Val[0], __fc._Val[1]), _LCbuild(__l, 0.0));
}

inline _Lcomplex __cpowl_l_fc(long double const __l, _Fcomplex const __fc)
{
    return cpowl(_LCbuild(__l, 0.0), _LCbuild(__fc._Val[0], __fc._Val[1]));
}

inline _Dcomplex __cpow_dc_fc(_Dcomplex const __dc, _Fcomplex const __fc)
{
    return cpow(__dc, _Cbuild(__fc._Val[0], __fc._Val[1]));
}

inline _Dcomplex __cpow_fc_dc(_Fcomplex const __fc, _Dcomplex const __dc)
{
    return cpow(_Cbuild(__fc._Val[0], __fc._Val[1]), __dc);
}

inline _Dcomplex __cpow_dc_d(_Dcomplex const __dc, double const __d)
{
    return cpow(__dc, _Cbuild(__d, 0.0));
}

inline _Dcomplex __cpow_d_dc(double const __d, _Dcomplex const __dc)
{
    return cpow(_Cbuild(__d, 0.0), __dc);
}

inline _Dcomplex __cpow_dc_f(_Dcomplex const __dc, float const __f)
{
    return cpow(__dc, _Cbuild(__f, 0.0));
}

inline _Dcomplex __cpow_f_dc(float const __f, _Dcomplex const __dc)
{
    return cpow(_Cbuild(__f, 0.0), __dc);
}

inline _Dcomplex __cpow_fc_d(_Fcomplex const __fc, double const __d)
{
    return cpow(_Cbuild(__fc._Val[0], __fc._Val[1]), _Cbuild(__d, 0.0));
}

inline _Dcomplex __cpow_d_fc(double const __d, _Fcomplex const __fc)
{
    return cpow(_Cbuild(__d, 0.0), _Cbuild(__fc._Val[0], __fc._Val[1]));
}

inline _Fcomplex __cpowf_fc_f(_Fcomplex const __fc, float const __f)
{
    return cpowf(__fc, _FCbuild(__f, 0.0f));
}

inline _Fcomplex __cpowf_f_fc(float const __f, _Fcomplex const __fc)
{
    return cpowf(_FCbuild(__f, 0.0f), __fc);
}

#define pow(X, Y) _Generic((X),     \
    _Lcomplex:   _Generic((Y),      \
        _Lcomplex:   cpowl,         \
        _Fcomplex:   __cpowl_lc_fc, \
        _Dcomplex:   __cpowl_lc_dc, \
        long double: __cpowl_lc_l,  \
        default:     __cpowl_lc_d,  \
        float:       __cpowl_lc_f   \
        ),                          \
                                    \
    _Fcomplex:   _Generic((Y),      \
        _Lcomplex:   __cpowl_fc_lc, \
        _Fcomplex:   cpowf,         \
        _Dcomplex:   __cpow_fc_dc,  \
        long double: __cpowl_fc_l,  \
        default:     __cpow_fc_d,   \
        float:       __cpowf_fc_f   \
        ),                          \
                                    \
    _Dcomplex:   _Generic((Y),      \
        _Lcomplex:   __cpowl_dc_lc, \
        _Fcomplex:   __cpow_dc_fc,  \
        _Dcomplex:   cpow,          \
        long double: __cpowl_dc_l,  \
        default:     __cpow_dc_d,   \
        float:       __cpow_dc_f   \
        ),                          \
                                    \
    long double: _Generic((Y),      \
        _Lcomplex: __cpowl_l_lc,    \
        _Fcomplex: __cpowl_l_fc,    \
        _Dcomplex: __cpowl_l_dc,    \
        default:   powl             \
        ),                          \
                                    \
    float:       _Generic((Y),      \
        _Lcomplex:   __cpowl_f_lc,  \
        _Fcomplex:   __cpowf_f_fc,  \
        _Dcomplex:   __cpow_f_dc,   \
        long double: powl,          \
        default:     pow,           \
        float:       powf           \
        ),                          \
                                    \
    default:     _Generic((Y),      \
        _Lcomplex:   __cpowl_d_lc,  \
        _Fcomplex:   __cpow_d_fc,   \
        _Dcomplex:   __cpow_d_dc,   \
        long double: powl,          \
        default:     pow            \
        )                           \
)(X, Y)

#define sqrt(X) _Generic((X), \
    _Lcomplex:   csqrtl,      \
    _Fcomplex:   csqrtf,      \
    _Dcomplex:   csqrt,       \
    long double: sqrtl,       \
    float:       sqrtf,       \
    default:     sqrt         \
)(X)

#define sin(X) _Generic((X), \
    _Lcomplex:   csinl,      \
    _Fcomplex:   csinf,      \
    _Dcomplex:   csin,       \
    long double: sinl,       \
    float:       sinf,       \
    default:     sin         \
)(X)

#define cos(X) _Generic((X), \
    _Lcomplex:   ccosl,      \
    _Fcomplex:   ccosf,      \
    _Dcomplex:   ccos,       \
    long double: cosl,       \
    float:       cosf,       \
    default:     cos         \
)(X)

#define tan(X) _Generic((X), \
    _Lcomplex:   ctanl,      \
    _Fcomplex:   ctanf,      \
    _Dcomplex:   ctan,       \
    long double: tanl,       \
    float:       tanf,       \
    default:     tan         \
)(X)

#define asin(X) _Generic((X), \
    _Lcomplex:   casinl,      \
    _Fcomplex:   casinf,      \
    _Dcomplex:   casin,       \
    long double: asinl,       \
    float:       asinf,       \
    default:     asin         \
)(X)

#define acos(X) _Generic((X), \
    _Lcomplex:   cacosl,      \
    _Fcomplex:   cacosf,      \
    _Dcomplex:   cacos,       \
    long double: acosl,       \
    float:       acosf,       \
    default:     acos         \
)(X)

#define atan(X) _Generic((X), \
    _Lcomplex:   catanl,      \
    _Fcomplex:   catanf,      \
    _Dcomplex:   catan,       \
    long double: atanl,       \
    float:       atanf,       \
    default:     atan         \
)(X)

#define asinh(X) _Generic((X), \
    _Lcomplex:   casinhl,      \
    _Fcomplex:   casinhf,      \
    _Dcomplex:   casinh,       \
    long double: asinhl,       \
    float:       asinhf,       \
    default:     asinh         \
)(X)

#define acosh(X) _Generic((X), \
    _Lcomplex:   cacoshl,      \
    _Fcomplex:   cacoshf,      \
    _Dcomplex:   cacosh,       \
    long double: acoshl,       \
    float:       acoshf,       \
    default:     acosh         \
)(X)

#define atanh(X) _Generic((X), \
    _Lcomplex:   catanhl,      \
    _Fcomplex:   catanhf,      \
    _Dcomplex:   catanh,       \
    long double: atanhl,       \
    float:       atanhf,       \
    default:     atanh         \
)(X)

#define atan2(X, Y) _Generic(__tgmath_resolve_real_binary_op((X), (Y)), \
    long double: atan2l,                                                \
    float:       atan2f,                                                \
    default:     atan2                                                  \
)(X, Y)

#define cbrt(X) _Generic((X), \
    long double: cbrtl,       \
    float:       cbrtf,       \
    default:     cbrt         \
)(X)

#define ceil(X) _Generic((X), \
    long double: ceill,       \
    float:       ceilf,       \
    default:     ceil         \
)(X)

#define copysign(X, Y) _Generic(__tgmath_resolve_real_binary_op((X), (Y)), \
    long double: copysignl,                                                \
    float:       copysignf,                                                \
    default:     copysign                                                  \
)(X, Y)

#define erf(X) _Generic((X), \
    long double: erfl,       \
    float:       erff,       \
    default:     erf         \
)(X)

#define erfc(X) _Generic((X), \
    long double: erfcl,       \
    float:       erfcf,       \
    default:     erfc         \
)(X)

#define exp2(X) _Generic((X), \
    long double: exp2l,       \
    float:       exp2f,       \
    default:     exp2         \
)(X)

#define expm1(X) _Generic((X), \
    long double: expm1l,       \
    float:       expm1f,       \
    default:     expm1         \
)(X)

#define fdim(X, Y) _Generic(__tgmath_resolve_real_binary_op((X), (Y)), \
    long double: fdiml,                                                \
    float:       fdimf,                                                \
    default:     fdim                                                  \
)(X, Y)

#define floor(X) _Generic((X), \
    long double: floorl,       \
    float:       floorf,       \
    default:     floor         \
)(X)

#define fma(X, Y, Z) _Generic(__tgmath_resolve_real_binary_op((X), __tgmath_resolve_real_binary_op((Y), (Z))), \
    long double: fmal,                                                                                         \
    float:       fmaf,                                                                                         \
    default:     fma                                                                                           \
)(X, Y, Z)

#define fmax(X, Y) _Generic(__tgmath_resolve_real_binary_op((X), (Y)), \
    long double: fmaxl,                                                \
    float:       fmaxf,                                                \
    default:     fmax                                                  \
)(X, Y)

#define fmin(X, Y) _Generic(__tgmath_resolve_real_binary_op((X), (Y)), \
    long double: fminl,                                                \
    float:       fminf,                                                \
    default:     fmin                                                  \
)(X, Y)

#define fmod(X, Y) _Generic(__tgmath_resolve_real_binary_op((X), (Y)), \
    long double: fmodl,                                                \
    float:       fmodf,                                                \
    default:     fmod                                                  \
)(X, Y)

#define frexp(X, INT_PTR) _Generic((X), \
    long double: frexpl,                \
    float:       frexpf,                \
    default:     frexp                  \
)(X, INT_PTR)

#define hypot(X, Y) _Generic(__tgmath_resolve_real_binary_op((X), (Y)), \
    long double: hypotl,                                                \
    float:       hypotf,                                                \
    default:     hypot                                                  \
)(X, Y)

#define ilogb(X) _Generic((X), \
    long double: ilogbl,       \
    float:       ilogbf,       \
    default:     ilogb         \
)(X)

#define ldexp(X, INT) _Generic((X), \
    long double: ldexpl,            \
    float:       ldexpf,            \
    default:     ldexp              \
)(X, INT)

#define lgamma(X) _Generic((X), \
    long double: lgammal,       \
    float:       lgammaf,       \
    default:     lgamma         \
)(X)

#define llrint(X) _Generic((X), \
    long double: llrintl,       \
    float:       llrintf,       \
    default:     llrint         \
)(X)

#define llround(X) _Generic((X), \
    long double: llroundl,       \
    float:       llroundf,       \
    default:     llround         \
)(X)

#define log10(X) _Generic((X), \
    long double: log10l,       \
    float:       log10f,       \
    default:     log10         \
)(X)

#define log1p(X) _Generic((X), \
    long double: log1pl,       \
    float:       log1pf,       \
    default:     log1p         \
)(X)

#define log2(X) _Generic((X), \
    long double: log2l,       \
    float:       log2f,       \
    default:     log2         \
)(X)

#define logb(X) _Generic((X), \
    long double: logbl,       \
    float:       logbf,       \
    default:     logb         \
)(X)

#define lrint(X) _Generic((X), \
    long double: lrintl,       \
    float:       lrintf,       \
    default:     lrint         \
)(X)

#define lround(X) _Generic((X), \
    long double: lroundl,       \
    float:       lroundf,       \
    default:     lround         \
)(X)

#define nearbyint(X) _Generic((X), \
    long double: nearbyintl,       \
    float:       nearbyintf,       \
    default:     nearbyint         \
)(X)

#define nextafter(X, Y) _Generic(__tgmath_resolve_real_binary_op((X), (Y)), \
    long double: nextafterl,                                                \
    float:       nextafterf,                                                \
    default:     nextafter                                                  \
)(X, Y)

#define nexttoward(X, LONG_DOUBLE) _Generic((X), \
    long double: nexttowardl,                    \
    float:       nexttowardf,                    \
    default:     nexttoward                      \
)(X, LONG_DOUBLE)

#define remainder(X, Y) _Generic(__tgmath_resolve_real_binary_op((X), (Y)), \
    long double: remainderl,                                                \
    float:       remainderf,                                                \
    default:     remainder                                                  \
)(X, Y)

#define remquo(X, Y, INT_PTR) _Generic(__tgmath_resolve_real_binary_op((X), (Y)), \
    long double: remquol,                                                         \
    float:       remquof,                                                         \
    default:     remquo                                                           \
)(X, Y, INT_PTR)

#define rint(X) _Generic((X), \
    long double: rintl,       \
    float:       rintf,       \
    default:     rint         \
)(X)

#define round(X) _Generic((X), \
    long double: roundl,       \
    float:       roundf,       \
    default:     round         \
)(X)

#define scalbln(X, LONG) _Generic((X), \
    long double: scalblnl,             \
    float:       scalblnf,             \
    default:     scalbln               \
)(X, LONG)

#define scalbn(X, INT) _Generic((X), \
    long double: scalbnl,            \
    float:       scalbnf,            \
    default:     scalbn              \
)(X, INT)

#define tgamma(X) _Generic((X), \
    long double: tgammal,       \
    float:       tgammaf,       \
    default:     tgamma         \
)(X)

#define trunc(X) _Generic((X), \
    long double: truncl,       \
    float:       truncf,       \
    default:     trunc         \
)(X)

inline double __carg_d(double const __d)
{
    return carg(_Cbuild(__d, 0.0));
}

#define carg(X) _Generic((X), \
    _Lcomplex: cargl,         \
    _Fcomplex: cargf,         \
    _Dcomplex: carg,          \
    default:   __carg_d       \
)(X)

inline _Dcomplex __conj_d(double const __d)
{
    return conj(_Cbuild(__d, 0.0));
}

#define conj(X) _Generic((X), \
    _Lcomplex: conjl,         \
    _Fcomplex: conjf,         \
    _Dcomplex: conj,          \
    default:   __conj_d       \
)(X)

inline double __creal_d(double const __d)
{
    // The real part of a double casted to a double complex is just the double value.
    return __d;
}

#define creal(X) _Generic((X), \
    _Lcomplex: creall,         \
    _Fcomplex: crealf,         \
    _Dcomplex: creal,          \
    default:   __creal_d       \
)(X)

inline double __cimag_d(double const __d)
{
    // The imaginary part of a double casted to a double complex is 0.
    (void) __d;
    return 0.0;
}

#define cimag(X) _Generic((X), \
    _Lcomplex: cimagl,         \
    _Fcomplex: cimagf,         \
    _Dcomplex: cimag,          \
    default:   __cimag_d       \
)(X)

inline _Dcomplex __cproj_d(double const __d)
{
    return cproj(_Cbuild(__d, 0.0));
}

#define cproj(X) _Generic((X), \
    _Lcomplex: cprojl,         \
    _Fcomplex: cprojf,         \
    _Dcomplex: cproj,          \
    default:   __cproj_d       \
)(X)

_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS

#endif // _CRT_HAS_C11 == 0

#endif // (_CRT_HAS_CXX17 == 1) && !defined(_CRT_USE_C_TGMATH_H)

#endif // _TGMATH
