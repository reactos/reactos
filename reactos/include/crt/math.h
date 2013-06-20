/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_MATH
#define _INC_MATH

#include "crtdefs.h"

struct _exception;

#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__)
/* Some 3rd party code needs the declaration of C99 functions */
#include "mingw_math.h"
#endif

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
#endif

#define _DOMAIN 1
#define _SING 2
#define _OVERFLOW 3
#define _UNDERFLOW 4
#define _TLOSS 5
#define _PLOSS 6
#define EDOM 33
#define ERANGE 34

  _CRTIMP extern double _HUGE;

#define HUGE_VAL _HUGE
#define _matherrl _matherr

#ifndef _CRT_ABS_DEFINED
#define _CRT_ABS_DEFINED
  int __cdecl abs(_In_ int x);
  long __cdecl labs(_In_ long x);
#endif

  double __cdecl acos(_In_ double x);
  double __cdecl asin(_In_ double x);
  double __cdecl atan(_In_ double x);
  double __cdecl atan2(_In_ double y, _In_ double x);
  double __cdecl cos(_In_ double x);
  double __cdecl cosh(_In_ double x);
  double __cdecl exp(_In_ double x);
  double __cdecl fabs(_In_ double x);
  double __cdecl fmod(_In_ double x, _In_ double y);
  double __cdecl log(_In_ double x);
  double __cdecl log10(_In_ double x);
  double __cdecl pow(_In_ double x, double y);
  double __cdecl sin(_In_ double x);
  double __cdecl sinh(_In_ double x);
  double __cdecl sqrt(_In_ double x);
  double __cdecl tan(_In_ double x);
  double __cdecl tanh(_In_ double x);

#ifndef _CRT_MATHERR_DEFINED
#define _CRT_MATHERR_DEFINED
  int __cdecl _matherr(_Inout_ struct _exception *except);
#endif

#ifndef _CRT_ATOF_DEFINED
#define _CRT_ATOF_DEFINED

  _Check_return_
  _CRTIMP
  double
  __cdecl
  atof(
    _In_z_ const char *str);

  _Check_return_
  _CRTIMP
  double
  __cdecl
  _atof_l(
    _In_z_ const char *str,
    _In_opt_ _locale_t locale);

#endif /* _CRT_ATOF_DEFINED */

#ifndef _SIGN_DEFINED
#define _SIGN_DEFINED
  _Check_return_ _CRTIMP double __cdecl _copysign(_In_ double x, _In_ double sgn);
  _Check_return_ _CRTIMP double __cdecl _chgsign(_In_ double x);
#endif

  _CRTIMP double __cdecl _cabs(_In_ struct _complex a);
  _CRTIMP double __cdecl ceil(_In_ double x);
  _CRTIMP double __cdecl floor(_In_ double x);
  _CRTIMP double __cdecl frexp(_In_ double x, _Out_ int *y);
  _CRTIMP double __cdecl _hypot(_In_ double x, _In_ double y);
  _CRTIMP double __cdecl _j0(_In_ double x);
  _CRTIMP double __cdecl _j1(_In_ double x);
  _CRTIMP double __cdecl _jn(_In_ int x, _In_ double y);
  _CRTIMP double __cdecl ldexp(_In_ double x, _In_ int y);
  _CRTIMP double __cdecl modf(_In_ double x, _Out_ double *y);
  _CRTIMP double __cdecl _y0(_In_ double x);
  _CRTIMP double __cdecl _y1(_In_ double x);
  _CRTIMP double __cdecl _yn(_In_ int x, _In_ double y);
  _CRTIMP float __cdecl _hypotf(_In_ float x, _In_ float y);

#if defined(__i386__) || defined(_M_IX86)
  _CRTIMP int __cdecl _set_SSE2_enable(_In_ int flag);
#endif

#if defined(__x86_64) || defined(_M_AMD64)
  _CRTIMP float __cdecl _copysignf(_In_ float x, _In_ float sgn);
  _CRTIMP float __cdecl _chgsignf(_In_ float x);
  _CRTIMP float __cdecl _logbf(_In_ float x);
  _CRTIMP float __cdecl _nextafterf(_In_ float x, _In_ float y);
  _CRTIMP int __cdecl _finitef(_In_ float x);
  _CRTIMP int __cdecl _isnanf(_In_ float x);
  _CRTIMP int __cdecl _fpclassf(_In_ float x);
