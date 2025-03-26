
#ifndef __GNUC__
#error This file should be included only with GCC compiler
#endif

#ifndef __NO_ISOCEXT
#if (defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
	|| !defined __STRICT_ANSI__ || defined __cplusplus

#if __MINGW_GNUC_PREREQ(3, 3)
#define HUGE_VALF	__builtin_huge_valf()
#define HUGE_VALL	__builtin_huge_vall()
#define INFINITY	__builtin_inf()
#define NAN		__builtin_nan("")
#else
extern const float __INFF;
#define HUGE_VALF __INFF
extern const long double  __INFL;
#define HUGE_VALL __INFL
#define INFINITY HUGE_VALF
extern const double __QNAN;
#define NAN __QNAN
#endif /* __MINGW_GNUC_PREREQ(3, 3) */

/* Use the compiler's builtin define for FLT_EVAL_METHOD to
   set float_t and double_t.  */
#if defined(__FLT_EVAL_METHOD__)
# if ( __FLT_EVAL_METHOD__== 0)
typedef float float_t;
typedef double double_t;
# elif (__FLT_EVAL_METHOD__ == 1)
typedef double float_t;
typedef double double_t;
# elif (__FLT_EVAL_METHOD__ == 2)
typedef long double float_t;
typedef long double double_t;
#endif
#else /* ix87 FPU default */
typedef long double float_t;
typedef long double double_t;
#endif

/* 7.12.3.1 */
/*
   Return values for fpclassify.
   These are based on Intel x87 fpu condition codes
   in the high byte of status word and differ from
   the return values for MS IEEE 754 extension _fpclass()
*/
#define FP_NAN		0x0100
#define FP_NORMAL	0x0400
#define FP_INFINITE	(FP_NAN | FP_NORMAL)
#define FP_ZERO		0x4000
#define FP_SUBNORMAL	(FP_NORMAL | FP_ZERO)
/* 0x0200 is signbit mask */

/*
  We can't inline float or double, because we want to ensure truncation
  to semantic type before classification.
  (A normal long double value might become subnormal when
  converted to double, and zero when converted to float.)
*/

  extern int __cdecl __fpclassifyl (long double);
  extern int __cdecl __fpclassifyf (float);
  extern int __cdecl __fpclassify (double);

#ifndef __CRT__NO_INLINE
  __CRT_INLINE int __cdecl __fpclassifyl (long double x) {
    unsigned short sw;
    __asm__ __volatile__ ("fxam; fstsw %%ax;" : "=a" (sw): "t" (x));
    return sw & (FP_NAN | FP_NORMAL | FP_ZERO );
  }
  __CRT_INLINE int __cdecl __fpclassify (double x) {
    unsigned short sw;
    __asm__ __volatile__ ("fxam; fstsw %%ax;" : "=a" (sw): "t" (x));
    return sw & (FP_NAN | FP_NORMAL | FP_ZERO );
  }
  __CRT_INLINE int __cdecl __fpclassifyf (float x) {
    unsigned short sw;
    __asm__ __volatile__ ("fxam; fstsw %%ax;" : "=a" (sw): "t" (x));
    return sw & (FP_NAN | FP_NORMAL | FP_ZERO );
  }
#endif

#define fpclassify(x) (sizeof (x) == sizeof (float) ? __fpclassifyf (x)	  \
  : sizeof (x) == sizeof (double) ? __fpclassify (x) \
  : __fpclassifyl (x))

/* 7.12.3.2 */
#define isfinite(x) ((fpclassify(x) & FP_NAN) == 0)

/* 7.12.3.3 */
#define isinf(x) (fpclassify(x) == FP_INFINITE)

/* 7.12.3.4 */
/* We don't need to worry about truncation here:
   A NaN stays a NaN. */

  extern int __cdecl __isnan (double);
  extern int __cdecl __isnanf (float);
  extern int __cdecl __isnanl (long double);

