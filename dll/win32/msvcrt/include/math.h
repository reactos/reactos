/*
 * Math functions.
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Hans Leidekker.
 * This file is in the public domain.
 */

#ifndef __WINE_MATH_H
#define __WINE_MATH_H

#include <corecrt.h>

#include <pshpack8.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _DOMAIN         1       /* domain error in argument */
#define _SING           2       /* singularity */
#define _OVERFLOW       3       /* range overflow */
#define _UNDERFLOW      4       /* range underflow */
#define _TLOSS          5       /* total loss of precision */
#define _PLOSS          6       /* partial loss of precision */

#ifndef _EXCEPTION_DEFINED
#define _EXCEPTION_DEFINED
struct _exception
{
  int     type;
  char    *name;
  double  arg1;
  double  arg2;
  double  retval;
};
#endif /* _EXCEPTION_DEFINED */

#ifndef _COMPLEX_DEFINED
#define _COMPLEX_DEFINED
struct _complex
{
  double x;      /* Real part */
  double y;      /* Imaginary part */
};
#endif /* _COMPLEX_DEFINED */

_ACRTIMP double __cdecl sin(double);
_ACRTIMP double __cdecl cos(double);
_ACRTIMP double __cdecl tan(double);
_ACRTIMP double __cdecl sinh(double);
_ACRTIMP double __cdecl cosh(double);
_ACRTIMP double __cdecl tanh(double);
_ACRTIMP double __cdecl asin(double);
_ACRTIMP double __cdecl acos(double);
_ACRTIMP double __cdecl atan(double);
_ACRTIMP double __cdecl atan2(double, double);
_ACRTIMP double __cdecl asinh(double);
_ACRTIMP double __cdecl acosh(double);
_ACRTIMP double __cdecl atanh(double);
_ACRTIMP double __cdecl exp(double);
_ACRTIMP double __cdecl log(double);
_ACRTIMP double __cdecl log10(double);
_ACRTIMP double __cdecl pow(double, double);
_ACRTIMP double __cdecl sqrt(double);
_ACRTIMP double __cdecl ceil(double);
_ACRTIMP double __cdecl floor(double);
_ACRTIMP double __cdecl fabs(double);
_ACRTIMP double __cdecl ldexp(double, int);
_ACRTIMP double __cdecl frexp(double, int*);
_ACRTIMP double __cdecl modf(double, double*);
_ACRTIMP double __cdecl fdim(double, double);
_ACRTIMP double __cdecl fmod(double, double);
_ACRTIMP double __cdecl fmin(double, double);
_ACRTIMP double __cdecl fmax(double, double);
_ACRTIMP double __cdecl erf(double);
_ACRTIMP double __cdecl remainder(double, double);
_ACRTIMP double __cdecl remquo(double, double, int*);
_ACRTIMP float __cdecl remquof(float, float, int*);
_ACRTIMP double __cdecl lgamma(double);
_ACRTIMP double __cdecl tgamma(double);

_ACRTIMP double __cdecl _hypot(double, double);
_ACRTIMP double __cdecl _j0(double);
_ACRTIMP double __cdecl _j1(double);
_ACRTIMP double __cdecl _jn(int, double);
_ACRTIMP double __cdecl _y0(double);
_ACRTIMP double __cdecl _y1(double);
_ACRTIMP double __cdecl _yn(int, double);

_ACRTIMP double __cdecl cbrt(double);
_ACRTIMP double __cdecl exp2(double);
_ACRTIMP double __cdecl expm1(double);
_ACRTIMP double __cdecl log1p(double);
_ACRTIMP double __cdecl log2(double);
_ACRTIMP double __cdecl logb(double);
_ACRTIMP double __cdecl rint(double);
_ACRTIMP double __cdecl round(double);
_ACRTIMP double __cdecl trunc(double);

_ACRTIMP float __cdecl cbrtf(float);
_ACRTIMP float __cdecl exp2f(float);
_ACRTIMP float __cdecl expm1f(float);
_ACRTIMP float __cdecl log1pf(float);
_ACRTIMP float __cdecl log2f(float);
_ACRTIMP float __cdecl logbf(float);
_ACRTIMP float __cdecl rintf(float);
_ACRTIMP float __cdecl roundf(float);
_ACRTIMP float __cdecl truncf(float);

_ACRTIMP int __cdecl ilogb(double);
_ACRTIMP int __cdecl ilogbf(float);

_ACRTIMP float __cdecl fmaf(float x, float y, float z);
_ACRTIMP double __cdecl fma(double x, double y, double z);

