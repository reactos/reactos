/* 
 * math.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Mathematical functions.
 *
 */


#ifndef _MATH_H_
#define _MATH_H_

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

/* All the headers include this file. */
#include <_mingw.h>

/*
 * Types for the _exception structure.
 */

#define	_DOMAIN		1	/* domain error in argument */
#define	_SING		2	/* singularity */
#define	_OVERFLOW	3	/* range overflow */
#define	_UNDERFLOW	4	/* range underflow */
#define	_TLOSS		5	/* total loss of precision */
#define	_PLOSS		6	/* partial loss of precision */

/*
 * Exception types with non-ANSI names for compatibility.
 */

#ifndef	__STRICT_ANSI__
#ifndef	_NO_OLDNAMES

#define	DOMAIN		_DOMAIN
#define	SING		_SING
#define	OVERFLOW	_OVERFLOW
#define	UNDERFLOW	_UNDERFLOW
#define	TLOSS		_TLOSS
#define	PLOSS		_PLOSS

#endif	/* Not _NO_OLDNAMES */
#endif	/* Not __STRICT_ANSI__ */


/* Traditional/XOPEN math constants (double precison) */
#ifndef __STRICT_ANSI__
#define M_E		2.7182818284590452354
#define M_LOG2E		1.4426950408889634074
#define M_LOG10E	0.43429448190325182765
#define M_LN2		0.69314718055994530942
#define M_LN10		2.30258509299404568402
#define M_PI		3.14159265358979323846
#define M_PI_2		1.57079632679489661923
#define M_PI_4		0.78539816339744830962
#define M_1_PI		0.31830988618379067154
#define M_2_PI		0.63661977236758134308
#define M_2_SQRTPI	1.12837916709551257390
#define M_SQRT2		1.41421356237309504880
#define M_SQRT1_2	0.70710678118654752440
#endif

/* These are also defined in Mingw float.h; needed here as well to work 
   around GCC build issues.  */
#ifndef	__STRICT_ANSI__
#ifndef __MINGW_FPCLASS_DEFINED
#define __MINGW_FPCLASS_DEFINED 1
/* IEEE 754 classication */
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
#endif	/* Not __STRICT_ANSI__ */

#ifndef RC_INVOKED