#ifndef __CRT__NO_INLINE
  __CRT_INLINE int __cdecl __isnan (double _x)
  {
    unsigned short sw;
    __asm__ __volatile__ ("fxam;"
      "fstsw %%ax": "=a" (sw) : "t" (_x));
    return (sw & (FP_NAN | FP_NORMAL | FP_INFINITE | FP_ZERO | FP_SUBNORMAL))
      == FP_NAN;
  }

  __CRT_INLINE int __cdecl __isnanf (float _x)
  {
    unsigned short sw;
    __asm__ __volatile__ ("fxam;"
      "fstsw %%ax": "=a" (sw) : "t" (_x));
    return (sw & (FP_NAN | FP_NORMAL | FP_INFINITE | FP_ZERO | FP_SUBNORMAL))
      == FP_NAN;
  }

  __CRT_INLINE int __cdecl __isnanl (long double _x)
  {
    unsigned short sw;
    __asm__ __volatile__ ("fxam;"
      "fstsw %%ax": "=a" (sw) : "t" (_x));
    return (sw & (FP_NAN | FP_NORMAL | FP_INFINITE | FP_ZERO | FP_SUBNORMAL))
      == FP_NAN;
  }
#endif

#define isnan(x) (sizeof (x) == sizeof (float) ? __isnanf (x)	\
  : sizeof (x) == sizeof (double) ? __isnan (x)	\
  : __isnanl (x))

/* 7.12.3.5 */
#define isnormal(x) (fpclassify(x) == FP_NORMAL)

/* 7.12.3.6 The signbit macro */
  extern int __cdecl __signbit (double);
  extern int __cdecl __signbitf (float);
  extern int __cdecl __signbitl (long double);
#ifndef __CRT__NO_INLINE
  __CRT_INLINE int __cdecl __signbit (double x) {
    unsigned short stw;
    __asm__ __volatile__ ( "fxam; fstsw %%ax;": "=a" (stw) : "t" (x));
    return stw & 0x0200;
  }

  __CRT_INLINE int __cdecl __signbitf (float x) {
    unsigned short stw;
    __asm__ __volatile__ ("fxam; fstsw %%ax;": "=a" (stw) : "t" (x));
    return stw & 0x0200;
  }

  __CRT_INLINE int __cdecl __signbitl (long double x) {
    unsigned short stw;
    __asm__ __volatile__ ("fxam; fstsw %%ax;": "=a" (stw) : "t" (x));
    return stw & 0x0200;
  }
#endif

#define signbit(x) (sizeof (x) == sizeof (float) ? __signbitf (x)	\
  : sizeof (x) == sizeof (double) ? __signbit (x)	\
  : __signbitl (x))

/* 7.12.4 Trigonometric functions: Double in C89 */
  // Already in math.h

/* 7.12.5 Hyperbolic functions: Double in C89  */
  // Already in math.h

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

/* Exponentials and logarithms  */
/* 7.12.6.1 Double in C89 */
  // exp functions. Already in math.h

/* 7.12.6.2 */
  extern double __cdecl exp2(double);
  extern float __cdecl exp2f(float);
  extern long double __cdecl exp2l(long double);

/* 7.12.6.3 The expm1 functions */
/* TODO: These could be inlined */
  extern double __cdecl expm1(double);
  extern float __cdecl expm1f(float);
  extern long double __cdecl expm1l(long double);

/* 7.12.6.4 Double in C89 */
  // frexp functions. Already in math.h

/* 7.12.6.5 */
#define FP_ILOGB0 ((int)0x80000000)
#define FP_ILOGBNAN ((int)0x80000000)
  extern int __cdecl ilogb (double);
  extern int __cdecl ilogbf (float);
  extern int __cdecl ilogbl (long double);

/* 7.12.6.6  Double in C89 */
  // ldexp functions. Already in math.h

/* 7.12.6.7 Double in C89 */
  // log functions. Already in math.h

/* 7.12.6.8 Double in C89 */
  // log10 functions. Already in math.h

/* 7.12.6.9 */
  extern double __cdecl log1p(double);
  extern float __cdecl log1pf(float);
  extern long double __cdecl log1pl(long double);

/* 7.12.6.10 */
  extern double __cdecl log2 (double);
  extern float __cdecl log2f (float);
  extern long double __cdecl log2l (long double);

