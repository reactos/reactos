/*
 * Copyright (c) 1999
 * Boris Fomitchev
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */

#ifndef _STLP_INTERNAL_CMATH
#define _STLP_INTERNAL_CMATH

/* gcc do not like when a using directive appear after a function
 * declaration. cmath have abs overloads and cstdlib a using directive
 * so cstdlib has to be included first.
 */
#if defined (__GNUC__) && defined (_STLP_USE_NEW_C_HEADERS)
#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <cstdlib>
#  else
#    include _STLP_NATIVE_CPP_C_HEADER(cstdlib)
#  endif
#endif

#if defined (_STLP_USE_NEW_C_HEADERS)
#  if defined (_STLP_HAS_NO_NAMESPACES) && !defined (exception)
#    define exception __math_exception
#  endif
#  if defined (_STLP_HAS_INCLUDE_NEXT)
#    include_next <cmath>
#  else
#    include _STLP_NATIVE_CPP_C_HEADER(cmath)
#  endif
#  if defined (_STLP_HAS_NO_NAMESPACES)
#    undef exception
#  endif
#else
#  include <math.h>
#endif

#if (defined (__SUNPRO_CC) && (__SUNPRO_CC > 0x500)) || \
     !(defined (__IBMCPP__) && (__IBMCPP__ >= 500) || !(defined(__HP_aCC) && (__HP_aCC >= 30000) ))
#  if !defined(_STLP_HAS_NO_NAMESPACES) && !defined(__SUNPRO_CC)
// All the other hypot stuff is going to be at file scope, so follow along here.
namespace std {
#  endif
extern "C" double hypot(double x, double y);
#  if !defined(_STLP_HAS_NO_NAMESPACES) && !defined(__SUNPRO_CC)
}
#  endif

#endif

#if defined (__sun) && defined (__GNUC__)
extern "C" {
  float __cosf(float v);
  float __sinf(float v);
  float __atan2f(float, float);
  float __coshf(float v);
  float __sinhf(float v);
  float __sqrtf(float v);
  float __expf(float v);
  float __logf(float v);
  float __log10f(float v);

  long double __cosl(long double v);
  long double __sinl(long double v);
  long double __atan2l(long double, long double);
  long double __coshl(long double v);
  long double __sinhl(long double v);
  long double __sqrtl(long double v);
  long double __expl(long double v);
  long double __logl(long double v);
  long double __log10l(long double v);
}

extern "C" {
  inline float cosf(float v) { return __cosf(v); }
  inline float sinf(float v) { return __sinf(v); }
  inline float atan2f(float v1, float v2) { return __atan2f(v1,v2); }
  inline float coshf(float v) { return __coshf(v); }
  inline float sinhf(float v) { return __sinhf(v); }
  inline float sqrtf(float v) { return __sqrtf(v); }
  inline float expf(float v) { return __expf(v); }
  inline float logf(float v) { return __logf(v); }
  inline float log10f(float v) { return __log10f(v); }

  inline long double cosl(long double v) { return __cosl(v); }
  inline long double sinl(long double v) { return __sinl(v); }
  inline long double atan2l(long double v1, long double v2) { return __atan2l(v1,v2); }
  inline long double coshl(long double v) { return __coshl(v); }
  inline long double sinhl(long double v) { return __sinhl(v); }
  inline long double sqrtl(long double v) { return __sqrtl(v); }
  inline long double expl(long double v) { return __expl(v); }
  inline long double logl(long double v) { return __logl(v); }
  inline long double log10l(long double v) { return __log10l(v); }
}
#endif // __sun && __GNUC__

