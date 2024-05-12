/**
 * This file has no copyright assigned and is placed in the Public Domain.
 */
#ifndef _INC_MATH
#define _INC_MATH

#include <corecrt.h>

#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif

typedef float float_t;
typedef double double_t;

#ifndef _EXCEPTION_DEFINED
#define _EXCEPTION_DEFINED
  struct _exception {
    int type;
    char *name;
    double arg1;
    double arg2;
    double retval;
  };
#endif

#ifndef _COMPLEX_DEFINED
#define _COMPLEX_DEFINED
  struct _complex {
    double x,y;
  };
#if !__STDC__ && !defined(__cplusplus)
#define complex _complex
#endif /* __STDC__ */
#endif /* _COMPLEX_DEFINED */

#define _DOMAIN 1
#define _SING 2
#define _OVERFLOW 3
#define _UNDERFLOW 4
#define _TLOSS 5
#define _PLOSS 6

#define EDOM 33
#define ERANGE 34

  _CRTIMP extern double const _HUGE;

#define HUGE_VAL _HUGE

#ifndef _HUGE_ENUF
#define _HUGE_ENUF 1e+300
#endif
#define INFINITY  ((float)(_HUGE_ENUF * _HUGE_ENUF))
#define HUGE_VALD ((double)INFINITY)
#define HUGE_VALF ((float)INFINITY)
#define HUGE_VALL ((long double)INFINITY)
#define NAN       ((float)(INFINITY * 0.0F))

#define _DENORM  (-2)
#define _FINITE  (-1)
#define _INFCODE 1
#define _NANCODE 2

#define FP_INFINITE  _INFCODE
#define FP_NAN       _NANCODE
#define FP_NORMAL    _FINITE
#define FP_SUBNORMAL _DENORM
#define FP_ZERO      0

#ifndef __cplusplus
#define _matherrl _matherr
#endif

#ifndef _CRT_ABS_DEFINED
#define _CRT_ABS_DEFINED
_Check_return_ int __cdecl abs(_In_ int x);
_Check_return_ long __cdecl labs(_In_ long x);
_Check_return_ long long __cdecl llabs(_In_ long long x);
#endif

_Check_return_ double __cdecl acos(_In_ double x);
_Check_return_ double __cdecl asin(_In_ double x);
_Check_return_ double __cdecl atan(_In_ double x);
_Check_return_ double __cdecl atan2(_In_ double y, _In_ double x);
_Check_return_ double __cdecl cos(_In_ double x);
_Check_return_ double __cdecl cosh(_In_ double x);
_Check_return_ double __cdecl exp(_In_ double x);
_Check_return_ _CRT_JIT_INTRINSIC double __cdecl fabs(_In_ double x);
_Check_return_ double __cdecl fmod(_In_ double x, _In_ double y);
_Check_return_ double __cdecl log(_In_ double x);
_Check_return_ double __cdecl log10(_In_ double x);
_Check_return_ double __cdecl pow(_In_ double x, double y);
_Check_return_ double __cdecl sin(_In_ double x);
_Check_return_ double __cdecl sinh(_In_ double x);
_Check_return_ _CRT_JIT_INTRINSIC double __cdecl sqrt(_In_ double x);
_Check_return_ double __cdecl tan(_In_ double x);
_Check_return_ double __cdecl tanh(_In_ double x);

#ifdef _MSC_VER
/* Prevent the compiler from generating calls to _CIatan2 */
#pragma function(atan2)
#ifdef _M_AMD64
/* Prevent the compiler from generating calls to __vdecl_xxx */
#pragma function(cos,pow,sin,tan)
#endif
#endif

#ifndef _CRT_MATHERR_DEFINED
#define _CRT_MATHERR_DEFINED
int __CRTDECL _matherr(_Inout_ struct _exception *exception);
#endif

#ifndef _CRT_ATOF_DEFINED
#define _CRT_ATOF_DEFINED
_Check_return_ _CRTIMP double __cdecl atof(_In_z_ const char *str);
_Check_return_ _CRTIMP double __cdecl _atof_l(_In_z_ const char *str, _In_opt_ _locale_t locale); // vista+
#endif /* _CRT_ATOF_DEFINED */

#ifndef _SIGN_DEFINED
#define _SIGN_DEFINED
_Check_return_ _CRTIMP double __cdecl _copysign(_In_ double x, _In_ double sgn);
_Check_return_ _CRTIMP double __cdecl _chgsign(_In_ double x);
#endif

