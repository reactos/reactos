/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _MATH_H_
#define _MATH_H_

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#include <_mingw.h>

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

#ifndef _HUGE
#ifdef _MSVCRT_
  extern double *_HUGE;
#else
  extern double *_imp___HUGE;
#define _HUGE	(*_imp___HUGE)
#endif
#endif

#define HUGE_VAL _HUGE

#ifndef _CRT_ABS_DEFINED
#define _CRT_ABS_DEFINED
  int __cdecl abs(int _X);
  long __cdecl labs(long _X);
#endif
  double __cdecl acos(double _X);
  double __cdecl asin(double _X);
  double __cdecl atan(double _X);
  double __cdecl atan2(double _Y,double _X);
#ifndef _SIGN_DEFINED
#define _SIGN_DEFINED
  _CRTIMP double __cdecl _copysign (double _Number,double _Sign);
  _CRTIMP double __cdecl _chgsign (double _X);
#endif
  double __cdecl cos(double _X);
  double __cdecl cosh(double _X);
  double __cdecl exp(double _X);
  double expm1(double _X);
  double __cdecl fabs(double _X);
  double __cdecl fmod(double _X,double _Y);
  double __cdecl log(double _X);
  double __cdecl log10(double _X);
  double __cdecl pow(double _X,double _Y);
  double __cdecl sin(double _X);
  double __cdecl sinh(double _X);
  double __cdecl tan(double _X);
  double __cdecl tanh(double _X);
  double __cdecl sqrt(double _X);
#ifndef _CRT_ATOF_DEFINED
#define _CRT_ATOF_DEFINED
  double __cdecl atof(const char *_String);
  double __cdecl _atof_l(const char *_String,_locale_t _Locale);
#endif

  _CRTIMP double __cdecl _cabs(struct _complex _ComplexA);
  double __cdecl ceil(double _X);
  double __cdecl floor(double _X);
  double __cdecl frexp(double _X,int *_Y);
  double __cdecl _hypot(double _X,double _Y);
  _CRTIMP double __cdecl _j0(double _X);
  _CRTIMP double __cdecl _j1(double _X);
  _CRTIMP double __cdecl _jn(int _X,double _Y);
  double __cdecl ldexp(double _X,int _Y);
#ifndef _CRT_MATHERR_DEFINED
#define _CRT_MATHERR_DEFINED
  int __cdecl _matherr(struct _exception *_Except);
#endif
  double __cdecl modf(double _X,double *_Y);
  _CRTIMP double __cdecl _y0(double _X);
  _CRTIMP double __cdecl _y1(double _X);
  _CRTIMP double __cdecl _yn(int _X,double _Y);

#if(defined(_X86_) && !defined(__x86_64))
  _CRTIMP int __cdecl _set_SSE2_enable(int _Flag);
  /* from libmingwex */
  float __cdecl _hypotf(float _X,float _Y);
#endif

  float frexpf(float _X,int *_Y);
  float __cdecl ldexpf(float _X,int _Y);
  long double __cdecl ldexpl(long double _X,int _Y);
  float __cdecl acosf(float _X);
  float __cdecl asinf(float _X);
   float __cdecl atanf(float _X);
   float __cdecl atan2f(float _X,float _Y);
   float __cdecl cosf(float _X);
   float __cdecl sinf(float _X);
   float __cdecl tanf(float _X);
   float __cdecl coshf(float _X);
   float __cdecl sinhf(float _X);
   float __cdecl tanhf(float _X);
   float __cdecl expf(float _X);
   float expm1f(float _X);
   float __cdecl logf(float _X);
   float __cdecl log10f(float _X);
   float __cdecl modff(float _X,float *_Y);
   float __cdecl powf(float _X,float _Y);
   float __cdecl sqrtf(float _X);
   float __cdecl ceilf(float _X);
   float __cdecl floorf(float _X);
  float __cdecl fmodf(float _X,float _Y);
   float __cdecl _hypotf(float _X,float _Y);
  float __cdecl fabsf(float _X);
