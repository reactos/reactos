/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_MATH
#define _INC_MATH

#include "crtdefs.h"

struct exception;

#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
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
  int __cdecl abs(int x);
  long __cdecl labs(long x);
#endif
  double __cdecl acos(double x);
  double __cdecl asin(double x);
  double __cdecl atan(double x);
  double __cdecl atan2(double y, double x);
  double __cdecl cos(double x);
  double __cdecl cosh(double x);
  double __cdecl exp(double x);
  double __cdecl fabs(double x);
  double __cdecl fmod(double x, double y);
  double __cdecl log(double x);
  double __cdecl log10(double x);
  double __cdecl pow(double x, double y);
  double __cdecl sin(double x);
  double __cdecl sinh(double x);
  double __cdecl sqrt(double x);
  double __cdecl tan(double x);
  double __cdecl tanh(double x);
#ifndef _CRT_MATHERR_DEFINED
#define _CRT_MATHERR_DEFINED
  int __cdecl _matherr(struct _exception *except);
#endif

#ifndef _CRT_ATOF_DEFINED
#define _CRT_ATOF_DEFINED
  _CRTIMP double __cdecl atof(const char *str);
  _CRTIMP double __cdecl _atof_l(const char *str ,_locale_t locale);
#endif
#ifndef _SIGN_DEFINED
#define _SIGN_DEFINED
  _CRTIMP double __cdecl _copysign(double x,double sgn);
  _CRTIMP double __cdecl _chgsign(double x);
#endif
  _CRTIMP double __cdecl _cabs(struct _complex a);
  _CRTIMP double __cdecl ceil(double x);
  _CRTIMP double __cdecl floor(double x);
  _CRTIMP double __cdecl frexp(double x, int *y);
  _CRTIMP double __cdecl _hypot(double x, double y);
  _CRTIMP double __cdecl _j0(double x);
  _CRTIMP double __cdecl _j1(double x);
  _CRTIMP double __cdecl _jn(int x, double y);
  _CRTIMP double __cdecl ldexp(double x, int y);
  _CRTIMP double __cdecl modf(double x, double *y);
  _CRTIMP double __cdecl _y0(double x);
  _CRTIMP double __cdecl _y1(double x);
  _CRTIMP double __cdecl _yn(int x, double y);
  _CRTIMP float __cdecl _hypotf(float x, float y);

#if defined(__i386__) || defined(_M_IX86)
  _CRTIMP int __cdecl _set_SSE2_enable(int flag);
#endif

#if defined(__x86_64) || defined(_M_AMD64)
  _CRTIMP float __cdecl _copysignf(float x, float sgn);
  _CRTIMP float __cdecl _chgsignf(float x);
  _CRTIMP float __cdecl _logbf(float x);
  _CRTIMP float __cdecl _nextafterf(float x,float y);
  _CRTIMP int __cdecl _finitef(float x);
  _CRTIMP int __cdecl _isnanf(float x);
  _CRTIMP int __cdecl _fpclassf(float x);
#endif

#if defined(__ia64__) || defined (_M_IA64)
  _CRTIMP float __cdecl fabsf(float x);
  _CRTIMP float __cdecl ldexpf(float x, int y);
  _CRTIMP long double __cdecl tanl(long double x);
#else
  __CRT_INLINE float __cdecl fabsf(float x) { return ((float)fabs((double)x)); }
  __CRT_INLINE float __cdecl ldexpf(float x, int expn) { return (float)ldexp (x, expn); }
  __CRT_INLINE long double tanl(long double x) { return (tan((double)x)); }
#endif

#if (_WIN32_WINNT >= 0x600) && \
    (defined(__x86_64) || defined(_M_AMD64) || \
     defined (__ia64__) || defined (_M_IA64))
  _CRTIMP float __cdecl acosf(float x);
  _CRTIMP float __cdecl asinf(float x);
  _CRTIMP float __cdecl atanf(float x);
  _CRTIMP float __cdecl atan2f(float x, float y);
  _CRTIMP float __cdecl ceilf(float x);
  _CRTIMP float __cdecl cosf(float x);
  _CRTIMP float __cdecl coshf(float x);
  _CRTIMP float __cdecl expf(float x);
  _CRTIMP float __cdecl floorf(float x);
  _CRTIMP float __cdecl fmodf(float x, float y);
  _CRTIMP float __cdecl logf(float x);
  _CRTIMP float __cdecl log10f(float x);
  _CRTIMP float __cdecl modff(float x, float *y);
  _CRTIMP float __cdecl powf(float b, float e);
  _CRTIMP float __cdecl sinf(float x);
  _CRTIMP float __cdecl sinhf(float x);
  _CRTIMP float __cdecl sqrtf(float x);
  _CRTIMP float __cdecl tanf(float x);
  _CRTIMP float __cdecl tanhf(float x);