_Check_return_ _CRTIMP double __cdecl _cabs(_In_ struct _complex a);
_Check_return_ _CRTIMP double __cdecl _hypot(_In_ double x, _In_ double y);
_Check_return_ _CRTIMP double __cdecl _j0(_In_ double x);
_Check_return_ _CRTIMP double __cdecl _j1(_In_ double x);
_Check_return_ _CRTIMP double __cdecl _jn(_In_ int x, _In_ double y);
_Check_return_ _CRTIMP double __cdecl _nextafter(_In_ double x, _In_ double y);
_Check_return_ _CRTIMP double __cdecl _y0(_In_ double x);
_Check_return_ _CRTIMP double __cdecl _y1(_In_ double x);
_Check_return_ _CRTIMP double __cdecl _yn(_In_ int x, _In_ double y);
_Check_return_ _CRTIMP double __cdecl ceil(_In_ double x);
_Check_return_ _CRTIMP double __cdecl floor(_In_ double x);
_Check_return_ _CRTIMP double __cdecl frexp(_In_ double x, _Out_ int *y);
_Check_return_ _CRTIMP double __cdecl ldexp(_In_ double x, _In_ int y);
_Check_return_ _CRTIMP double __cdecl modf(_In_ double x, _Out_ double *y);

#if defined(__i386__) || defined(_M_IX86)
_Check_return_ _CRTIMP int __cdecl _set_SSE2_enable(_In_ int flag);
#endif

#if defined(__x86_64) || defined(_M_AMD64)
_Check_return_ _CRTIMP float __cdecl _nextafterf(_In_ float x, _In_ float y);
_Check_return_ _CRTIMP int __cdecl _isnanf(_In_ float x);
_Check_return_ _CRTIMP int __cdecl _fpclassf(_In_ float x);
#ifdef _MSC_VER
/* Prevent the compiler from generating calls to __vdecl_floor2 */
#pragma function(floor)
#endif
#endif

#if defined(__x86_64) || defined(_M_AMD64) || \
    defined(__arm__) || defined(_M_ARM)  || \
    defined(__arm64__) || defined(_M_ARM64)
_Check_return_ _CRTIMP int __cdecl _finitef(_In_ float x);
_Check_return_ _CRTIMP float __cdecl _logbf(_In_ float x);
#endif /* _M_AMD64 || _M_ARM || _M_ARM64 */

#if defined(__ia64__) || defined (_M_IA64)
_Check_return_ _CRTIMP float __cdecl ldexpf(_In_ float x, _In_ int y);
_Check_return_ _CRTIMP long double __cdecl tanl(_In_ long double x);
#else
_Check_return_ __CRT_INLINE float __CRTDECL ldexpf(_In_ float x, _In_ int y) { return (float)ldexp(x, y); }
_Check_return_ __CRT_INLINE long double __CRTDECL tanl(_In_ long double x) { return (tan((double)x)); }
#endif

#if defined(_CRTBLD)
_Check_return_ float __cdecl fabsf(_In_ float x);
#elif defined(__ia64__) || defined(_M_IA64) || \
    defined(__arm__) || defined(_M_ARM)  || \
    defined(__arm64__) || defined(_M_ARM64)
_Check_return_ _CRT_JIT_INTRINSIC _CRTIMP float __cdecl fabsf(_In_ float x);
#else
_Check_return_ __CRT_INLINE float __CRTDECL fabsf(_In_ float x) { return ((float)fabs((double)x)); }
#endif /* _M_IA64 || _M_ARM || _M_ARM64 */

_Check_return_ float __cdecl _chgsignf(_In_ float x);
_Check_return_ float __cdecl _copysignf(_In_ float x, _In_ float y);
_Check_return_ float __cdecl _hypotf(_In_ float x, _In_ float y);
_Check_return_ float __cdecl acosf(_In_ float x);
_Check_return_ float __cdecl asinf(_In_ float x);
_Check_return_ float __cdecl atanf(_In_ float x);
_Check_return_ float __cdecl atan2f(_In_ float x, _In_ float y);
_Check_return_ float __cdecl ceilf(_In_ float x);
_Check_return_ float __cdecl cosf(_In_ float x);
_Check_return_ float __cdecl coshf(_In_ float x);
_Check_return_ float __cdecl expf(_In_ float x);
_Check_return_ float __cdecl floorf(_In_ float x);
_Check_return_ float __cdecl fmodf(_In_ float x, _In_ float y);
_Check_return_ float __cdecl logf(_In_ float x);
_Check_return_ float __cdecl log10f(_In_ float x);
_Check_return_ float __cdecl modff(_In_ float x, _Out_ float *y);
_Check_return_ float __cdecl powf(_In_ float b, _In_ float e);
_Check_return_ float __cdecl sinf(_In_ float x);
_Check_return_ float __cdecl sinhf(_In_ float x);
_Check_return_ float __cdecl sqrtf(_In_ float x);
_Check_return_ float __cdecl tanf(_In_ float x);
_Check_return_ float __cdecl tanhf(_In_ float x);

#if defined(_MSC_VER)
/* Make sure intrinsics don't get in our way */
#if defined(_M_AMD64) || defined(_M_ARM) || defined(_M_ARM64)
#pragma function(acosf,asinf,atanf,atan2f,ceilf,cosf,coshf,expf,floorf,fmodf,logf,log10f,powf,sinf,sinhf,sqrtf,tanf,tanhf)
#endif /* defined(_M_AMD64) || defined(_M_ARM) || defined(_M_ARM64) */
#if (_MSC_VER >= 1920)
#pragma function(_hypotf)
#endif
#endif /* _MSC_VER */

#if !defined(_CRTBLD)
_Check_return_ __CRT_INLINE float _chgsignf(_In_ float x) { return (float)_chgsign((double)x); }
_Check_return_ __CRT_INLINE float _copysignf(_In_ float x, _In_ float y) { return (float)_copysign((double)x, (double)y); }
_Check_return_ __CRT_INLINE float _hypotf(_In_ float x, _In_ float y) { return (float)_hypot((double)x, (double)y); }
_Check_return_ __CRT_INLINE float acosf(_In_ float x) { return (float)acos((double)x); }
_Check_return_ __CRT_INLINE float asinf(_In_ float x) { return (float)asin((double)x); }
_Check_return_ __CRT_INLINE float atanf(_In_ float x) { return (float)atan((double)x); }
_Check_return_ __CRT_INLINE float atan2f(_In_ float x, _In_ float y) { return (float)atan2((double)x,(double)y); }
_Check_return_ __CRT_INLINE float ceilf(_In_ float x) { return (float)ceil((double)x); }
_Check_return_ __CRT_INLINE float cosf(_In_ float x) { return (float)cos((double)x); }
_Check_return_ __CRT_INLINE float coshf(_In_ float x) { return (float)cosh((double)x); }
_Check_return_ __CRT_INLINE float expf(_In_ float x) { return (float)exp((double)x); }
_Check_return_ __CRT_INLINE float floorf(_In_ float x) { return (float)floor((double)x); }
_Check_return_ __CRT_INLINE float fmodf(_In_ float x, _In_ float y) { return (float)fmod((double)x,(double)y); }
_Check_return_ __CRT_INLINE float logf(_In_ float x) { return (float)log((double)x); }
_Check_return_ __CRT_INLINE float log10f(_In_ float x) { return (float)log10((double)x); }
_Check_return_ __CRT_INLINE float modff(_In_ float x, _Out_ float *y) { double _Di,_Df = modf((double)x,&_Di); *y = (float)_Di; return (float)_Df; }
_Check_return_ __CRT_INLINE float powf(_In_ float x, _In_ float y) { return (float)pow((double)x,(double)y); }
_Check_return_ __CRT_INLINE float sinf(_In_ float x) { return (float)sin((double)x); }
_Check_return_ __CRT_INLINE float sinhf(_In_ float x) { return (float)sinh((double)x); }
_Check_return_ __CRT_INLINE float sqrtf(_In_ float x) { return (float)sqrt((double)x); }
_Check_return_ __CRT_INLINE float tanf(_In_ float x) { return (float)tan((double)x); }
_Check_return_ __CRT_INLINE float tanhf(_In_ float x) { return (float)tanh((double)x); }
#endif /* !defined(_CRTBLD) */

_Check_return_ __CRT_INLINE double hypot(_In_ double x, _In_ double y) { return _hypot(x, y); }
_Check_return_ __CRT_INLINE float hypotf(_In_ float x, _In_ float y) { return _hypotf(x, y); }
_Check_return_ __CRT_INLINE float frexpf(_In_ float x, _Out_ int *y) { return ((float)frexp((double)x,y)); }