#if !defined(__ia64__)
   /* from libmingwex */
   float __cdecl _copysignf (float _Number,float _Sign);
   float __cdecl _chgsignf (float _X);
   float __cdecl _logbf(float _X);
   float __cdecl _nextafterf(float _X,float _Y);
   int __cdecl _finitef(float _X);
   int __cdecl _isnanf(float _X);
   int __cdecl _fpclassf(float _X);
#endif

#ifndef __cplusplus
  __CRT_INLINE long double __cdecl fabsl (long double x)
  {
    long double res;
    __asm__ ("fabs;" : "=t" (res) : "0" (x));
    return res;
  }
#define _hypotl(x,y) ((long double)_hypot((double)(x),(double)(y)))
#define _matherrl _matherr
  __CRT_INLINE long double _chgsignl(long double _Number) { return _chgsign((double)(_Number)); }
  __CRT_INLINE long double _copysignl(long double _Number,long double _Sign) { return _copysign((double)(_Number),(double)(_Sign)); }
  __CRT_INLINE float frexpf(float _X,int *_Y) { return ((float)frexp((double)_X,_Y)); }

#if !defined (__ia64__)
  __CRT_INLINE float __cdecl fabsf (float x)
  {
    float res;
    __asm__ ("fabs;" : "=t" (res) : "0" (x));
    return res;
  }

  __CRT_INLINE float __cdecl ldexpf (float x, int expn) { return (float) ldexp (x, expn); }
#endif
#else
  // cplusplus
  __CRT_INLINE long double __cdecl fabsl (long double x)
  {
    long double res;
    __asm__ ("fabs;" : "=t" (res) : "0" (x));
    return res;
  }
  __CRT_INLINE long double modfl(long double _X,long double *_Y) {
    double _Di,_Df = modf((double)_X,&_Di);
    *_Y = (long double)_Di;
    return (_Df);
  }
  __CRT_INLINE long double _chgsignl(long double _Number) { return _chgsign(static_cast<double>(_Number)); }
  __CRT_INLINE long double _copysignl(long double _Number,long double _Sign) { return _copysign(static_cast<double>(_Number),static_cast<double>(_Sign)); }
  __CRT_INLINE float frexpf(float _X,int *_Y) { return ((float)frexp((double)_X,_Y)); }
#ifndef __ia64__
  __CRT_INLINE float __cdecl fabsf (float x)
  {
    float res;
    __asm__ ("fabs;" : "=t" (res) : "0" (x));
    return res;
  }
  __CRT_INLINE float __cdecl ldexpf (float x, int expn) { return (float) ldexp (x, expn); }
#ifndef __x86_64
  __CRT_INLINE float acosf(float _X) { return ((float)acos((double)_X)); }
  __CRT_INLINE float asinf(float _X) { return ((float)asin((double)_X)); }
  __CRT_INLINE float atanf(float _X) { return ((float)atan((double)_X)); }
  __CRT_INLINE float atan2f(float _X,float _Y) { return ((float)atan2((double)_X,(double)_Y)); }
  __CRT_INLINE float ceilf(float _X) { return ((float)ceil((double)_X)); }
  __CRT_INLINE float cosf(float _X) { return ((float)cos((double)_X)); }
  __CRT_INLINE float coshf(float _X) { return ((float)cosh((double)_X)); }
  __CRT_INLINE float expf(float _X) { return ((float)exp((double)_X)); }
  __CRT_INLINE float floorf(float _X) { return ((float)floor((double)_X)); }
  __CRT_INLINE float fmodf(float _X,float _Y) { return ((float)fmod((double)_X,(double)_Y)); }
  __CRT_INLINE float logf(float _X) { return ((float)log((double)_X)); }
  __CRT_INLINE float log10f(float _X) { return ((float)log10((double)_X)); }
  __CRT_INLINE float modff(float _X,float *_Y) {
    double _Di,_Df = modf((double)_X,&_Di);
    *_Y = (float)_Di;
    return ((float)_Df);
  }
  __CRT_INLINE float powf(float _X,float _Y) { return ((float)pow((double)_X,(double)_Y)); }
  __CRT_INLINE float sinf(float _X) { return ((float)sin((double)_X)); }
  __CRT_INLINE float sinhf(float _X) { return ((float)sinh((double)_X)); }
  __CRT_INLINE float sqrtf(float _X) { return ((float)sqrt((double)_X)); }
  __CRT_INLINE float tanf(float _X) { return ((float)tan((double)_X)); }
  __CRT_INLINE float tanhf(float _X) { return ((float)tanh((double)_X)); }