#if defined (__sun)
extern "C" {
extern float __acosf(float);
extern float __asinf(float);
extern float __atanf(float);
extern float __atan2f(float, float);
extern float __ceilf(float);
extern float __cosf(float);
extern float __coshf(float);
extern float __expf(float);
extern float __fabsf(float);
extern float __floorf(float);
extern float __fmodf(float, float);
extern float __frexpf(float, int *);
extern float __ldexpf(float, int);
extern float __logf(float);
extern float __log10f(float);
extern float __modff(float, float *);
extern float __powf(float, float);
extern float __sinf(float);
extern float __sinhf(float);
extern float __sqrtf(float);
extern float __tanf(float);
extern float __tanhf(float);

extern long double __acosl(long double);
extern long double __asinl(long double);
extern long double __atanl(long double);
extern long double __atan2l(long double, long double);
extern long double __ceill(long double);
extern long double __cosl(long double);
extern long double __coshl(long double);
extern long double __expl(long double);
extern long double __fabsl(long double);
extern long double __floorl(long double);
extern long double __fmodl(long double, long double);
extern long double __frexpl(long double, int *);
extern long double __ldexpl(long double, int);
extern long double __logl(long double);
extern long double __log10l(long double);
extern long double __modfl(long double, long double *);
extern long double __powl(long double, long double);
extern long double __sinl(long double);
extern long double __sinhl(long double);
extern long double __sqrtl(long double);
extern long double __tanl(long double);
extern long double __tanhl(long double);
}
#endif

#if defined (__BORLANDC__)
#  define _STLP_CMATH_FUNC_NAMESPACE _STLP_VENDOR_CSTD
#else
#  define _STLP_CMATH_FUNC_NAMESPACE
#endif

#if !defined (__sun) || defined (__GNUC__)
#  define _STLP_MATH_INLINE(float_type, func, cfunc) \
     inline float_type func (float_type x) { return _STLP_CMATH_FUNC_NAMESPACE::cfunc(x); }
#  define _STLP_MATH_INLINE2(float_type, type, func, cfunc) \
     inline float_type func (float_type x, type y) { return _STLP_CMATH_FUNC_NAMESPACE::cfunc(x, y); }