/* 7.12.6.11 */
  extern double __cdecl logb (double);
  extern float __cdecl logbf (float);
  extern long double __cdecl logbl (long double);

/* Inline versions.  GCC-4.0+ can do a better fast-math optimization
   with __builtins. */
#ifndef __CRT__NO_INLINE
#if !(__MINGW_GNUC_PREREQ (4, 0) && defined (__FAST_MATH__))
  __CRT_INLINE double __cdecl logb (double x)
  {
    double res = 0.0;
    __asm__ __volatile__ ("fxtract\n\t"
      "fstp	%%st" : "=t" (res) : "0" (x));
    return res;
  }

  __CRT_INLINE float __cdecl logbf (float x)
  {
    float res = 0.0F;
    __asm__ __volatile__ ("fxtract\n\t"
      "fstp	%%st" : "=t" (res) : "0" (x));
    return res;
  }

  __CRT_INLINE long double __cdecl logbl (long double x)
  {
    long double res = 0.0l;
    __asm__ __volatile__ ("fxtract\n\t"
      "fstp	%%st" : "=t" (res) : "0" (x));
    return res;
  }
#endif /* !defined __FAST_MATH__ || !__MINGW_GNUC_PREREQ (4, 0) */
#endif /* __CRT__NO_INLINE */

/* 7.12.6.12  Double in C89 */
  // modf functions. Already in math.h

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

/* 7.12.7.2 The fabs functions: Double in C89 */
  // Already in math.h

/* 7.12.7.3  */
  // hypot functions. Already in math.h

/* 7.12.7.4 The pow functions. Double in C89 */
  // Already in math.h

/* 7.12.7.5 The sqrt functions. Double in C89. */
  // Already in math.h

/* 7.12.8.1 The erf functions  */
  extern double __cdecl erf (double);
  extern float __cdecl erff (float);
  extern long double __cdecl erfl (long double);

/* 7.12.8.2 The erfc functions  */
  extern double __cdecl erfc (double);
  extern float __cdecl erfcf (float);
  extern long double __cdecl erfcl (long double);

/* 7.12.8.3 The lgamma functions */
  extern double __cdecl lgamma (double);
  extern float __cdecl lgammaf (float);
  extern long double __cdecl lgammal (long double);

/* 7.12.8.4 The tgamma functions */
  extern double __cdecl tgamma (double);
  extern float __cdecl tgammaf (float);
  extern long double __cdecl tgammal (long double);

/* 7.12.9.1 Double in C89 */
  // ceil functions. Already in math.h

/* 7.12.9.2 Double in C89 */
  // floor functions. Already in math.h

/* 7.12.9.3 */
  extern double __cdecl nearbyint ( double);
  extern float __cdecl nearbyintf (float);
  extern long double __cdecl nearbyintl (long double);

/* 7.12.9.4 */
/* round, using fpu control word settings */
extern double __cdecl rint (double);
extern float __cdecl rintf (float);
extern long double __cdecl rintl (long double);

/* 7.12.9.5 */
extern long __cdecl lrint (double);
extern long __cdecl lrintf (float);
extern long __cdecl lrintl (long double);

__MINGW_EXTENSION long long __cdecl llrint (double);
__MINGW_EXTENSION long long __cdecl llrintf (float);
__MINGW_EXTENSION long long __cdecl llrintl (long double);

/* Inline versions of above.
   GCC 4.0+ can do a better fast-math job with __builtins. */