#endif
#endif
#endif

#ifndef	NO_OLDNAMES
#define DOMAIN _DOMAIN
#define SING _SING
#define OVERFLOW _OVERFLOW
#define UNDERFLOW _UNDERFLOW
#define TLOSS _TLOSS
#define PLOSS _PLOSS
#define matherr _matherr

#define HUGE	_HUGE
  /*	double __cdecl cabs(struct _complex _X); */
  double __cdecl hypot(double _X,double _Y);
  _CRTIMP double __cdecl j0(double _X);
  _CRTIMP double __cdecl j1(double _X);
  _CRTIMP double __cdecl jn(int _X,double _Y);
  _CRTIMP double __cdecl y0(double _X);
  _CRTIMP double __cdecl y1(double _X);
  _CRTIMP double __cdecl yn(int _X,double _Y);
#endif

#ifndef __NO_ISOCEXT
#if (defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
  || !defined __STRICT_ANSI__ || defined __GLIBCPP__

#define NAN (0.0F/0.0F)
#define HUGE_VALF (1.0F/0.0F)
#define HUGE_VALL (1.0L/0.0L)
#define INFINITY (1.0F/0.0F)


#define FP_NAN		0x0100
#define FP_NORMAL	0x0400
#define FP_INFINITE	(FP_NAN | FP_NORMAL)
#define FP_ZERO		0x4000
#define FP_SUBNORMAL	(FP_NORMAL | FP_ZERO)
  /* 0x0200 is signbit mask */


  /*
  We can't __CRT_INLINE float or double, because we want to ensure truncation
  to semantic type before classification. 
  (A normal long double value might become subnormal when 
  converted to double, and zero when converted to float.)
  */

  extern int __cdecl __fpclassifyf (float);
  extern int __cdecl __fpclassify (double);

  __CRT_INLINE int __cdecl __fpclassifyl (long double x){
    unsigned short sw;
    __asm__ ("fxam; fstsw %%ax;" : "=a" (sw): "t" (x));
    return sw & (FP_NAN | FP_NORMAL | FP_ZERO );
  }

#define fpclassify(x) (sizeof (x) == sizeof (float) ? __fpclassifyf (x)	  \
  : sizeof (x) == sizeof (double) ? __fpclassify (x) \
  : __fpclassifyl (x))

  /* 7.12.3.2 */
#define isfinite(x) ((fpclassify(x) & FP_NAN) == 0)

  /* 7.12.3.3 */
#define isinf(x) (fpclassify(x) == FP_INFINITE)

  /* 7.12.3.4 */
  /* We don't need to worry about trucation here:
  A NaN stays a NaN. */

  __CRT_INLINE int __cdecl __isnan (double _x)
  {
    unsigned short sw;
    __asm__ ("fxam;"
      "fstsw %%ax": "=a" (sw) : "t" (_x));
    return (sw & (FP_NAN | FP_NORMAL | FP_INFINITE | FP_ZERO | FP_SUBNORMAL))
      == FP_NAN;
  }

  __CRT_INLINE int __cdecl __isnanf (float _x)
  {
    unsigned short sw;
    __asm__ ("fxam;"
      "fstsw %%ax": "=a" (sw) : "t" (_x));
    return (sw & (FP_NAN | FP_NORMAL | FP_INFINITE | FP_ZERO | FP_SUBNORMAL))
      == FP_NAN;
  }

  __CRT_INLINE int __cdecl __isnanl (long double _x)
  {
    unsigned short sw;
    __asm__ ("fxam;"
      "fstsw %%ax": "=a" (sw) : "t" (_x));
    return (sw & (FP_NAN | FP_NORMAL | FP_INFINITE | FP_ZERO | FP_SUBNORMAL))
      == FP_NAN;
  }


#define isnan(x) (sizeof (x) == sizeof (float) ? __isnanf (x)	\
  : sizeof (x) == sizeof (double) ? __isnan (x)	\
  : __isnanl (x))

  /* 7.12.3.5 */
#define isnormal(x) (fpclassify(x) == FP_NORMAL)

  /* 7.12.3.6 The signbit macro */
  __CRT_INLINE int __cdecl __signbit (double x) {
    unsigned short stw;
    __asm__ ( "fxam; fstsw %%ax;": "=a" (stw) : "t" (x));
    return stw & 0x0200;
  }

  __CRT_INLINE int __cdecl __signbitf (float x) {
    unsigned short stw;
    __asm__ ("fxam; fstsw %%ax;": "=a" (stw) : "t" (x));
    return stw & 0x0200;
  }

  __CRT_INLINE int __cdecl __signbitl (long double x) {
    unsigned short stw;
    __asm__ ("fxam; fstsw %%ax;": "=a" (stw) : "t" (x));
    return stw & 0x0200;
  }

#define signbit(x) (sizeof (x) == sizeof (float) ? __signbitf (x)	\
  : sizeof (x) == sizeof (double) ? __signbit (x)	\
  : __signbitl (x))

  extern double __cdecl exp2(double);
  extern float __cdecl exp2f(float);
  extern long double __cdecl exp2l(long double);