_ACRTIMP __int64 __cdecl llrint(double);
_ACRTIMP __int64 __cdecl llrintf(float);
_ACRTIMP __int64 __cdecl llround(double);
_ACRTIMP __int64 __cdecl llroundf(float);
_ACRTIMP __msvcrt_long __cdecl lrint(double);
_ACRTIMP __msvcrt_long __cdecl lrintf(float);
_ACRTIMP __msvcrt_long __cdecl lround(double);
_ACRTIMP __msvcrt_long __cdecl lroundf(float);

_ACRTIMP double __cdecl scalbn(double,int);
_ACRTIMP float  __cdecl scalbnf(float,int);
_ACRTIMP double __cdecl scalbln(double,__msvcrt_long);
_ACRTIMP float  __cdecl scalblnf(float,__msvcrt_long);

_ACRTIMP double __cdecl _copysign (double, double);
_ACRTIMP double __cdecl _chgsign (double);
_ACRTIMP double __cdecl _scalb(double, __msvcrt_long);
_ACRTIMP double __cdecl _logb(double);
_ACRTIMP double __cdecl _nextafter(double, double);
_ACRTIMP int    __cdecl _finite(double);
_ACRTIMP int    __cdecl _isnan(double);
_ACRTIMP int    __cdecl _fpclass(double);

_ACRTIMP double __cdecl nextafter(double, double);

#if !defined(__i386__) || defined(_NO_CRT_MATH_INLINE)

_ACRTIMP float __cdecl sinf(float);
_ACRTIMP float __cdecl cosf(float);
_ACRTIMP float __cdecl tanf(float);
_ACRTIMP float __cdecl sinhf(float);
_ACRTIMP float __cdecl coshf(float);
_ACRTIMP float __cdecl tanhf(float);
_ACRTIMP float __cdecl asinf(float);
_ACRTIMP float __cdecl acosf(float);
_ACRTIMP float __cdecl atanf(float);
_ACRTIMP float __cdecl atan2f(float, float);
_ACRTIMP float __cdecl atanhf(float);
_ACRTIMP float __cdecl expf(float);
_ACRTIMP float __cdecl logf(float);
_ACRTIMP float __cdecl log10f(float);
_ACRTIMP float __cdecl powf(float, float);
_ACRTIMP float __cdecl sqrtf(float);
_ACRTIMP float __cdecl ceilf(float);
_ACRTIMP float __cdecl floorf(float);
_ACRTIMP float __cdecl modff(float, float*);
_ACRTIMP float __cdecl fmodf(float, float);

_ACRTIMP int   __cdecl _finitef(float);
_ACRTIMP int   __cdecl _isnanf(float);
_ACRTIMP int   __cdecl _fpclassf(float);

#else

static inline float sinf(float x) { return sin(x); }
static inline float cosf(float x) { return cos(x); }
static inline float tanf(float x) { return tan(x); }
static inline float sinhf(float x) { return sinh(x); }
static inline float coshf(float x) { return cosh(x); }
static inline float tanhf(float x) { return tanh(x); }
static inline float asinf(float x) { return asin(x); }
static inline float acosf(float x) { return acos(x); }
static inline float atanf(float x) { return atan(x); }
static inline float atan2f(float x, float y) { return atan2(x, y); }
static inline float expf(float x) { return exp(x); }
static inline float logf(float x) { return log(x); }
static inline float log10f(float x) { return log10(x); }
static inline float powf(float x, float y) { return pow(x, y); }
static inline float sqrtf(float x) { return sqrt(x); }
static inline float ceilf(float x) { return ceil(x); }
static inline float floorf(float x) { return floor(x); }
static inline float modff(float x, float *y) { double yd, ret = modf(x, &yd); *y = yd; return ret; }
static inline float fmodf(float x, float y) { return fmod(x, y); }

static inline int   _finitef(float x) { return _finite(x); }
static inline int   _isnanf(float x) { return _isnan(x); }

static inline int   _fpclassf(float x)
{
    unsigned int ix = *(int*)&x;
    double d = x;

    /* construct denormal double */
    if (!(ix >> 23 & 0xff) && (ix << 1))
    {
        unsigned __int64 id = (((unsigned __int64)ix >> 31) << 63) | 1;
        d = *(double*)&id;
    }
    return _fpclass(d);
}

#endif

#if (defined(__x86_64__) && !defined(_UCRT)) || defined(_NO_CRT_MATH_INLINE)
_ACRTIMP float __cdecl frexpf(float, int*);
#else
static inline float frexpf(float x, int *y) { return frexp(x, y); }
#endif

#if (!defined(__i386__) && !defined(__x86_64__) && (_MSVCR_VER == 0 || _MSVCR_VER >= 110)) || defined(_NO_CRT_MATH_INLINE)
_ACRTIMP float __cdecl fabsf(float);
#else
static inline float fabsf(float x) { return fabs(x); }
#endif