#endif

#if defined(__ia64__) || defined (_M_IA64)
  _CRTIMP float __cdecl fabsf(_In_ float x);
  _CRTIMP float __cdecl ldexpf(_In_ float x, _In_ int y);
  _CRTIMP long double __cdecl tanl(_In_ long double x);
#else
  __CRT_INLINE float __cdecl fabsf(_In_ float x) { return ((float)fabs((double)x)); }
  __CRT_INLINE float __cdecl ldexpf(_In_ float x, _In_ int expn) { return (float)ldexp(x, expn); }
  __CRT_INLINE long double tanl(_In_ long double x) { return (tan((double)x)); }
#endif

#if (_WIN32_WINNT >= 0x600) &&                 \
    (defined(__x86_64) || defined(_M_AMD64) || \
     defined (__ia64__) || defined (_M_IA64))

  _CRTIMP float __cdecl acosf(_In_ float x);
  _CRTIMP float __cdecl asinf(_In_ float x);
  _CRTIMP float __cdecl atanf(_In_ float x);
  _CRTIMP float __cdecl atan2f(_In_ float x, _In_ float y);
  _CRTIMP float __cdecl ceilf(_In_ float x);
  _CRTIMP float __cdecl cosf(_In_ float x);
  _CRTIMP float __cdecl coshf(_In_ float x);
  _CRTIMP float __cdecl expf(_In_ float x);
  _CRTIMP float __cdecl floorf(_In_ float x);
  _CRTIMP float __cdecl fmodf(_In_ float x, _In_ float y);
  _CRTIMP float __cdecl logf(_In_ float x);
  _CRTIMP float __cdecl log10f(_In_ float x);
  _CRTIMP float __cdecl modff(_In_ float x, _Out_ float *y);
  _CRTIMP float __cdecl powf(_In_ float b, _In_ float e);
  _CRTIMP float __cdecl sinf(_In_ float x);
  _CRTIMP float __cdecl sinhf(_In_ float x);
  _CRTIMP float __cdecl sqrtf(_In_ float x);
  _CRTIMP float __cdecl tanf(_In_ float x);
  _CRTIMP float __cdecl tanhf(_In_ float x);

#else

  __CRT_INLINE float acosf(_In_ float x) { return ((float)acos((double)x)); }
  __CRT_INLINE float asinf(_In_ float x) { return ((float)asin((double)x)); }
  __CRT_INLINE float atanf(_In_ float x) { return ((float)atan((double)x)); }
  __CRT_INLINE float atan2f(_In_ float x, _In_ float y) { return ((float)atan2((double)x,(double)y)); }
  __CRT_INLINE float ceilf(_In_ float x) { return ((float)ceil((double)x)); }
  __CRT_INLINE float cosf(_In_ float x) { return ((float)cos((double)x)); }
  __CRT_INLINE float coshf(_In_ float x) { return ((float)cosh((double)x)); }
  __CRT_INLINE float expf(_In_ float x) { return ((float)exp((double)x)); }
  __CRT_INLINE float floorf(_In_ float x) { return ((float)floor((double)x)); }
  __CRT_INLINE float fmodf(_In_ float x, _In_ float y) { return ((float)fmod((double)x,(double)y)); }
  __CRT_INLINE float logf(_In_ float x) { return ((float)log((double)x)); }
  __CRT_INLINE float log10f(_In_ float x) { return ((float)log10((double)x)); }
  __CRT_INLINE float modff(_In_ float x, _Out_ float *y) {
    double _Di,_Df = modf((double)x,&_Di);
    *y = (float)_Di;
    return ((float)_Df);
  }
  __CRT_INLINE float powf(_In_ float x, _In_ float y) { return ((float)pow((double)x,(double)y)); }
  __CRT_INLINE float sinf(_In_ float x) { return ((float)sin((double)x)); }
  __CRT_INLINE float sinhf(_In_ float x) { return ((float)sinh((double)x)); }
  __CRT_INLINE float sqrtf(_In_ float x) { return ((float)sqrt((double)x)); }
  __CRT_INLINE float tanf(_In_ float x) { return ((float)tan((double)x)); }
  __CRT_INLINE float tanhf(_In_ float x) { return ((float)tanh((double)x)); }