/* long double equals double, so just use inline wrappers */
_Check_return_ __CRT_INLINE long double acosl(_In_ long double x) { return (acos((double)x)); }
_Check_return_ __CRT_INLINE long double asinl(_In_ long double x) { return (asin((double)x)); }
_Check_return_ __CRT_INLINE long double atanl(_In_ long double x) { return (atan((double)x)); }
_Check_return_ __CRT_INLINE long double atan2l(_In_ long double y, _In_ long double x) { return (atan2((double)y, (double)x)); }
_Check_return_ __CRT_INLINE long double ceill(_In_ long double x) { return (ceil((double)x)); }
_Check_return_ __CRT_INLINE long double cosl(_In_ long double x) { return (cos((double)x)); }
_Check_return_ __CRT_INLINE long double coshl(_In_ long double x) { return (cosh((double)x)); }
_Check_return_ __CRT_INLINE long double expl(_In_ long double x) { return (exp((double)x)); }
_Check_return_ __CRT_INLINE long double fabsl(_In_ long double x) { return fabs((double)x); }
_Check_return_ __CRT_INLINE long double floorl(_In_ long double x) { return (floor((double)x)); }
_Check_return_ __CRT_INLINE long double fmodl(_In_ long double x, _In_ long double y) { return (fmod((double)x, (double)y)); }
_Check_return_ __CRT_INLINE long double frexpl(_In_ long double x, _Out_ int *y) { return (frexp((double)x, y)); }
_Check_return_ __CRT_INLINE long double hypotl(_In_ long double x, _In_ long double y) { return _hypot((double)x, (double)y); }
_Check_return_ __CRT_INLINE long double logl(_In_ long double x) { return (log((double)x)); }
_Check_return_ __CRT_INLINE long double log10l(_In_ long double x) { return (log10((double)x)); }
_Check_return_ __CRT_INLINE long double powl(_In_ long double x, _In_ long double y) { return (pow((double)x, (double)y)); }
_Check_return_ __CRT_INLINE long double sinl(_In_ long double x) { return (sin((double)x)); }
_Check_return_ __CRT_INLINE long double sinhl(_In_ long double x) { return (sinh((double)x)); }
_Check_return_ __CRT_INLINE long double sqrtl(_In_ long double x) { return (sqrt((double)x)); }
_Check_return_ __CRT_INLINE long double tanhl(_In_ long double x) {return (tanh((double)x)); }
_Check_return_ __CRT_INLINE long double _chgsignl(_In_ long double number) { return _chgsign((double)number); }
_Check_return_ __CRT_INLINE long double _copysignl(_In_ long double number, _In_ long double sign) { return _copysign((double)number, (double)sign); }
_Check_return_ __CRT_INLINE long double _hypotl(_In_ long double x, _In_ long double y) { return _hypot((double)x, (double)y); }
_Check_return_ __CRT_INLINE long double ldexpl(_In_ long double x, _In_ int y) { return ldexp((double)x, y); }
_Check_return_ __CRT_INLINE long double modfl(_In_ long double x, _Out_ long double *y) { return (long double)modf((double)x, (double *)y); }

/* Support for some functions, not exported in MSVCRT */
#if (_MSC_VER >= 1929)
_Check_return_ long lrint(_In_ double x);
_Check_return_ long lrintf(_In_ float x);
_Check_return_ long lrintl(_In_ long double x);
#pragma function(lrint, lrintf, lrintl)
#endif