#ifdef __cplusplus
extern "C" {
#endif

/*
 * HUGE_VAL is returned by strtod when the value would overflow the
 * representation of 'double'. There are other uses as well.
 *
 * __imp__HUGE is a pointer to the actual variable _HUGE in
 * MSVCRT.DLL. If we used _HUGE directly we would get a pointer
 * to a thunk function.
 *
 * NOTE: The CRTDLL version uses _HUGE_dll instead.
 */

#if __MINGW_GNUC_PREREQ(3, 3)
#define	HUGE_VAL __builtin_huge_val()
#else

#ifndef __DECLSPEC_SUPPORTED

#ifdef __MSVCRT__
extern double*	_imp___HUGE;
#define	HUGE_VAL	(*_imp___HUGE)
#else
/* CRTDLL */
extern double*	_imp___HUGE_dll;
#define	HUGE_VAL	(*_imp___HUGE_dll)
#endif

#else /* __DECLSPEC_SUPPORTED */

#ifdef __MSVCRT__
__MINGW_IMPORT double	_HUGE;
#define	HUGE_VAL	_HUGE
#else
/* CRTDLL */
__MINGW_IMPORT double	_HUGE_dll;
#define	HUGE_VAL	_HUGE_dll
#endif

#endif /* __DECLSPEC_SUPPORTED */
#endif /* __MINGW_GNUC_PREREQ(3, 3) */


struct _exception
{
	int	type;
	char	*name;
	double	arg1;
	double	arg2;
	double	retval;
};

_CRTIMP double __cdecl sin (double);
_CRTIMP double __cdecl cos (double);
_CRTIMP double __cdecl tan (double);
_CRTIMP double __cdecl sinh (double);
_CRTIMP double __cdecl cosh (double);
_CRTIMP double __cdecl tanh (double);
_CRTIMP double __cdecl asin (double);
_CRTIMP double __cdecl acos (double);
_CRTIMP double __cdecl atan (double);
_CRTIMP double __cdecl atan2 (double, double);
_CRTIMP double __cdecl exp (double);
_CRTIMP double __cdecl log (double);
_CRTIMP double __cdecl log10 (double);
_CRTIMP	double __cdecl pow (double, double);
_CRTIMP double __cdecl sqrt (double);
_CRTIMP double __cdecl ceil (double);
_CRTIMP double __cdecl floor (double);
_CRTIMP double __cdecl fabs (double);
_CRTIMP double __cdecl ldexp (double, int);
_CRTIMP double __cdecl frexp (double, int*);
_CRTIMP double __cdecl modf (double, double*);
_CRTIMP double __cdecl fmod (double, double);

/* Excess precision when using a 64-bit mantissa for FPU math ops can
   cause unexpected results with some of the MSVCRT math functions.  For
   example, unless the function return value is stored (truncating to
   53-bit mantissa), calls to pow with both x and y as integral values
   sometimes produce a non-integral result.
   One workaround is to reset the FPU env to 53-bit mantissa
   by a call to fesetenv (FE_PC53_ENV).  Amother is to force storage
   of the return value of individual math functions using wrappers.
   NB, using these wrappers will disable builtin math functions and
   hence disable the folding of function results at compile time when
   arguments are constant.  */

#if 0
#define __DEFINE_FLOAT_STORE_MATHFN_D1(fn1)	\
static __inline__ double			\
__float_store_ ## fn1 (double x)		\
{						\
   __volatile__ double res = (fn1) (x);		\
  return res;					\
}

#define __DEFINE_FLOAT_STORE_MATHFN_D2(fn2)	\
static __inline__ double			\
__float_store_ ## fn2 (double x, double y)	\
{						\
  __volatile__ double res = (fn2) (x, y);	\
  return res;					\
}
#endif

/* For example, here is how to force the result of the pow function
   to be stored:   */
#if 0
#undef pow
/* Define the ___float_store_pow function and use it instead of pow().  */
__DEFINE_FLOAT_STORE_MATHFN_D2 (pow)
#define pow __float_store_pow
#endif

#ifndef __STRICT_ANSI__

/* Complex number (for _cabs). This is the MS version. The ISO
   C99 counterpart _Complex is an intrinsic type in GCC and
   'complex' is defined as a macro.  See complex.h  */
struct _complex
{
	double	x;	/* Real part */
	double	y;	/* Imaginary part */
};

_CRTIMP double __cdecl _cabs (struct _complex);

_CRTIMP double __cdecl _hypot (double, double);
_CRTIMP double __cdecl _j0 (double);
_CRTIMP double __cdecl _j1 (double);
_CRTIMP double __cdecl _jn (int, double);
_CRTIMP double __cdecl _y0 (double);
_CRTIMP double __cdecl _y1 (double);
_CRTIMP double __cdecl _yn (int, double);
_CRTIMP int __cdecl _matherr (struct _exception *);

/* These are also declared in Mingw float.h; needed here as well to work 
   around GCC build issues.  */
/* BEGIN FLOAT.H COPY */
/*
 * IEEE recommended functions
 */

_CRTIMP double __cdecl _chgsign (double);
_CRTIMP double __cdecl _copysign (double, double);
_CRTIMP double __cdecl _logb (double);
_CRTIMP double __cdecl _nextafter (double, double);
_CRTIMP double __cdecl _scalb (double, long);

_CRTIMP int __cdecl _finite (double);
_CRTIMP int __cdecl _fpclass (double);
_CRTIMP int __cdecl _isnan (double);

/* END FLOAT.H COPY */


/*
 * Non-underscored versions of non-ANSI functions.
 * These reside in liboldnames.a.
 */

#if !defined (_NO_OLDNAMES)

_CRTIMP double __cdecl j0 (double);
_CRTIMP double __cdecl j1 (double);
_CRTIMP double __cdecl jn (int, double);
_CRTIMP double __cdecl y0 (double);
_CRTIMP double __cdecl y1 (double);
_CRTIMP double __cdecl yn (int, double);

_CRTIMP double __cdecl chgsign (double);
_CRTIMP double __cdecl scalb (double, long);
_CRTIMP int __cdecl finite (double);
_CRTIMP int __cdecl fpclass (double);

#define FP_SNAN    _FPCLASS_SNAN
#define FP_QNAN    _FPCLASS_QNAN
#define FP_NINF    _FPCLASS_NINF
#define FP_PINF    _FPCLASS_PINF
#define FP_NDENORM _FPCLASS_ND
#define FP_PDENORM _FPCLASS_PD
#define FP_NZERO   _FPCLASS_NZ
#define FP_PZERO   _FPCLASS_PZ
#define FP_NNORM   _FPCLASS_NN
#define FP_PNORM   _FPCLASS_PN

#endif /* Not _NO_OLDNAMES */

/* This require msvcr70.dll or higher. */ 
#if __MSVCRT_VERSION__ >= 0x0700
_CRTIMP int __cdecl _set_SSE2_enable (int);
#endif /* __MSVCRT_VERSION__ >= 0x0700 */


#endif /* __STRICT_ANSI__ */


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
  return (stw & 0x0200) != 0;
}