#define FP_ILOGB0 ((int)0x80000000)
#define FP_ILOGBNAN ((int)0x80000000)
  extern int __cdecl ilogb (double);
  extern int __cdecl ilogbf (float);
  extern int __cdecl ilogbl (long double);

  extern double __cdecl log1p(double);
  extern float __cdecl log1pf(float);
  extern long double __cdecl log1pl(long double);

  extern double __cdecl log2 (double);
  extern float __cdecl log2f (float);
  extern long double __cdecl log2l (long double);

  extern double __cdecl logb (double);
  extern float __cdecl logbf (float);
  extern long double __cdecl logbl (long double);

  __CRT_INLINE double __cdecl logb (double x)
  {
    double res;
    __asm__ ("fxtract\n\t"
      "fstp	%%st" : "=t" (res) : "0" (x));
    return res;
  }

  __CRT_INLINE float __cdecl logbf (float x)
  {
    float res;
    __asm__ ("fxtract\n\t"
      "fstp	%%st" : "=t" (res) : "0" (x));
    return res;
  }

  __CRT_INLINE long double __cdecl logbl (long double x)
  {
    long double res;
    __asm__ ("fxtract\n\t"
      "fstp	%%st" : "=t" (res) : "0" (x));
    return res;
  }

  extern long double __cdecl modfl (long double, long double*);

  /* 7.12.6.13 */
  extern double __cdecl scalbn (double, int);
  extern float __cdecl scalbnf (float, int);
  extern long double __cdecl scalbnl (long double, int);

  extern double __cdecl scalbln (double, long);
  extern float __cdecl scalblnf (float, long);
  extern long double __cdecl scalblnl (long double, long);

  /* 7.12.7.1 */
  /* Implementations adapted from Cephes versions */ 
  extern double __cdecl cbrt (double);
  extern float __cdecl cbrtf (float);
  extern long double __cdecl cbrtl (long double);

  __CRT_INLINE float __cdecl hypotf (float x, float y)
  { return (float) hypot (x, y);}
  extern long double __cdecl hypotl (long double, long double);

  extern long double __cdecl powl (long double, long double);
  extern long double __cdecl expl(long double);
  extern long double expm1l(long double);
  extern long double __cdecl coshl(long double);
  extern long double __cdecl fabsl (long double);
  extern long double __cdecl acosl(long double);
  extern long double __cdecl asinl(long double);
  extern long double __cdecl atanl(long double);
  extern long double __cdecl atan2l(long double,long double);
  extern long double __cdecl sinhl(long double);
  extern long double __cdecl tanhl(long double);

  /* 7.12.8.1 The erf functions  */
  extern double __cdecl erf (double);
  extern float __cdecl erff (float);
  /* TODO
  extern long double __cdecl erfl (long double);
  */ 

  /* 7.12.8.2 The erfc functions  */
  extern double __cdecl erfc (double);
  extern float __cdecl erfcf (float);
  /* TODO
  extern long double __cdecl erfcl (long double);
  */ 

  /* 7.12.8.3 The lgamma functions */
  extern double __cdecl lgamma (double);
  extern float __cdecl lgammaf (float);
  extern long double __cdecl lgammal (long double);

  /* 7.12.8.4 The tgamma functions */
  extern double __cdecl tgamma (double);
  extern float __cdecl tgammaf (float);
  extern long double __cdecl tgammal (long double);

  extern long double __cdecl ceill (long double);
  extern long double __cdecl floorl (long double);
  extern long double __cdecl frexpl(long double,int *);
  extern long double __cdecl log10l(long double);
  extern long double __cdecl logl(long double);
  extern long double __cdecl cosl(long double);
  extern long double __cdecl sinl(long double);
  extern long double __cdecl tanl(long double);
  extern long double sqrtl(long double);

  /* 7.12.9.3 */
  extern double __cdecl nearbyint ( double);
  extern float __cdecl nearbyintf (float);
  extern long double __cdecl nearbyintl (long double);

  /* 7.12.9.4 */
  /* round, using fpu control word settings */
  __CRT_INLINE double __cdecl rint (double x)
  {
    double retval;
    __asm__ ("frndint;": "=t" (retval) : "0" (x));
    return retval;
  }

  __CRT_INLINE float __cdecl rintf (float x)
  {
    float retval;
    __asm__ ("frndint;" : "=t" (retval) : "0" (x) );
    return retval;
  }

  __CRT_INLINE long double __cdecl rintl (long double x)
  {
    long double retval;
    __asm__ ("frndint;" : "=t" (retval) : "0" (x) );
    return retval;
  }

  /* 7.12.9.5 */
  __CRT_INLINE long __cdecl lrint (double x) 
  {
    long retval;  
    __asm__ __volatile__							      \
      ("fistpl %0"  : "=m" (retval) : "t" (x) : "st");				      \
      return retval;
  }

  __CRT_INLINE long __cdecl lrintf (float x) 
  {
    long retval;
    __asm__ __volatile__							      \
      ("fistpl %0"  : "=m" (retval) : "t" (x) : "st");				      \
      return retval;
  }

  __CRT_INLINE long __cdecl lrintl (long double x) 
  {
    long retval;
    __asm__ __volatile__							      \
      ("fistpl %0"  : "=m" (retval) : "t" (x) : "st");				      \
      return retval;
  }

  __CRT_INLINE long long __cdecl llrint (double x) 
  {
    long long retval;
    __asm__ __volatile__							      \
      ("fistpll %0"  : "=m" (retval) : "t" (x) : "st");				      \
      return retval;
  }

  __CRT_INLINE long long __cdecl llrintf (float x) 
  {
    long long retval;
    __asm__ __volatile__							      \
      ("fistpll %0"  : "=m" (retval) : "t" (x) : "st");				      \
      return retval;
  }

  __CRT_INLINE long long __cdecl llrintl (long double x) 
  {
    long long retval;
    __asm__ __volatile__							      \
      ("fistpll %0"  : "=m" (retval) : "t" (x) : "st");				      \
      return retval;
  }

  /* 7.12.9.6 */
  /* round away from zero, regardless of fpu control word settings */
  extern double __cdecl round (double);
  extern float __cdecl roundf (float);
  extern long double __cdecl roundl (long double);

  /* 7.12.9.7  */
  extern long __cdecl lround (double);
  extern long __cdecl lroundf (float);
  extern long __cdecl lroundl (long double);

  extern long long __cdecl llround (double);
  extern long long __cdecl llroundf (float);
  extern long long __cdecl llroundl (long double);

  /* 7.12.9.8 */
  /* round towards zero, regardless of fpu control word settings */
  extern double __cdecl trunc (double);
  extern float __cdecl truncf (float);
  extern long double __cdecl truncl (long double);

  extern long double __cdecl fmodl (long double, long double);

  /* 7.12.10.2 */ 
  extern double __cdecl remainder (double, double);
  extern float __cdecl remainderf (float, float);
  extern long double __cdecl remainderl (long double, long double);

  /* 7.12.10.3 */
  extern double __cdecl remquo(double, double, int *);
  extern float __cdecl remquof(float, float, int *);
  extern long double __cdecl remquol(long double, long double, int *);

  /* 7.12.11.1 */
  extern double __cdecl copysign (double, double); /* in libmoldname.a */
  extern float __cdecl copysignf (float, float);
  extern long double __cdecl copysignl (long double, long double);

  /* 7.12.11.2 Return a NaN */
  extern double __cdecl nan(const char *tagp);
  extern float __cdecl nanf(const char *tagp);
  extern long double __cdecl nanl(const char *tagp);

#ifndef __STRICT_ANSI__
#define _nan() nan("")
#define _nanf() nanf("")
#define _nanl() nanl("")
#endif

  /* 7.12.11.3 */
  extern double __cdecl nextafter (double, double); /* in libmoldname.a */
  extern float __cdecl nextafterf (float, float);
  extern long double __cdecl nextafterl (long double, long double);

  /* 7.12.11.4 The nexttoward functions: TODO */

  /* 7.12.12.1 */
  /*  x > y ? (x - y) : 0.0  */
  extern double __cdecl fdim (double x, double y);
  extern float __cdecl fdimf (float x, float y);
  extern long double __cdecl fdiml (long double x, long double y);

  /* fmax and fmin.
  NaN arguments are treated as missing data: if one argument is a NaN
  and the other numeric, then these functions choose the numeric
  value. */

  /* 7.12.12.2 */
  extern double __cdecl fmax  (double, double);
  extern float __cdecl fmaxf (float, float);
  extern long double __cdecl fmaxl (long double, long double);

  /* 7.12.12.3 */
  extern double __cdecl fmin (double, double);
  extern float __cdecl fminf (float, float);
  extern long double __cdecl fminl (long double, long double);

  /* 7.12.13.1 */
  /* return x * y + z as a ternary op */ 
  extern double __cdecl fma (double, double, double);
  extern float __cdecl fmaf (float, float, float);
  extern long double __cdecl fmal (long double, long double, long double);


  /* 7.12.14 */
  /* 
  *  With these functions, comparisons involving quiet NaNs set the FP
  *  condition code to "unordered".  The IEEE floating-point spec
  *  dictates that the result of floating-point comparisons should be
  *  false whenever a NaN is involved, with the exception of the != op, 
  *  which always returns true: yes, (NaN != NaN) is true).
  */

#if __GNUC__ >= 3

#define isgreater(x, y) __builtin_isgreater(x, y)
#define isgreaterequal(x, y) __builtin_isgreaterequal(x, y)
#define isless(x, y) __builtin_isless(x, y)
#define islessequal(x, y) __builtin_islessequal(x, y)
#define islessgreater(x, y) __builtin_islessgreater(x, y)
#define isunordered(x, y) __builtin_isunordered(x, y)

#else
  /*  helper  */
  __CRT_INLINE int  __cdecl
    __fp_unordered_compare (long double x, long double y){
      unsigned short retval;
      __asm__ ("fucom %%st(1);"
	"fnstsw;": "=a" (retval) : "t" (x), "u" (y));
      return retval;
  }

#define isgreater(x, y) ((__fp_unordered_compare(x, y) \
  & 0x4500) == 0)
#define isless(x, y) ((__fp_unordered_compare (y, x) \
  & 0x4500) == 0)
#define isgreaterequal(x, y) ((__fp_unordered_compare (x, y) \
  & FP_INFINITE) == 0)