#if !defined(__i386__) || _MSVCR_VER>=120 || defined(_NO_CRT_MATH_INLINE)

_ACRTIMP float __cdecl _chgsignf(float);
_ACRTIMP float __cdecl _copysignf(float, float);
_ACRTIMP float __cdecl _logbf(float);
_ACRTIMP float __cdecl acoshf(float);
_ACRTIMP float __cdecl asinhf(float);
_ACRTIMP float __cdecl atanhf(float);
_ACRTIMP float __cdecl erff(float);
_ACRTIMP float __cdecl fdimf(float, float);
_ACRTIMP float __cdecl fmaxf(float, float);
_ACRTIMP float __cdecl fminf(float, float);
_ACRTIMP float __cdecl lgammaf(float);
_ACRTIMP float __cdecl nextafterf(float, float);
_ACRTIMP float __cdecl remainderf(float, float);
_ACRTIMP float __cdecl tgammaf(float);

#else

static inline float _chgsignf(float x) { return _chgsign(x); }
static inline float _copysignf(float x, float y) { return _copysign(x, y); }
static inline float _logbf(float x) { return _logb(x); }

#endif

static inline float ldexpf(float x, int y) { return ldexp(x, y); }

#ifdef _UCRT
_ACRTIMP double __cdecl copysign(double, double);
_ACRTIMP float  __cdecl copysignf(float, float);
#else
#define copysign(x,y)  _copysign(x,y)
#define copysignf(x,y) _copysignf(x,y)
#endif

_ACRTIMP double __cdecl nearbyint(double);
_ACRTIMP float __cdecl nearbyintf(float);
_ACRTIMP float __cdecl _hypotf(float, float);
_ACRTIMP int __cdecl _matherr(struct _exception*);
_ACRTIMP double __cdecl _cabs(struct _complex);

#if (defined(__GNUC__) && ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 3)))) || defined(__clang__)
# define INFINITY __builtin_inff()
# define NAN      __builtin_nanf("")
# define HUGE_VAL __builtin_huge_val()
#else
static const union {
    unsigned int __i;
    float __f;
} __inff = { 0x7f800000 }, __nanf = { 0x7fc00000 };
# define INFINITY (__inff.__f)
# define NAN      (__nanf.__f)
# define HUGE_VAL ((double)INFINITY)
#endif

#define FP_INFINITE   1
#define FP_NAN        2
#define FP_NORMAL    -1
#define FP_SUBNORMAL -2
#define FP_ZERO       0

#define _C2 1
#define FP_ILOGB0 (-0x7fffffff - _C2)
#define FP_ILOGBNAN 0x7fffffff

_ACRTIMP short __cdecl _dtest(double*);
_ACRTIMP short __cdecl _ldtest(long double*);
_ACRTIMP short __cdecl _fdtest(float*);

#ifdef __cplusplus

extern "C++" {
inline int fpclassify(float x) throw() { return _fdtest(&x); }
inline int fpclassify(double x) throw() { return _dtest(&x); }
inline int fpclassify(long double x) throw() { return _ldtest(&x); }
template <class T> inline bool isfinite(T x) throw() { return fpclassify(x) <= 0; }
template <class T> inline bool isinf(T x) throw() { return fpclassify(x) == FP_INFINITE; }
template <class T> inline bool isnan(T x) throw() { return fpclassify(x) == FP_NAN; }
} /* extern "C++" */

#elif _MSVCR_VER >= 120

_ACRTIMP short __cdecl _dclass(double);
_ACRTIMP short __cdecl _fdclass(float);
_ACRTIMP int   __cdecl _dsign(double);
_ACRTIMP int   __cdecl _fdsign(float);

#define fpclassify(x) (sizeof(x) == sizeof(float) ? _fdclass(x) : _dclass(x))
#define signbit(x)    (sizeof(x) == sizeof(float) ? _fdsign(x) : _dsign(x))
#define isinf(x)      (fpclassify(x) == FP_INFINITE)
#define isnan(x)      (fpclassify(x) == FP_NAN)
#define isnormal(x)   (fpclassify(x) == FP_NORMAL)
#define isfinite(x)   (fpclassify(x) <= 0)

#else