#ifndef _CRTBLD
_Check_return_ __CRT_INLINE double round(_In_ double x) { return (x < 0) ? ceil(x - 0.5f) : floor(x + 0.5); }
_Check_return_ __CRT_INLINE float roundf(_In_ float x) { return (x < 0) ? ceilf(x - 0.5f) : floorf(x + 0.5); }
_Check_return_ __CRT_INLINE long double roundl(_In_ long double x) { return (x < 0) ? ceill(x - 0.5f) : floorl(x + 0.5); }
_Check_return_ __CRT_INLINE long lround(_In_ double x) { return (long)((x < 0) ? (x - 0.5f) : (x + 0.5)); }
_Check_return_ __CRT_INLINE long lroundf(_In_ float x) { return (long)((x < 0) ? (x - 0.5f) : (x + 0.5)); }
_Check_return_ __CRT_INLINE long lroundl(_In_ long double x) { return (long)((x < 0) ? (x - 0.5f) : (x + 0.5)); }
_Check_return_ __CRT_INLINE long long llround(_In_ double x) { return (long long)((x < 0) ? (x - 0.5f) : (x + 0.5)); }
_Check_return_ __CRT_INLINE long long llroundf(_In_ float x) { return (long long)((x < 0) ? (x - 0.5f) : (x + 0.5)); }
_Check_return_ __CRT_INLINE long long llroundl(_In_ long double x) { return (long long)((x < 0) ? (x - 0.5f) : (x + 0.5)); }
_Check_return_ __CRT_INLINE double rint(_In_ double x) { return round(x); }
_Check_return_ __CRT_INLINE float rintf(_In_ float x) { return roundf(x); }
_Check_return_ __CRT_INLINE long double rintl(_In_ long double x) { return roundl(x); }
_Check_return_ __CRT_INLINE long lrint(_In_ double x) { return (long)((x < 0) ? (x - 0.5f) : (x + 0.5)); }
_Check_return_ __CRT_INLINE long lrintf(_In_ float x) { return (long)((x < 0) ? (x - 0.5f) : (x + 0.5)); }
_Check_return_ __CRT_INLINE long lrintl(_In_ long double x) { return (long)((x < 0) ? (x - 0.5f) : (x + 0.5)); }
_Check_return_ __CRT_INLINE long long llrint(_In_ double x) { return (long long)((x < 0) ? (x - 0.5f) : (x + 0.5)); }
_Check_return_ __CRT_INLINE long long llrintf(_In_ float x) { return (long long)((x < 0) ? (x - 0.5f) : (x + 0.5)); }
_Check_return_ __CRT_INLINE long long llrintl(_In_ long double x) { return (long long)((x < 0) ? (x - 0.5f) : (x + 0.5)); }
#ifdef _MSC_VER
#define log2 _log2 // nasty hack, see CORE-18255
#endif
_Check_return_ __CRT_INLINE double log2(_In_ double x) { return log(x) / log(2); }
#endif /* !_CRTBLD */

#ifndef NO_OLDNAMES /* !__STDC__ */

#define DOMAIN _DOMAIN
#define SING _SING
#define OVERFLOW _OVERFLOW
#define UNDERFLOW _UNDERFLOW
#define TLOSS _TLOSS
#define PLOSS _PLOSS
#define matherr _matherr
//_CRTIMP extern double HUGE;
#define HUGE _HUGE
//_CRT_NONSTDC_DEPRECATE(_cabs) _CRTIMP double  __cdecl cabs(_In_ struct _complex x);
#define cabs _cabs

_CRT_NONSTDC_DEPRECATE(_j0) _CRTIMP double __cdecl j0(_In_ double x);
_CRT_NONSTDC_DEPRECATE(_j1) _CRTIMP double __cdecl j1(_In_ double x);
_CRT_NONSTDC_DEPRECATE(_jn) _CRTIMP double __cdecl jn(_In_ int x, _In_ double y);
_CRT_NONSTDC_DEPRECATE(_y0) _CRTIMP double __cdecl y0(_In_ double x);
_CRT_NONSTDC_DEPRECATE(_y1) _CRTIMP double __cdecl y1(_In_ double x);
_CRT_NONSTDC_DEPRECATE(_yn) _CRTIMP double __cdecl yn(_In_ int x, _In_ double y);

#endif /* NO_OLDNAMES */