#define islessequal(x, y) ((__fp_unordered_compare(y, x) \
  & FP_INFINITE) == 0)
#define islessgreater(x, y) ((__fp_unordered_compare(x, y) \
  & FP_SUBNORMAL) == 0)
#define isunordered(x, y) ((__fp_unordered_compare(x, y) \
  & 0x4500) == 0x4500)

#endif


#endif /* __STDC_VERSION__ >= 199901L */
#endif /* __NO_ISOCEXT */

#ifdef __cplusplus
}
extern "C++" {
  template<class _Ty> inline _Ty _Pow_int(_Ty _X,int _Y) {
    unsigned int _N;
    if(_Y >= 0) _N = (unsigned int)_Y;
    else _N = (unsigned int)(-_Y);
    for(_Ty _Z = _Ty(1);;_X *= _X) {
      if((_N & 1)!=0) _Z *= _X;
      if((_N >>= 1)==0) return (_Y < 0 ? _Ty(1) / _Z : _Z); 
    }
  }
}
#endif

#pragma pack(pop)

#if !defined(__STRICT_ANSI__) && !defined(_MATH_DEFINES_DEFINED)
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
#endif

#ifndef __MINGW_FPCLASS_DEFINED
#define __MINGW_FPCLASS_DEFINED 1
#define	_FPCLASS_SNAN	0x0001	/* Signaling "Not a Number" */
#define	_FPCLASS_QNAN	0x0002	/* Quiet "Not a Number" */
#define	_FPCLASS_NINF	0x0004	/* Negative Infinity */
#define	_FPCLASS_NN	0x0008	/* Negative Normal */
#define	_FPCLASS_ND	0x0010	/* Negative Denormal */
#define	_FPCLASS_NZ	0x0020	/* Negative Zero */
#define	_FPCLASS_PZ	0x0040	/* Positive Zero */
#define	_FPCLASS_PD	0x0080	/* Positive Denormal */
#define	_FPCLASS_PN	0x0100	/* Positive Normal */
#define	_FPCLASS_PINF	0x0200	/* Positive Infinity */
#endif /* __MINGW_FPCLASS_DEFINED */

/* 7.12.14 */
/* 
 *  With these functions, comparisons involving quiet NaNs set the FP
 *  condition code to "unordered".  The IEEE floating-point spec
 *  dictates that the result of floating-point comparisons should be
 *  false whenever a NaN is involved, with the exception of the != op, 
 *  which always returns true: yes, (NaN != NaN) is true).
 */

#if __GNUC__ >= 3

#define isgreater(x, y) __builtin_isgreater(x, y)
#define isgreaterequal(x, y) __builtin_isgreaterequal(x, y)
#define isless(x, y) __builtin_isless(x, y)
#define islessequal(x, y) __builtin_islessequal(x, y)
#define islessgreater(x, y) __builtin_islessgreater(x, y)
#define isunordered(x, y) __builtin_isunordered(x, y)

#else
/*  helper  */
__CRT_INLINE int  __cdecl
__fp_unordered_compare (long double x, long double y){
  unsigned short retval;
  __asm__ ("fucom %%st(1);"
	   "fnstsw;": "=a" (retval) : "t" (x), "u" (y));
  return retval;
}

#define isgreater(x, y) ((__fp_unordered_compare(x, y) \
			   & 0x4500) == 0)
#define isless(x, y) ((__fp_unordered_compare (y, x) \
                       & 0x4500) == 0)
#define isgreaterequal(x, y) ((__fp_unordered_compare (x, y) \
                               & FP_INFINITE) == 0)
#define islessequal(x, y) ((__fp_unordered_compare(y, x) \
			    & FP_INFINITE) == 0)
#define islessgreater(x, y) ((__fp_unordered_compare(x, y) \
			      & FP_SUBNORMAL) == 0)
#define isunordered(x, y) ((__fp_unordered_compare(x, y) \
			    & 0x4500) == 0x4500)

#endif

#endif /* End _MATH_H_ */