static inline int __isnanf(float x)
{
    union { float x; unsigned int i; } u = { x };
    return (u.i & 0x7fffffff) > 0x7f800000;
}
static inline int __isnan(double x)
{
    union { double x; unsigned __int64 i; } u = { x };
    return (u.i & ~0ull >> 1) > 0x7ffull << 52;
}
static inline int __isinff(float x)
{
    union { float x; unsigned int i; } u = { x };
    return (u.i & 0x7fffffff) == 0x7f800000;
}
static inline int __isinf(double x)
{
    union { double x; unsigned __int64 i; } u = { x };
    return (u.i & ~0ull >> 1) == 0x7ffull << 52;
}
static inline int __isnormalf(float x)
{
    union { float x; unsigned int i; } u = { x };
    return ((u.i + 0x00800000) & 0x7fffffff) >= 0x01000000;
}
static inline int __isnormal(double x)
{
    union { double x; unsigned __int64 i; } u = { x };
    return ((u.i + (1ull << 52)) & ~0ull >> 1) >= 1ull << 53;
}
static inline int __signbitf(float x)
{
    union { float x; unsigned int i; } u = { x };
    return (int)(u.i >> 31);
}
static inline int __signbit(double x)
{
    union { double x; unsigned __int64 i; } u = { x };
    return (int)(u.i >> 63);
}

#define isinf(x)    (sizeof(x) == sizeof(float) ? __isinff(x) : __isinf(x))
#define isnan(x)    (sizeof(x) == sizeof(float) ? __isnanf(x) : __isnan(x))
#define isnormal(x) (sizeof(x) == sizeof(float) ? __isnormalf(x) : __isnormal(x))
#define signbit(x)  (sizeof(x) == sizeof(float) ? __signbitf(x) : __signbit(x))
#define isfinite(x) (!isinf(x) && !isnan(x))

#endif

#ifdef _UCRT

 _ACRTIMP int __cdecl _dpcomp(double, double);
 _ACRTIMP int __cdecl _fdpcomp(float, float);

#define _FP_LT  1
#define _FP_EQ  2
#define _FP_GT  4

#if defined(__GNUC__) || defined(__clang__)
# define isgreater(x, y)      __builtin_isgreater(x, y)
# define isgreaterequal(x, y) __builtin_isgreaterequal(x, y)
# define isless(x, y)         __builtin_isless(x, y)
# define islessequal(x, y)    __builtin_islessequal(x, y)
# define islessgreater(x, y)  __builtin_islessgreater(x, y)
# define isunordered(x, y)    __builtin_isunordered(x, y)
#else
# define __FP_COMPARE(x,y) (sizeof(x) == sizeof(float) && sizeof(y) == sizeof(float) ? _fdpcomp(x,y) : _dpcomp(x,y))
# define isgreater(x, y)      ((__FP_COMPARE(x, y) & _FP_GT) != 0)
# define isgreaterequal(x, y) ((__FP_COMPARE(x, y) & (_FP_GT|_FP_EQ)) != 0)
# define isless(x, y)         ((__FP_COMPARE(x, y) & _FP_LT) != 0)
# define islessequal(x, y)    ((__FP_COMPARE(x, y) & (_FP_LT|_FP_EQ)) != 0)
# define islessgreater(x, y)  ((__FP_COMPARE(x, y) & (_FP_LT|_FP_GT)) != 0)
# define isunordered(x, y)    (!__FP_COMPARE(x, y))
#endif

#endif /* _UCRT */

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#if !defined(__STRICT_ANSI__) || defined(_POSIX_C_SOURCE) || defined(_POSIX_SOURCE) || defined(_XOPEN_SOURCE) || defined(_GNU_SOURCE) || defined(_BSD_SOURCE) || defined(_USE_MATH_DEFINES)
#ifndef _MATH_DEFINES_DEFINED
#define _MATH_DEFINES_DEFINED
#define M_E         2.71828182845904523536
#define M_LOG2E     1.44269504088896340736
#define M_LOG10E    0.434294481903251827651
#define M_LN2       0.693147180559945309417
#define M_LN10      2.30258509299404568402
#define M_PI        3.14159265358979323846
#define M_PI_2      1.57079632679489661923
#define M_PI_4      0.785398163397448309616
#define M_1_PI      0.318309886183790671538
#define M_2_PI      0.636619772367581343076
#define M_2_SQRTPI  1.12837916709551257390
#define M_SQRT2     1.41421356237309504880
#define M_SQRT1_2   0.707106781186547524401
#endif /* !_MATH_DEFINES_DEFINED */
#endif /* _USE_MATH_DEFINES */

static inline double hypot( double x, double y ) { return _hypot( x, y ); }
static inline double j0( double x ) { return _j0( x ); }
static inline double j1( double x ) { return _j1( x ); }
static inline double jn( int n, double x ) { return _jn( n, x ); }
static inline double y0( double x ) { return _y0( x ); }
static inline double y1( double x ) { return _y1( x ); }
static inline double yn( int n, double x ) { return _yn( n, x ); }

static inline float hypotf( float x, float y ) { return _hypotf( x, y ); }
static inline long double atan2l( long double x, long double y ) { return atan2( (double)y, (double)x ); }

#endif /* __WINE_MATH_H */