#ifdef __cplusplus
}
#ifndef _CMATH_
extern "C++" {

//inline long abs(_In_ long x) { return labs(x); }
_Check_return_ inline double abs(_In_ double x)  throw() { return fabs(x); }

_Check_return_ inline float abs(_In_ float x) throw() { return fabsf(x); }
_Check_return_ inline float acos(_In_ float x) throw() { return acosf(x); }
_Check_return_ inline float asin(_In_ float x) throw() { return asinf(x); }
_Check_return_ inline float atan(_In_ float x) throw() { return atanf(x); }
_Check_return_ inline float atan2(_In_ float y, _In_ float x) throw() { return atan2f(y, x); }
_Check_return_ inline float ceil(_In_ float x) throw() { return ceilf(x); }
_Check_return_ inline float copysign(_In_ float x, _In_ float y) throw() { return _copysignf(x, y); }
_Check_return_ inline float cos(_In_ float x) throw() { return cosf(x); }
_Check_return_ inline float cosh(_In_ float x) throw() { return coshf(x); }
_Check_return_ inline float exp(_In_ float x) throw() { return expf(x); }
_Check_return_ inline float fabs(_In_ float x) throw() { return fabsf(x); }
_Check_return_ inline float floor(_In_ float x) throw() { return floorf(x); }
_Check_return_ inline float fmod(_In_ float x, _In_ float y) throw() { return fmodf(x, y); }
_Check_return_ inline float frexp(_In_ float x, _Out_ int * y) throw() { return frexpf(x, y); }
_Check_return_ inline float hypot(_In_ float x, _In_ float y) throw() { return _hypotf(x, y); }
_Check_return_ inline float ldexp(_In_ float x, _In_ int y) throw() { return ldexpf(x, y); }
_Check_return_ inline float log(_In_ float x) throw() { return logf(x); }
_Check_return_ inline float log10(_In_ float x) throw() { return log10f(x); }
_Check_return_ inline float modf(_In_ float x, _Out_ float * y) throw() { return modff(x, y); }
_Check_return_ inline float pow(_In_ float x, _In_ float y) throw() { return powf(x, y); }
_Check_return_ inline float sin(_In_ float x) throw() { return sinf(x); }
_Check_return_ inline float sinh(_In_ float x) throw() { return sinhf(x); }
_Check_return_ inline float sqrt(_In_ float x) throw() { return sqrtf(x); }
_Check_return_ inline float tan(_In_ float x) throw() { return tanf(x); }
_Check_return_ inline float tanh(_In_ float x) throw() { return tanhf(x); }

_Check_return_ inline long double abs(_In_ long double x) throw() { return fabsl(x); }
_Check_return_ inline long double acos(_In_ long double x) throw() { return acosl(x); }
_Check_return_ inline long double asin(_In_ long double x) throw() { return asinl(x); }
_Check_return_ inline long double atan(_In_ long double x) throw() { return atanl(x); }
_Check_return_ inline long double atan2(_In_ long double y, _In_ long double x) throw() { return atan2l(y, x); }
_Check_return_ inline long double ceil(_In_ long double x) throw() { return ceill(x); }
_Check_return_ inline long double copysign(_In_ long double x, _In_ long double y) throw() { return _copysignl(x, y); }
_Check_return_ inline long double cos(_In_ long double x) throw() { return cosl(x); }
_Check_return_ inline long double cosh(_In_ long double x) throw() { return coshl(x); }
_Check_return_ inline long double exp(_In_ long double x) throw() { return expl(x); }
_Check_return_ inline long double fabs(_In_ long double x) throw() { return fabsl(x); }
_Check_return_ inline long double floor(_In_ long double x) throw() { return floorl(x); }
_Check_return_ inline long double fmod(_In_ long double x, _In_ long double y) throw() { return fmodl(x, y); }
_Check_return_ inline long double frexp(_In_ long double x, _Out_ int * y) throw() { return frexpl(x, y); }
_Check_return_ inline long double hypot(_In_ long double x, _In_ long double y) throw() { return hypotl(x, y); }
_Check_return_ inline long double ldexp(_In_ long double x, _In_ int y) throw() { return ldexpl(x, y); }
_Check_return_ inline long double log(_In_ long double x) throw() { return logl(x); }
_Check_return_ inline long double log10(_In_ long double x) throw() { return log10l(x); }
_Check_return_ inline long double modf(_In_ long double x, _Out_ long double * y) throw() { return modfl(x, y); }
_Check_return_ inline long double pow(_In_ long double x, _In_ long double y) throw() { return powl(x, y); }
_Check_return_ inline long double sin(_In_ long double x) throw() { return sinl(x); }
_Check_return_ inline long double sinh(_In_ long double x) throw() { return sinhl(x); }
_Check_return_ inline long double sqrt(_In_ long double x) throw() { return sqrtl(x); }
_Check_return_ inline long double tan(_In_ long double x) throw() { return tanl(x); }
_Check_return_ inline long double tanh(_In_ long double x) throw() { return tanhl(x); }
}
#endif /* !_CMATH_ */
#endif /* __cplusplus */

#pragma pack(pop)

#endif /* !_INC_MATH */

#if defined(_USE_MATH_DEFINES) && !defined(_MATH_DEFINES_DEFINED)
#define _MATH_DEFINES_DEFINED

#define M_E 2.71828182845904523536
#define M_LOG2E 1.44269504088896340736
#define M_LOG10E 0.434294481903251827651
#define M_LN2 0.693147180559945309417
#define M_LN10 2.30258509299404568402
#define M_PI 3.14159265358979323846
#define M_PI_2 1.57079632679489661923
#define M_PI_4 0.785398163397448309616
#define M_1_PI 0.318309886183790671538
#define M_2_PI 0.636619772367581343076
#define M_2_SQRTPI 1.12837916709551257390
#define M_SQRT2 1.41421356237309504880
#define M_SQRT1_2 0.707106781186547524401

#endif /* _USE_MATH_DEFINES */