#ifndef __CRT__NO_INLINE
#if !(__MINGW_GNUC_PREREQ (4, 0) && defined __FAST_MATH__ )
  __CRT_INLINE double __cdecl rint (double x)
  {
    double retval = 0.0;
    __asm__ __volatile__ ("frndint;": "=t" (retval) : "0" (x));
    return retval;
  }

  __CRT_INLINE float __cdecl rintf (float x)
  {
    float retval = 0.0;
    __asm__ __volatile__ ("frndint;" : "=t" (retval) : "0" (x) );
    return retval;
  }

  __CRT_INLINE long double __cdecl rintl (long double x)
  {
    long double retval = 0.0l;
    __asm__ __volatile__ ("frndint;" : "=t" (retval) : "0" (x) );
    return retval;
  }

  __CRT_INLINE long __cdecl lrint (double x)
  {
    long retval = 0;
    __asm__ __volatile__							      \
      ("fistpl %0"  : "=m" (retval) : "t" (x) : "st");				      \
      return retval;
  }

  __CRT_INLINE long __cdecl lrintf (float x)
  {
    long retval = 0;
    __asm__ __volatile__							      \
      ("fistpl %0"  : "=m" (retval) : "t" (x) : "st");				      \
      return retval;
  }

  __CRT_INLINE long __cdecl lrintl (long double x)
  {
    long retval = 0;
    __asm__ __volatile__							      \
      ("fistpl %0"  : "=m" (retval) : "t" (x) : "st");				      \
      return retval;
  }

  __MINGW_EXTENSION __CRT_INLINE long long __cdecl llrint (double x)
  {
    __MINGW_EXTENSION long long retval = 0ll;
    __asm__ __volatile__							      \
      ("fistpll %0"  : "=m" (retval) : "t" (x) : "st");				      \
      return retval;
  }

  __MINGW_EXTENSION __CRT_INLINE long long __cdecl llrintf (float x)
  {
    __MINGW_EXTENSION long long retval = 0ll;
    __asm__ __volatile__							      \
      ("fistpll %0"  : "=m" (retval) : "t" (x) : "st");				      \
      return retval;
  }

  __MINGW_EXTENSION __CRT_INLINE long long __cdecl llrintl (long double x)
  {
    __MINGW_EXTENSION long long retval = 0ll;
    __asm__ __volatile__							      \
      ("fistpll %0"  : "=m" (retval) : "t" (x) : "st");				      \
      return retval;
  }
#endif /* !__FAST_MATH__ || !__MINGW_GNUC_PREREQ (4,0)  */
#endif /* !__CRT__NO_INLINE */

/* 7.12.9.6 */
/* round away from zero, regardless of fpu control word settings */
  extern double __cdecl round (double);
  extern float __cdecl roundf (float);
  extern long double __cdecl roundl (long double);

/* 7.12.9.7  */
  extern long __cdecl lround (double);
  extern long __cdecl lroundf (float);
  extern long __cdecl lroundl (long double);
  __MINGW_EXTENSION long long __cdecl llround (double);
  __MINGW_EXTENSION long long __cdecl llroundf (float);
  __MINGW_EXTENSION long long __cdecl llroundl (long double);

/* 7.12.9.8 */
/* round towards zero, regardless of fpu control word settings */
  extern double __cdecl trunc (double);
  extern float __cdecl truncf (float);
  extern long double __cdecl truncl (long double);

/* 7.12.10.1 Double in C89 */
  // fmod functions. Already in math.h

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

/* 7.12.11.4 The nexttoward functions */
  extern double __cdecl nexttoward (double,  long double);
  extern float __cdecl nexttowardf (float,  long double);
  extern long double __cdecl nexttowardl (long double, long double);

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
#ifndef __CRT__NO_INLINE
  __CRT_INLINE int  __cdecl
    __fp_unordered_compare (long double x, long double y){
      unsigned short retval;
      __asm__ __volatile__ ("fucom %%st(1);"
	"fnstsw;": "=a" (retval) : "t" (x), "u" (y));
      return retval;
  }
#endif

#define isgreater(x, y) ((__fp_unordered_compare(x, y)  & 0x4500) == 0)
#define isless(x, y) ((__fp_unordered_compare (y, x)  & 0x4500) == 0)
#define isgreaterequal(x, y) ((__fp_unordered_compare (x, y)  & FP_INFINITE) == 0)
#define islessequal(x, y) ((__fp_unordered_compare(y, x)  & FP_INFINITE) == 0)
#define islessgreater(x, y) ((__fp_unordered_compare(x, y)  & FP_SUBNORMAL) == 0)
#define isunordered(x, y) ((__fp_unordered_compare(x, y)  & 0x4500) == 0x4500)

#endif

#endif /* C99 or non strict ANSI or C++ */
#endif /* __NO_ISOCEXT */
