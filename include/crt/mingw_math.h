

  float expm1f(float _X);
  double expm1(double _X);


#ifndef __NO_ISOCEXT
#if (defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
  || !defined __STRICT_ANSI__ || defined __GLIBCPP__

#if !defined(_MSC_VER)
#define NAN (0.0F/0.0F)
#define HUGE_VALF (1.0F/0.0F)
#define HUGE_VALL (1.0L/0.0L)
#define INFINITY (1.0F/0.0F)
#endif


#define FP_NAN		0x0100
#define FP_NORMAL	0x0400
#define FP_INFINITE	(FP_NAN | FP_NORMAL)
#define FP_ZERO		0x4000
#define FP_SUBNORMAL	(FP_NORMAL | FP_ZERO)
  /* 0x0200 is signbit mask */

#if defined(__GNUC__)

#define __fxam(x, sw) \
	__asm__ ("fxam; fstsw %%ax;" : "=a" (sw): "t" (x))

#elif defined(_MSC_VER)

#ifdef _M_IX86
#define __fxam(x, sw) \
    __asm { fld [(x)] } \
    __asm { fxam } \
    __asm { wait } \
    __asm { fnstsw [(sw)] } \
    __asm { fstp st(0) }
#else
#define __fxam(x, sw)
#pragma message("WARNING: __fxam is undefined")
#endif

#endif

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
    __fxam(x, sw);
    return sw & (FP_NAN | FP_NORMAL | FP_ZERO );
  }

  __CRT_INLINE int __cdecl __fpclassify (double x){
    return __fpclassifyl((long double)x);
  }

#define fpclassify(x) (sizeof (x) == sizeof (float) ? __fpclassifyf (x)	  \
  : sizeof (x) == sizeof (double) ? __fpclassify (x) \
  : __fpclassifyl (x))

  /* 7.12.3.2 */
#define isfinite(x) ((fpclassify(x) & FP_NAN) == 0)

  /* 7.12.3.3 */
/* #define isinf(x) (fpclassify(x) == FP_INFINITE) */

  /* we don't have fpclassify */
__CRT_INLINE int isinf (double d) {
  int expon = 0;
  double val = frexp (d, &expon);
  if (expon == 1025) {
    if (val == 0.5) {
        return 1;
    } else if (val == -0.5) {
        return -1;
    } else {
        return 0;
    }
  } else {
    return 0;
  }
}

  /* 7.12.3.4 */
  /* We don't need to worry about truncation here:
  A NaN stays a NaN. */

  __CRT_INLINE int __cdecl __isnan (double _x)
  {
    unsigned short sw;
    __fxam(_x, sw);
    return (sw & (FP_NAN | FP_NORMAL | FP_INFINITE | FP_ZERO | FP_SUBNORMAL))
      == FP_NAN;
  }

  __CRT_INLINE int __cdecl __isnanf (float _x)
  {
    unsigned short sw;
    __fxam(_x, sw);
    return (sw & (FP_NAN | FP_NORMAL | FP_INFINITE | FP_ZERO | FP_SUBNORMAL))
      == FP_NAN;
  }

  __CRT_INLINE int __cdecl __isnanl (long double _x)
  {
    unsigned short sw;
    __fxam(_x, sw);
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
    __fxam(x, stw);
    return stw & 0x0200;
  }

  __CRT_INLINE int __cdecl __signbitf (float x) {
    unsigned short stw;
    __fxam(x, stw);
    return stw & 0x0200;
  }

  __CRT_INLINE int __cdecl __signbitl (long double x) {
    unsigned short stw;
    __fxam(x, stw);
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

#if defined(__GNUC__)

#define __fxtract(x, res) \
    __asm__ ("fxtract\n\t" \
      "fstp	%%st" : "=t" (res) : "0" (x))

#elif defined(_MSC_VER)

#define __fxtract(x, res) \
    __asm { fld [(x)] } \
    __asm { fxtract } \
    __asm { fstp st(0) } \
    __asm { fstp [(res)] }

#endif

  __CRT_INLINE double __cdecl logb (double x)
  {
    double res;
    __fxtract(x, res);
    return res;
  }

  __CRT_INLINE float __cdecl logbf (float x)
  {
    float res;
    __fxtract(x, res);
    return res;
  }

  __CRT_INLINE long double __cdecl logbl (long double x)
  {
    long double res;
    __fxtract(x, res);
    return res;
  }

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

  extern long double __cdecl hypotl (long double, long double);

  extern long double __cdecl powl (long double, long double);
  extern long double __cdecl expl(long double);
  extern long double expm1l(long double);
  extern long double __cdecl coshl(long double);
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
#if defined(__GNUC__)

#define __frndint(x, res) \
  __asm__ ("fabs;" : "=t" (res) : "0" (x))

#elif defined(_MSC_VER)

#define __frndint(x, res) \
  __asm { fld [(x)] } \
  __asm { frndint } \
  __asm { fstp [(res)] }

#endif

  __CRT_INLINE double __cdecl rint (double x)
  {
    double retval;
    __frndint(x, retval);
    return retval;
  }

  __CRT_INLINE float __cdecl rintf (float x)
  {
    float retval;
    __frndint(x, retval);
    return retval;
  }

  __CRT_INLINE long double __cdecl rintl (long double x)
  {
    long double retval;
    __frndint(x, retval);
    return retval;
  }

  /* 7.12.9.5 */
#if defined(__GNUC__)

#define __fistpl(x, res) \
  __asm__ __volatile__("fistpl %0"  : "=m" (res) : "t" (x) : "st")

#define __fistpll(x, res) \
  __asm__ __volatile__("fistpll %0"  : "=m" (res) : "t" (x) : "st")

#elif defined(_MSC_VER)

#define __fistpl(x, res) \
  __asm { fld [(x)] } \
  __asm { fistp [(res)] }

#define __fistpll(x, res) \
  __asm { fld [(x)] } \
  __asm { fistp [(res)] }

#endif

  __CRT_INLINE long __cdecl lrint (double x)
  {
    long retval;
    __fistpl(x, retval);
    return retval;
  }

  __CRT_INLINE long __cdecl lrintf (float x)
  {
    long retval;
    __fistpl(x, retval);
    return retval;
  }

  __CRT_INLINE long __cdecl lrintl (long double x)
  {
    long retval;
    __fistpl(x, retval);
    return retval;
  }

  __MINGW_EXTENSION __CRT_INLINE long long __cdecl llrint (double x)
  {
    __MINGW_EXTENSION long long retval;
    __fistpll(x, retval);
    return retval;
  }

  __MINGW_EXTENSION __CRT_INLINE long long __cdecl llrintf (float x)
  {
    __MINGW_EXTENSION long long retval;
    __fistpll(x, retval);
    return retval;
  }

  __MINGW_EXTENSION __CRT_INLINE long long __cdecl llrintl (long double x)
  {
    __MINGW_EXTENSION long long retval;
    __fistpll(x, retval);
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

  __MINGW_EXTENSION extern long long __cdecl llround (double);
  __MINGW_EXTENSION extern long long __cdecl llroundf (float);
  __MINGW_EXTENSION extern long long __cdecl llroundl (long double);

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

#if defined(__GNUC__) && __GNUC__ >= 3

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
#if defined(__GNUC__)
      __asm__ ("fucom %%st(1);"
	"fnstsw;": "=a" (retval) : "t" (x), "u" (y));
#elif defined(_MSC_VER)
      __asm {
        fld [x]
        fld [y]
        fxch st(1)
        fucom st(1)
        fnstsw [retval]
        fstp st(0)
        fstp st(0)
      }
#endif
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