__CRT_INLINE int __cdecl __signbitf (float x) {
  unsigned short stw;
  __asm__ ("fxam; fstsw %%ax;": "=a" (stw) : "t" (x));
  return (stw & 0x0200) != 0;
}

__CRT_INLINE int __cdecl __signbitl (long double x) {
  unsigned short stw;
  __asm__ ("fxam; fstsw %%ax;": "=a" (stw) : "t" (x));
  return (stw & 0x0200) != 0;
}

#define signbit(x) (sizeof (x) == sizeof (float) ? __signbitf (x)	\
		    : sizeof (x) == sizeof (double) ? __signbit (x)	\
		    : __signbitl (x))

/* 7.12.4 Trigonometric functions: Double in C89 */
extern float __cdecl sinf (float);
extern long double __cdecl sinl (long double);

extern float __cdecl cosf (float);
extern long double __cdecl cosl (long double);

extern float __cdecl tanf (float);
extern long double __cdecl tanl (long double);

extern float __cdecl asinf (float);
extern long double __cdecl asinl (long double);

extern float __cdecl acosf (float);
extern long double __cdecl acosl (long double);

extern float __cdecl atanf (float);
extern long double __cdecl atanl (long double);

extern float __cdecl atan2f (float, float);
extern long double __cdecl atan2l (long double, long double);

/* 7.12.5 Hyperbolic functions: Double in C89  */
__CRT_INLINE float __cdecl sinhf (float x)
  {return (float) sinh (x);}
extern long double __cdecl sinhl (long double);

__CRT_INLINE float __cdecl coshf (float x)
  {return (float) cosh (x);}
extern long double __cdecl coshl (long double);

__CRT_INLINE float __cdecl tanhf (float x)
  {return (float) tanh (x);}
extern long double __cdecl tanhl (long double);

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
extern float __cdecl atanf  (float);
extern long double __cdecl atanhl (long double);

/* Exponentials and logarithms  */
/* 7.12.6.1 Double in C89 */
__CRT_INLINE float __cdecl expf (float x)
  {return (float) exp (x);}
extern long double __cdecl expl (long double);

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
__CRT_INLINE float __cdecl frexpf (float x, int* expn)
  {return (float) frexp (x, expn);}
extern long double __cdecl frexpl (long double, int*);