#else
  __CRT_INLINE float acosf(float x) { return ((float)acos((double)x)); }
  __CRT_INLINE float asinf(float x) { return ((float)asin((double)x)); }
  __CRT_INLINE float atanf(float x) { return ((float)atan((double)x)); }
  __CRT_INLINE float atan2f(float x,float y) { return ((float)atan2((double)x,(double)y)); }
  __CRT_INLINE float ceilf(float x) { return ((float)ceil((double)x)); }
  __CRT_INLINE float cosf(float x) { return ((float)cos((double)x)); }
  __CRT_INLINE float coshf(float x) { return ((float)cosh((double)x)); }
  __CRT_INLINE float expf(float x) { return ((float)exp((double)x)); }
  __CRT_INLINE float floorf(float x) { return ((float)floor((double)x)); }
  __CRT_INLINE float fmodf(float x,float y) { return ((float)fmod((double)x,(double)y)); }
  __CRT_INLINE float logf(float x) { return ((float)log((double)x)); }
  __CRT_INLINE float log10f(float x) { return ((float)log10((double)x)); }
  __CRT_INLINE float modff(float x,float *y) {
    double _Di,_Df = modf((double)x,&_Di);
    *y = (float)_Di;
    return ((float)_Df);
  }
  __CRT_INLINE float powf(float x,float y) { return ((float)pow((double)x,(double)y)); }
  __CRT_INLINE float sinf(float x) { return ((float)sin((double)x)); }
  __CRT_INLINE float sinhf(float x) { return ((float)sinh((double)x)); }
  __CRT_INLINE float sqrtf(float x) { return ((float)sqrt((double)x)); }
  __CRT_INLINE float tanf(float x) { return ((float)tan((double)x)); }
  __CRT_INLINE float tanhf(float x) { return ((float)tanh((double)x)); }
#endif

  __CRT_INLINE long double acosl(long double x) { return (acos((double)x)); }
  __CRT_INLINE long double asinl(long double x) { return (asin((double)x)); }
  __CRT_INLINE long double atanl(long double x) { return (atan((double)x)); }
  __CRT_INLINE long double atan2l(long double y, long double x) { return (atan2((double)y, (double)x)); }
  __CRT_INLINE long double ceill(long double x) { return (ceil((double)x)); }
  __CRT_INLINE long double cosl(long double x) { return (cos((double)x)); }
  __CRT_INLINE long double coshl(long double x) { return (cosh((double)x)); }
  __CRT_INLINE long double expl(long double x) { return (exp((double)x)); }
  __CRT_INLINE long double floorl(long double x) { return (floor((double)x)); }
  __CRT_INLINE long double fmodl(long double x, long double y) { return (fmod((double)x, (double)y)); }
  __CRT_INLINE long double frexpl(long double x, int *y) { return (frexp((double)x, y)); }
  __CRT_INLINE long double logl(long double x) { return (log((double)x)); }
  __CRT_INLINE long double log10l(long double x) { return (log10((double)x)); }
  __CRT_INLINE long double powl(long double x, long double y) { return (pow((double)x, (double)y)); }
  __CRT_INLINE long double sinl(long double x) { return (sin((double)x)); }
  __CRT_INLINE long double sinhl(long double x) { return (sinh((double)x)); }
  __CRT_INLINE long double sqrtl(long double x) { return (sqrt((double)x)); }
  __CRT_INLINE long double tanhl(long double x) {return (tanh((double)x)); }
  __CRT_INLINE long double __cdecl fabsl(long double x) { return fabs((double)x); }
  __CRT_INLINE long double _chgsignl(long double _Number) { return _chgsign((double)(_Number)); }
  __CRT_INLINE long double _copysignl(long double _Number, long double _Sign) { return _copysign((double)(_Number),(double)(_Sign)); }
  __CRT_INLINE long double _hypotl(long double x,long double y) { return _hypot((double)(x),(double)(y)); }
  __CRT_INLINE float frexpf(float x, int *y) { return ((float)frexp((double)x,y)); }
  __CRT_INLINE long double ldexpl(long double x, int y) { return ldexp((double)x, y); }
  __CRT_INLINE long double modfl(long double x,long double *y) {
    double _Di,_Df = modf((double)x,&_Di);
    *y = (long double)_Di;
    return (_Df);
  }

#ifndef	NO_OLDNAMES
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
  _CRTIMP double __cdecl hypot(double x,double y);
  _CRTIMP double __cdecl j0(double x);
  _CRTIMP double __cdecl j1(double x);
  _CRTIMP double __cdecl jn(int x,double y);
  _CRTIMP double __cdecl y0(double x);
  _CRTIMP double __cdecl y1(double x);
  _CRTIMP double __cdecl yn(int x,double y);
  __CRT_INLINE float __cdecl hypotf(float x, float y) { return (float) hypot (x, y); }
#endif

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

#ifndef __NO_ISOCEXT
#if (defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
  || !defined __STRICT_ANSI__ || defined __GLIBCPP__
  
  /* Inverse hyperbolic trig functions  */ 
  /* 7.12.5.1 */
  extern double __cdecl acosh (double);
  extern float __cdecl acoshf (float);
  extern long double __cdecl acoshl (long double);

  /* 7.12.5.2 */
  extern double __cdecl asinh (double);
  extern float __cdecl asinhf (float);
  extern long double __cdecl asinhl (long double);

  /* 7.12.5.3 */
  extern double __cdecl atanh (double);
  extern float __cdecl atanhf  (float);
  extern long double __cdecl atanhl (long double);
  
  /* 7.12.6.2 */
  extern double __cdecl exp2(double);
  extern float __cdecl exp2f(float);
  extern long double __cdecl exp2l(long double);
  
  /* 7.12.6.10 */
  extern double __cdecl log2 (double);
  extern float __cdecl log2f (float);
  extern long double __cdecl log2l (long double);
  
  /* 7.12.9.8 */
  /* round towards zero, regardless of fpu control word settings */
  extern double __cdecl trunc (double);
  extern float __cdecl truncf (float);
  extern long double __cdecl truncl (long double);

#endif /* __STDC_VERSION__ >= 199901L */
#endif /* __NO_ISOCEXT */