#endif

  __CRT_INLINE long double acosl(_In_ long double x) { return (acos((double)x)); }
  __CRT_INLINE long double asinl(_In_ long double x) { return (asin((double)x)); }
  __CRT_INLINE long double atanl(_In_ long double x) { return (atan((double)x)); }
  __CRT_INLINE long double atan2l(_In_ long double y, _In_ long double x) { return (atan2((double)y, (double)x)); }
  __CRT_INLINE long double ceill(_In_ long double x) { return (ceil((double)x)); }
  __CRT_INLINE long double cosl(_In_ long double x) { return (cos((double)x)); }
  __CRT_INLINE long double coshl(_In_ long double x) { return (cosh((double)x)); }
  __CRT_INLINE long double expl(_In_ long double x) { return (exp((double)x)); }
  __CRT_INLINE long double floorl(_In_ long double x) { return (floor((double)x)); }
  __CRT_INLINE long double fmodl(_In_ long double x, _In_ long double y) { return (fmod((double)x, (double)y)); }
  __CRT_INLINE long double frexpl(_In_ long double x, _Out_ int *y) { return (frexp((double)x, y)); }
  __CRT_INLINE long double logl(_In_ long double x) { return (log((double)x)); }
  __CRT_INLINE long double log10l(_In_ long double x) { return (log10((double)x)); }
  __CRT_INLINE long double powl(_In_ long double x, _In_ long double y) { return (pow((double)x, (double)y)); }
  __CRT_INLINE long double sinl(_In_ long double x) { return (sin((double)x)); }
  __CRT_INLINE long double sinhl(_In_ long double x) { return (sinh((double)x)); }
  __CRT_INLINE long double sqrtl(_In_ long double x) { return (sqrt((double)x)); }
  __CRT_INLINE long double tanhl(_In_ long double x) {return (tanh((double)x)); }
  __CRT_INLINE long double __cdecl fabsl(_In_ long double x) { return fabs((double)x); }
  __CRT_INLINE long double _chgsignl(_In_ long double _Number) { return _chgsign((double)(_Number)); }
  __CRT_INLINE long double _copysignl(_In_ long double _Number, _In_ long double _Sign) { return _copysign((double)(_Number),(double)(_Sign)); }
  __CRT_INLINE long double _hypotl(_In_ long double x, _In_ long double y) { return _hypot((double)(x),(double)(y)); }
  __CRT_INLINE float frexpf(_In_ float x, _Out_ int *y) { return ((float)frexp((double)x,y)); }
  __CRT_INLINE long double ldexpl(_In_ long double x, _In_ int y) { return ldexp((double)x, y); }
  __CRT_INLINE long double modfl(_In_ long double x, _Out_ long double *y) {
    double _Di,_Df = modf((double)x,&_Di);
    *y = (long double)_Di;
    return (_Df);
  }

#ifndef NO_OLDNAMES

#define DOMAIN _DOMAIN
#define SING _SING
#define OVERFLOW _OVERFLOW
#define UNDERFLOW _UNDERFLOW
#define TLOSS _TLOSS
#define PLOSS _PLOSS
#define matherr _matherr
#define HUGE _HUGE
  // _CRTIMP double __cdecl cabs(struct _complex x);
  #define cabs _cabs

  _CRTIMP double __cdecl hypot(_In_ double x, _In_ double y);
  _CRTIMP double __cdecl j0(_In_ double x);
  _CRTIMP double __cdecl j1(_In_ double x);
  _CRTIMP double __cdecl jn(_In_ int x, _In_ double y);
  _CRTIMP double __cdecl y0(_In_ double x);
  _CRTIMP double __cdecl y1(_In_ double x);
  _CRTIMP double __cdecl yn(_In_ int x, _In_ double y);
  __CRT_INLINE float __cdecl hypotf(_In_ float x, _In_ float y) { return (float) hypot(x, y); }

#endif /* NO_OLDNAMES */

#ifdef __cplusplus
}
extern "C++" {
  template<class _Ty> inline _Ty _Pow_int(_Ty x,int y) {
    unsigned int _N;
    if(y >= 0) _N = (unsigned int)y;
    else _N = (unsigned int)(-y);
    for(_Ty _Z = _Ty(1);;x *= x) {
      if((_N & 1)!=0) _Z *= x;
      if((_N >>= 1)==0) return (y < 0 ? _Ty(1) / _Z : _Z);
    }
  }
}
#endif

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