/* 7.12.6.5 */
#define FP_ILOGB0 ((int)0x80000000)
#define FP_ILOGBNAN ((int)0x80000000)
extern int __cdecl ilogb (double);
extern int __cdecl ilogbf (float);
extern int __cdecl ilogbl (long double);

/* 7.12.6.6  Double in C89 */
__CRT_INLINE float __cdecl ldexpf (float x, int expn)
  {return (float) ldexp (x, expn);}
extern long double __cdecl ldexpl (long double, int);

/* 7.12.6.7 Double in C89 */
extern float __cdecl logf (float);
extern long double __cdecl logl (long double);

/* 7.12.6.8 Double in C89 */
extern float __cdecl log10f (float);
extern long double __cdecl log10l (long double);

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
#if !(__MINGW_GNUC_PREREQ (4, 0) && defined __FAST_MATH__ )
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
#endif /* !defined __FAST_MATH__ || !__MINGW_GNUC_PREREQ (4, 0) */

/* 7.12.6.12  Double in C89 */
extern float __cdecl modff (float, float*);
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

/* 7.12.7.2 The fabs functions: Double in C89 */
extern  float __cdecl fabsf (float x);
extern long double __cdecl fabsl (long double x);

/* 7.12.7.3  */
extern double __cdecl hypot (double, double); /* in libmoldname.a */
__CRT_INLINE float __cdecl hypotf (float x, float y)
  { return (float) hypot (x, y);}
extern long double __cdecl hypotl (long double, long double);

/* 7.12.7.4 The pow functions. Double in C89 */
__CRT_INLINE float __cdecl powf (float x, float y)
  {return (float) pow (x, y);}
extern long double __cdecl powl (long double, long double);

/* 7.12.7.5 The sqrt functions. Double in C89. */
extern float __cdecl sqrtf (float);
extern long double __cdecl sqrtl (long double);

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
extern float __cdecl ceilf (float);
extern long double __cdecl ceill (long double);

/* 7.12.9.2 Double in C89 */
extern float __cdecl floorf (float);
extern long double __cdecl floorl (long double);

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

extern long long __cdecl llrint (double);
extern long long __cdecl llrintf (float);
extern long long __cdecl llrintl (long double);

/* Inline versions of above. 
   GCC 4.0+ can do a better fast-math job with __builtins. */
#if !(__MINGW_GNUC_PREREQ (4, 0) && defined __FAST_MATH__ )
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

__CRT_INLINE long __cdecl lrint (double x) 
{
  long retval;  
  __asm__ __volatile__
    ("fistpl %0"  : "=m" (retval) : "t" (x) : "st");
  return retval;
}

__CRT_INLINE long __cdecl lrintf (float x) 
{
  long retval;
  __asm__ __volatile__
    ("fistpl %0"  : "=m" (retval) : "t" (x) : "st");
  return retval;
}

__CRT_INLINE long __cdecl lrintl (long double x) 
{
  long retval;
  __asm__ __volatile__
    ("fistpl %0"  : "=m" (retval) : "t" (x) : "st");
  return retval;
}

__CRT_INLINE long long __cdecl llrint (double x)
{
  long long retval;
  __asm__ __volatile__
    ("fistpll %0"  : "=m" (retval) : "t" (x) : "st");
  return retval;
}

__CRT_INLINE long long __cdecl llrintf (float x)
{
  long long retval;
  __asm__ __volatile__
    ("fistpll %0"  : "=m" (retval) : "t" (x) : "st");
  return retval;
}

__CRT_INLINE long long __cdecl llrintl (long double x) 
{
  long long retval;
  __asm__ __volatile__
    ("fistpll %0"  : "=m" (retval) : "t" (x) : "st");
  return retval;
}
#endif /* !__FAST_MATH__ || !__MINGW_GNUC_PREREQ (4,0)  */

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

/* 7.12.10.1 Double in C89 */
extern float __cdecl fmodf (float, float);
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
#endif
#endif	/* Not RC_INVOKED */


#endif	/* Not _MATH_H_ */