#  define _STLP_MATH_INLINE_D(float_type, func, cfunc)
#  define _STLP_MATH_INLINE2_D(float_type, type, func, cfunc)
#else
#  ifdef __SUNPRO_CC
#    define _STLP_MATH_INLINE(float_type, func, cfunc) \
       inline float_type func (float_type x) { return _STLP_VENDOR_CSTD::__##cfunc(x); }
#    define _STLP_MATH_INLINE_D(float_type, func, cfunc) \
       inline float_type func (float_type x) { return _STLP_VENDOR_CSTD::cfunc(x); }
#    define _STLP_MATH_INLINE2(float_type, type, func, cfunc) \
       inline float_type func (float_type x, type y) { return _STLP_VENDOR_CSTD::__##cfunc(x,y); }
#    define _STLP_MATH_INLINE2_D(float_type, type, func, cfunc) \
       inline float_type func (float_type x, type y) { return _STLP_VENDOR_CSTD::cfunc(x,y); }
#  else
#    error Unknown compiler for the Sun platform
#  endif
#endif

/** macros to define math functions
These macros (having an X somewhere in the name) forward to the C library's
double functions but cast the arguments and return values to the given type. */

#define _STLP_MATH_INLINEX(__type,func,cfunc) \
  inline __type func (__type x) \
  { return __STATIC_CAST(__type, _STLP_CMATH_FUNC_NAMESPACE::cfunc((double)x)); }
#define _STLP_MATH_INLINE2X(__type1,__type2,func,cfunc) \
  inline __type1 func (__type1 x, __type2 y) \
  { return __STATIC_CAST(__type1, _STLP_CMATH_FUNC_NAMESPACE::cfunc((double)x, y)); }
#define _STLP_MATH_INLINE2PX(__type,func,cfunc) \
  inline __type func (__type x, __type *y) { \
    double tmp1, tmp2; \
    tmp1 = _STLP_CMATH_FUNC_NAMESPACE::cfunc(__STATIC_CAST(double, x), &tmp2); \
    *y = __STATIC_CAST(__type, tmp2); \
    return __STATIC_CAST(__type, tmp1); \
  }
#define _STLP_MATH_INLINE2XX(__type,func,cfunc) \
  inline __type func (__type x, __type y) \
  { return __STATIC_CAST(__type, _STLP_CMATH_FUNC_NAMESPACE::cfunc((double)x, (double)y)); }


/** rough characterization of compiler and native C library
For the compiler, it can either support long double or not. If it doesn't, the
macro _STLP_NO_LONG_DOUBLE is not defined and we don't define any long double
overloads.
For the native C library the question is whether it has variants with an 'f'
suffix (for float as opposed to double) or an 'l' suffix (for long double). If
the float variants are missing, _STLP_NO_VENDOR_MATH_F is defined, when the
long double variants are missing, _STLP_NO_VENDOR_MATH_L is defined. Of course
the latter doesn't make sense anyway when the compiler already has no long
double support.

Those two traits determine a) which overloads get defined and b) how they are
defined.

Meaning of suffixes:
""   : function returning and taking a float_type
"2"  : function returning a float_type and taking to float_types
"2P" : function returning a float_type and taking a float_type and a float_type*
"2PI": function returning a float_type and taking a float_type and an int*
"2I" : function returning a float_type and taking a float_Type and an int
*/

#if !defined (_STLP_NO_LONG_DOUBLE) && !defined (_STLP_NO_VENDOR_MATH_L) && !defined (_STLP_NO_VENDOR_MATH_F)
   // long double support and both e.g. sinl(long double) and sinf(float)
   // This is the default for a correct and complete native library.
#  define _STLP_DEF_MATH_INLINE(func,cf) \
  _STLP_MATH_INLINE(float,func,cf##f) \
  _STLP_MATH_INLINE_D(double,func,cf) \
  _STLP_MATH_INLINE(long double,func,cf##l)
#  define _STLP_DEF_MATH_INLINE2(func,cf) \
  _STLP_MATH_INLINE2(float,float,func,cf##f) \
  _STLP_MATH_INLINE2_D(double,double,func,cf) \
  _STLP_MATH_INLINE2(long double,long double,func,cf##l)
#  define _STLP_DEF_MATH_INLINE2P(func,cf) \
  _STLP_MATH_INLINE2(float,float *,func,cf##f) \
  _STLP_MATH_INLINE2_D(double,double *,func,cf) \
  _STLP_MATH_INLINE2(long double,long double *,func,cf##l)
#  define _STLP_DEF_MATH_INLINE2PI(func,cf) \
  _STLP_MATH_INLINE2(float,int *,func,cf##f) \
  _STLP_MATH_INLINE2_D(double,int *,func,cf) \
  _STLP_MATH_INLINE2(long double,int *,func,cf##l)
#  define _STLP_DEF_MATH_INLINE2I(func,cf) \
  _STLP_MATH_INLINE2(float,int,func,cf##f) \
  _STLP_MATH_INLINE2_D(double,int,func,cf) \
  _STLP_MATH_INLINE2(long double,int,func,cf##l)
#else
#  if !defined (_STLP_NO_LONG_DOUBLE)
#    if !defined (_STLP_NO_VENDOR_MATH_F)
       // long double support and e.g. sinf(float) but not e.g. sinl(long double)
#      define _STLP_DEF_MATH_INLINE(func,cf) \
      _STLP_MATH_INLINE(float,func,cf##f) \
      _STLP_MATH_INLINEX(long double,func,cf)
#      define _STLP_DEF_MATH_INLINE2(func,cf) \
      _STLP_MATH_INLINE2(float,float,func,cf##f) \
      _STLP_MATH_INLINE2XX(long double,func,cf)
#      define _STLP_DEF_MATH_INLINE2P(func,cf) \
      _STLP_MATH_INLINE2(float,float *,func,cf##f) \
      _STLP_MATH_INLINE2PX(long double,func,cf)
#      define _STLP_DEF_MATH_INLINE2PI(func,cf) \
      _STLP_MATH_INLINE2(float,int *,func,cf##f) \
      _STLP_MATH_INLINE2X(long double,int *,func,cf)
#      define _STLP_DEF_MATH_INLINE2I(func,cf) \
      _STLP_MATH_INLINE2(float,int,func,cf##f) \
      _STLP_MATH_INLINE2X(long double,int,func,cf)
#    elif !defined (_STLP_NO_VENDOR_MATH_L)
       // long double support and e.g. sinl(long double) but not e.g. sinf(float)
#      define _STLP_DEF_MATH_INLINE(func,cf) \
      _STLP_MATH_INLINEX(float,func,cf) \
      _STLP_MATH_INLINE(long double,func,cf##l)
#      define _STLP_DEF_MATH_INLINE2(func,cf) \
      _STLP_MATH_INLINE2XX(float,func,cf) \
      _STLP_MATH_INLINE2(long double,long double,func,cf##l)
#      define _STLP_DEF_MATH_INLINE2P(func,cf) \
      _STLP_MATH_INLINE2PX(float,func,cf) \
      _STLP_MATH_INLINE2(long double,long double *,func,cf##l)
#      define _STLP_DEF_MATH_INLINE2PI(func,cf) \
      _STLP_MATH_INLINE2X(float,int *,func,cf) \
      _STLP_MATH_INLINE2(long double,int *,func,cf##l)
#      define _STLP_DEF_MATH_INLINE2I(func,cf) \
      _STLP_MATH_INLINE2X(float,int,func,cf) \
      _STLP_MATH_INLINE2(long double,int,func,cf##l)
#    else
#      define _STLP_DEF_MATH_INLINE(func,cf) \
      _STLP_MATH_INLINEX(float,func,cf) \
      _STLP_MATH_INLINEX(long double,func,cf)
#      define _STLP_DEF_MATH_INLINE2(func,cf) \
      _STLP_MATH_INLINE2XX(float,func,cf) \
      _STLP_MATH_INLINE2XX(long double,func,cf)
#      define _STLP_DEF_MATH_INLINE2P(func,cf) \
      _STLP_MATH_INLINE2PX(float,func,cf) \
      _STLP_MATH_INLINE2PX(long double,func,cf)
#      define _STLP_DEF_MATH_INLINE2PI(func,cf) \
      _STLP_MATH_INLINE2X(float,int *,func,cf) \
      _STLP_MATH_INLINE2X(long double,int *,func,cf)
#      define _STLP_DEF_MATH_INLINE2I(func,cf) \
      _STLP_MATH_INLINE2X(float,int,func,cf) \
      _STLP_MATH_INLINE2X(long double,int,func,cf)
#    endif
#  else
#    if !defined (_STLP_NO_VENDOR_MATH_F)
#      define _STLP_DEF_MATH_INLINE(func,cf) \
      _STLP_MATH_INLINE(float,func,cf##f)
#      define _STLP_DEF_MATH_INLINE2(func,cf) \
      _STLP_MATH_INLINE2(float,float,func,cf##f)
#      define _STLP_DEF_MATH_INLINE2P(func,cf) \
      _STLP_MATH_INLINE2(float,float *,func,cf##f)
#      define _STLP_DEF_MATH_INLINE2PI(func,cf) \
      _STLP_MATH_INLINE2(float,int *,func,cf##f)
#      define _STLP_DEF_MATH_INLINE2I(func,cf) \
      _STLP_MATH_INLINE2(float,int,func,cf##f)
#    else // _STLP_NO_VENDOR_MATH_F
       // neither long double support nor e.g. sinf(float) functions
#      define _STLP_DEF_MATH_INLINE(func,cf) \
      _STLP_MATH_INLINEX(float,func,cf)
#      define _STLP_DEF_MATH_INLINE2(func,cf) \
      _STLP_MATH_INLINE2XX(float,func,cf)
#      define _STLP_DEF_MATH_INLINE2P(func,cf) \
      _STLP_MATH_INLINE2PX(float,func,cf)
#      define _STLP_DEF_MATH_INLINE2PI(func,cf) \
      _STLP_MATH_INLINE2X(float,int *,func,cf)
#      define _STLP_DEF_MATH_INLINE2I(func,cf) \
      _STLP_MATH_INLINE2X(float,int,func,cf)
#    endif // _STLP_NO_VENDOR_MATH_F
#  endif
#endif

#if defined (_STLP_WCE) || \
   (defined(_STLP_MSVC) && (_STLP_MSVC <= 1300) && defined (_MSC_EXTENSIONS) /* && !defined(_STLP_WCE_NET) */)
/*
 * dums: VC6 has all the required C++ functions but only define them if
 * _MSC_EXTENSIONS is not defined (a bug?). STLport just do the same
 * thing also when _MSC_EXTENSIONS is defined.
 * TODO: above check (_STLP_MSVC <= 1300) also catches VC7.0, is that intended?
 */
//We have to tell the compilers that abs, acos ... math functions are not intrinsic
//otherwise we have Internal Compiler Error in release mode...
#  pragma warning(push)
#  pragma warning(disable: 4162) // no function with C linkage found
#  pragma warning(disable: 4163) // not available as an intrinsic function
#  pragma function (abs, acos, asin, atan, atan2, cos, cosh, exp, fabs, fmod, log, log10, sin, sinh, sqrt, tan, tanh)
#  if defined (_STLP_WCE)
#    pragma function (ceil, floor)
#  endif
#  define _STLP_RESTORE_FUNCTION_INTRINSIC
#endif // _STLP_MSVC && _STLP_MSVC <= 1300 && !_STLP_WCE && _MSC_EXTENSIONS

#if (defined (__BORLANDC__) || defined (__WATCOMC__)) && defined (_STLP_USE_NEW_C_HEADERS)
/* In this config Borland native lib only define functions in std namespace.
 * In order to have all overloads in STLport namespace we need to add the
 * double overload in global namespace. We do not use a using statement to avoid
 * import of invalid overload.
 */
#  define _STLP_DMATH_INLINE(func) _STLP_MATH_INLINE(double, func, func)
#  define _STLP_DMATH_INLINE2(func) _STLP_MATH_INLINE2(double, double, func, func)

_STLP_DMATH_INLINE(acos)
_STLP_DMATH_INLINE(asin)
_STLP_DMATH_INLINE(atan)
_STLP_DMATH_INLINE2(atan2)
_STLP_DMATH_INLINE(ceil)
_STLP_DMATH_INLINE(cos)
_STLP_DMATH_INLINE(cosh)
_STLP_DMATH_INLINE(exp)
_STLP_DMATH_INLINE(fabs)
_STLP_DMATH_INLINE(floor)
_STLP_DMATH_INLINE2(fmod)
_STLP_MATH_INLINE2X(double, int*, frexp, frexp)
_STLP_MATH_INLINE2X(double, int, ldexp, ldexp)
_STLP_DMATH_INLINE(log)
_STLP_DMATH_INLINE(log10)
_STLP_MATH_INLINE2PX(double, modf, modf)
_STLP_DMATH_INLINE(sin)
_STLP_DMATH_INLINE(sinh)
_STLP_DMATH_INLINE(sqrt)
_STLP_DMATH_INLINE(tan)
_STLP_DMATH_INLINE(tanh)
_STLP_DMATH_INLINE2(pow)
_STLP_DMATH_INLINE2(hypot)

#  undef _STLP_DMATH_INLINE
#  undef _STLP_DMATH_INLINE2
#endif

#if defined (__DMC__)
#  if defined (fabs)
inline double __stlp_fabs(double __x) { return fabs(__x); }
#    undef fabs
inline double fabs(double __x) { return __stlp_fabs(__x); }
#  endif
#  if defined (cos)
inline double __stlp_cos(double __x) { return cos(__x); }
#    undef cos
inline double cos(double __x) { return __stlp_cos(__x); }
#  endif
#  if defined (sin)
inline double __stlp_sin(double __x) { return sin(__x); }
#    undef sin
inline double sin(double __x) { return __stlp_sin(__x); }
#  endif
#  if defined (sqrt)
inline double __stlp_sqrt(double __x) { return sqrt(__x); }
#    undef sqrt
inline double sqrt(double __x) { return __stlp_sqrt(__x); }
#  endif
#  if defined (ldexp)
inline double __stlp_ldexp(double __x, int __y) { return ldexp(__x, __y); }
#    undef ldexp
inline double ldexp(double __x, int __y) { return __stlp_ldexp(__x, __y); }
#  endif
#endif

/* MSVC native lib starting with .Net 2003 has already all math functions
 * in global namespace.
 * HP-UX native lib has math functions in the global namespace.
 */
#if (!defined (_STLP_MSVC_LIB) || (_STLP_MSVC_LIB < 1310) || defined(UNDER_CE)) && \
    (!defined (__HP_aCC) || (__HP_aCC < 30000)) && \
    !defined (__WATCOMC__)
inline double abs(double __x)
{ return ::fabs(__x); }
#  if !defined (__MVS__)
_STLP_DEF_MATH_INLINE(abs, fabs)
#  else // __MVS__ has native long double abs?
inline float abs(float __x) { return ::fabsf(__x); }
#  endif

_STLP_DEF_MATH_INLINE(acos, acos)
_STLP_DEF_MATH_INLINE(asin, asin)
_STLP_DEF_MATH_INLINE(atan, atan)
_STLP_DEF_MATH_INLINE2(atan2, atan2)
_STLP_DEF_MATH_INLINE(ceil, ceil)
_STLP_DEF_MATH_INLINE(cos, cos)
_STLP_DEF_MATH_INLINE(cosh, cosh)
_STLP_DEF_MATH_INLINE(exp, exp)
_STLP_DEF_MATH_INLINE(fabs, fabs)
_STLP_DEF_MATH_INLINE(floor, floor)
_STLP_DEF_MATH_INLINE2(fmod, fmod)
_STLP_DEF_MATH_INLINE2PI(frexp, frexp)
_STLP_DEF_MATH_INLINE2I(ldexp, ldexp)
_STLP_DEF_MATH_INLINE(log, log)
_STLP_DEF_MATH_INLINE(log10, log10)
_STLP_DEF_MATH_INLINE2P(modf, modf)
_STLP_DEF_MATH_INLINE(sin, sin)
_STLP_DEF_MATH_INLINE(sinh, sinh)
_STLP_DEF_MATH_INLINE(sqrt, sqrt)
_STLP_DEF_MATH_INLINE(tan, tan)
_STLP_DEF_MATH_INLINE(tanh, tanh)
_STLP_DEF_MATH_INLINE2(pow, pow)

#  if !defined(_STLP_MSVC) /* || (_STLP_MSVC > 1300) */ || defined(_STLP_WCE) || !defined (_MSC_EXTENSIONS) /* && !defined(_STLP_WCE_NET) */
#    ifndef _STLP_NO_VENDOR_MATH_F
#      ifndef __sun
inline float pow(float __x, int __y) { return _STLP_CMATH_FUNC_NAMESPACE::powf(__x, __STATIC_CAST(float,__y)); }
#      else
inline float pow(float __x, int __y) { return ::__powf(__x, __STATIC_CAST(float,__y)); }
#      endif
#    else
inline float pow(float __x, int __y) { return __STATIC_CAST(float, _STLP_CMATH_FUNC_NAMESPACE::pow(__x, __STATIC_CAST(float,__y))); }
#    endif
inline double pow(double __x, int __y) { return _STLP_CMATH_FUNC_NAMESPACE::pow(__x, __STATIC_CAST(double,__y)); }
#    if !defined (_STLP_NO_LONG_DOUBLE)
#      if !defined(_STLP_NO_VENDOR_MATH_L)
#        ifndef __sun
inline long double pow(long double __x, int __y) { return _STLP_CMATH_FUNC_NAMESPACE::powl(__x, __STATIC_CAST(long double,__y)); }
#        else
#          ifndef __SUNPRO_CC
inline long double pow(long double __x, int __y) { return ::__powl(__x, __STATIC_CAST(long double,__y)); }
#          else
inline long double pow(long double __x, int __y) { return _STLP_VENDOR_CSTD::__powl(__x, __STATIC_CAST(long double,__y)); }
#          endif
#        endif
#      else
inline long double pow(long double __x, int __y) { return __STATIC_CAST(long double, _STLP_CMATH_FUNC_NAMESPACE::pow(__x, __STATIC_CAST(long double,__y))); }
#      endif
#    endif
#  else
//The MS native pow version has a bugged overload so it is not imported
//in the STLport namespace.
//Here is the bugged version:
//inline double pow(int __x, int __y)            { return (_Pow_int(__x, __y)); }
inline double      pow(double __x, int __y)      { return (_Pow_int(__x, __y)); }
inline float       pow(float __x, int __y)       { return (_Pow_int(__x, __y)); }
inline long double pow(long double __x, int __y) { return (_Pow_int(__x, __y)); }
#  endif
#endif

#if (defined (_STLP_MSVC) && !defined (_STLP_WCE)) || defined (__ICL) || defined (__sun)
#  if defined (_STLP_MSVC) && (_STLP_MSVC >= 1400)
#    pragma warning (push)
#    pragma warning (disable : 4996) // hypot is deprecated.
#  endif
_STLP_MATH_INLINE2XX(float, hypot, hypot)
inline long double hypot(long double x, long double y) { return sqrt(x * x + y * y); }
#  if defined (_STLP_MSVC) && (_STLP_MSVC >= 1400)
#    pragma warning (pop)
#  endif
#else
#  if defined (_STLP_USE_UCLIBC)
inline double hypot(double x, double y) { return sqrt(x * x + y * y); }
_STLP_DEF_MATH_INLINE2(hypot, hypot)
#  elif defined (_STLP_WCE)
   /* CE has a double _hypot(double,double) which we use */
inline double hypot(double __x, double __y) { return _hypot(__x,__y); }
_STLP_DEF_MATH_INLINE2(hypot, _hypot)
#  endif
#endif

#if defined (_STLP_RESTORE_FUNCTION_INTRINSIC)
//restoration of the default intrinsic status of those functions:
#  pragma intrinsic (abs, acos, asin, atan, atan2, cos, cosh, exp, fabs, fmod, log, log10, sin, sinh, sqrt, tan, tanh)
#  if defined (_STLP_WCE)
#    pragma intrinsic (ceil, floor)
#  endif
#  pragma warning(pop)
#  undef _STLP_RESTORE_FUNCTION_INTRINSIC
#endif // _STLP_MSVC && _STLP_MSVC <= 1300 && !_STLP_WCE && _MSC_EXTENSIONS

/* C++ Standard is unclear about several call to 'using ::func' if new overloads
 * of ::func appears between 2 successive 'using' calls. To avoid this potential
 * problem we provide all abs overload before the 'using' call.
 * Beware: This header inclusion has to be after all abs overload of this file.
 *         The first 'using ::abs' call is going to be in the other header.
 */
#ifndef _STLP_INTERNAL_CSTDLIB
#  include <stl/_cstdlib.h>
#endif

#if defined (_STLP_IMPORT_VENDOR_CSTD) && !defined (_STLP_NO_CSTD_FUNCTION_IMPORTS)
_STLP_BEGIN_NAMESPACE
using ::abs;
using ::acos;
using ::asin;
using ::atan;
using ::atan2;
using ::ceil;
using ::cos;
using ::cosh;
using ::exp;
using ::fabs;
using ::floor;
using ::fmod;
using ::frexp;
/*
   Because of some weird interaction between STLport headers
   and native HP-UX headers, when compiled with _STLP_DEBUG
   macro defined with aC++, hypot() is not declared.
   At some point we'll need to get to the bottom line of
   this problem.
*/
#if !(defined(__HP_aCC) && defined(_STLP_DEBUG))
using ::hypot;
#endif
using ::ldexp;
using ::log;
using ::log10;
using ::modf;
using ::pow;
using ::sin;
using ::sinh;
using ::sqrt;
using ::tan;
using ::tanh;
_STLP_END_NAMESPACE
#  if defined (__BORLANDC__) && (__BORLANDC__ >= 0x560) && !defined (__linux__)
using _STLP_VENDOR_CSTD::_ecvt;
using _STLP_VENDOR_CSTD::_fcvt;
#  endif
#endif

#endif /* _STLP_INTERNAL_CMATH */

// Local Variables:
// mode:C++
// End:
