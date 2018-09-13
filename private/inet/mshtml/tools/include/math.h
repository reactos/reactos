/***
*math.h - definitions and declarations for math library
*
*       Copyright (c) 1985-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains constant definitions and external subroutine
*       declarations for the math subroutine library.
*       [ANSI/System V]
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_MATH
#define _INC_MATH

#if !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif


#ifdef  _MSC_VER
/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,8)
#endif  /* _MSC_VER */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __assembler /* Protect from assembler */

/* Define _CRTAPI1 (for compatibility with the NT SDK) */

#ifndef _CRTAPI1
#if     _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI1 __cdecl
#else
#define _CRTAPI1
#endif
#endif


/* Define _CRTAPI2 (for compatibility with the NT SDK) */

#ifndef _CRTAPI2
#if     _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI2 __cdecl
#else
#define _CRTAPI2
#endif
#endif


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef  _NTSDK
/* definition compatible with NT SDK */
#define _CRTIMP
#else   /* ndef _NTSDK */
/* current definition */
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* _NTSDK */
#endif  /* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if     ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


/* Definition of _exception struct - this struct is passed to the matherr
 * routine when a floating point exception is detected
 */

#ifndef _EXCEPTION_DEFINED
struct _exception {
        int type;       /* exception type - see below */
        char *name;     /* name of function where error occured */
        double arg1;    /* first argument to function */
        double arg2;    /* second argument (if any) to function */
        double retval;  /* value to be returned by function */
        } ;

#define _EXCEPTION_DEFINED
#endif


/* Definition of a _complex struct to be used by those who use cabs and
 * want type checking on their argument
 */

#ifndef _COMPLEX_DEFINED
struct _complex {
        double x,y; /* real and imaginary parts */
        } ;

#if     !__STDC__ && !defined (__cplusplus)
/* Non-ANSI name for compatibility */
#define complex _complex
#endif

#define _COMPLEX_DEFINED
#endif
#endif  /* __assembler */


/* Constant definitions for the exception type passed in the _exception struct
 */

#define _DOMAIN     1   /* argument domain error */
#define _SING       2   /* argument singularity */
#define _OVERFLOW   3   /* overflow range error */
#define _UNDERFLOW  4   /* underflow range error */
#define _TLOSS      5   /* total loss of precision */
#define _PLOSS      6   /* partial loss of precision */

#define EDOM        33
#define ERANGE      34


/* Definitions of _HUGE and HUGE_VAL - respectively the XENIX and ANSI names
 * for a value returned in case of error by a number of the floating point
 * math routines
 */
#ifndef __assembler /* Protect from assembler */
#ifdef  _NTSDK
/* definition compatible with NT SDK */
#ifdef  _DLL
#define _HUGE   (*_HUGE_dll)
extern double * _HUGE_dll;
#else   /* ndef _DLL */
extern double _HUGE;
#endif  /* _DLL */
#else   /* ndef _NTSDK */
/* current definition */
_CRTIMP extern double _HUGE;
#endif  /* _NTSDK */
#endif  /* __assembler */

#define HUGE_VAL _HUGE


/* Function prototypes */

#if !defined(__assembler)   /* Protect from assembler */
#if _M_MRX000
_CRTIMP int     __cdecl abs(int);
_CRTIMP double  __cdecl acos(double);
_CRTIMP double  __cdecl asin(double);
_CRTIMP double  __cdecl atan(double);
_CRTIMP double  __cdecl atan2(double, double);
_CRTIMP double  __cdecl cos(double);
_CRTIMP double  __cdecl cosh(double);
_CRTIMP double  __cdecl exp(double);
_CRTIMP double  __cdecl fabs(double);
_CRTIMP double  __cdecl fmod(double, double);
_CRTIMP long    __cdecl labs(long);
_CRTIMP double  __cdecl log(double);
_CRTIMP double  __cdecl log10(double);
_CRTIMP double  __cdecl pow(double, double);
_CRTIMP double  __cdecl sin(double);
_CRTIMP double  __cdecl sinh(double);
_CRTIMP double  __cdecl tan(double);
_CRTIMP double  __cdecl tanh(double);
_CRTIMP double  __cdecl sqrt(double);
#else
        int     __cdecl abs(int);
        double  __cdecl acos(double);
        double  __cdecl asin(double);
        double  __cdecl atan(double);
        double  __cdecl atan2(double, double);
        double  __cdecl cos(double);
        double  __cdecl cosh(double);
        double  __cdecl exp(double);
        double  __cdecl fabs(double);
        double  __cdecl fmod(double, double);
        long    __cdecl labs(long);
        double  __cdecl log(double);
        double  __cdecl log10(double);
        double  __cdecl pow(double, double);
        double  __cdecl sin(double);
        double  __cdecl sinh(double);
        double  __cdecl tan(double);
        double  __cdecl tanh(double);
        double  __cdecl sqrt(double);
#endif
_CRTIMP double  __cdecl atof(const char *);
_CRTIMP double  __cdecl _cabs(struct _complex);
_CRTIMP double  __cdecl ceil(double);
_CRTIMP double  __cdecl floor(double);
_CRTIMP double  __cdecl frexp(double, int *);
_CRTIMP double  __cdecl _hypot(double, double);
_CRTIMP double  __cdecl _j0(double);
_CRTIMP double  __cdecl _j1(double);
_CRTIMP double  __cdecl _jn(int, double);
_CRTIMP double  __cdecl ldexp(double, int);
        int     __cdecl _matherr(struct _exception *);
_CRTIMP double  __cdecl modf(double, double *);

_CRTIMP double  __cdecl _y0(double);
_CRTIMP double  __cdecl _y1(double);
_CRTIMP double  __cdecl _yn(int, double);


#ifdef _M_MRX000

/* MIPS fast prototypes for float */
/* ANSI C, 4.5 Mathematics        */

/* 4.5.2 Trigonometric functions */

_CRTIMP float  __cdecl acosf( float );
_CRTIMP float  __cdecl asinf( float );
_CRTIMP float  __cdecl atanf( float );
_CRTIMP float  __cdecl atan2f( float , float );
_CRTIMP float  __cdecl cosf( float );
_CRTIMP float  __cdecl sinf( float );
_CRTIMP float  __cdecl tanf( float );

/* 4.5.3 Hyperbolic functions */
_CRTIMP float  __cdecl coshf( float );
_CRTIMP float  __cdecl sinhf( float );
_CRTIMP float  __cdecl tanhf( float );

/* 4.5.4 Exponential and logarithmic functions */
_CRTIMP float  __cdecl expf( float );
_CRTIMP float  __cdecl logf( float );
_CRTIMP float  __cdecl log10f( float );
_CRTIMP float  __cdecl modff( float , float* );

/* 4.5.5 Power functions */
_CRTIMP float  __cdecl powf( float , float );
        float  __cdecl sqrtf( float );

/* 4.5.6 Nearest integer, absolute value, and remainder functions */
        float  __cdecl ceilf( float );
        float  __cdecl fabsf( float );
        float  __cdecl floorf( float );
_CRTIMP float  __cdecl fmodf( float , float );

_CRTIMP float  __cdecl hypotf(float, float);

#endif /* _M_MRX000 */

#if !defined(_M_M68K)
/* Macros defining long double functions to be their double counterparts
 * (long double is synonymous with double in this implementation).
 */


#ifndef __cplusplus
#define acosl(x)    ((long double)acos((double)(x)))
#define asinl(x)    ((long double)asin((double)(x)))
#define atanl(x)    ((long double)atan((double)(x)))
#define atan2l(x,y) ((long double)atan2((double)(x), (double)(y)))
#define _cabsl      _cabs
#define ceill(x)    ((long double)ceil((double)(x)))
#define cosl(x)     ((long double)cos((double)(x)))
#define coshl(x)    ((long double)cosh((double)(x)))
#define expl(x)     ((long double)exp((double)(x)))
#define fabsl(x)    ((long double)fabs((double)(x)))
#define floorl(x)   ((long double)floor((double)(x)))
#define fmodl(x,y)  ((long double)fmod((double)(x), (double)(y)))
#define frexpl(x,y) ((long double)frexp((double)(x), (y)))
#define _hypotl(x,y)    ((long double)_hypot((double)(x), (double)(y)))
#define ldexpl(x,y) ((long double)ldexp((double)(x), (y)))
#define logl(x)     ((long double)log((double)(x)))
#define log10l(x)   ((long double)log10((double)(x)))
#define _matherrl   _matherr
#define modfl(x,y)  ((long double)modf((double)(x), (double *)(y)))
#define powl(x,y)   ((long double)pow((double)(x), (double)(y)))
#define sinl(x)     ((long double)sin((double)(x)))
#define sinhl(x)    ((long double)sinh((double)(x)))
#define sqrtl(x)    ((long double)sqrt((double)(x)))
#define tanl(x)     ((long double)tan((double)(x)))
#define tanhl(x)    ((long double)tanh((double)(x)))
#else	/* __cplusplus */
inline long double acosl(long double _X)
	{return (acos((double)_X)); }
inline long double asinl(long double _X)
	{return (asin((double)_X)); }
inline long double atanl(long double _X)
	{return (atan((double)_X)); }
inline long double atan2l(long double _X, long double _Y)
	{return (atan2((double)_X, (double)_Y)); }
inline long double ceill(long double _X)
	{return (ceil((double)_X)); }
inline long double cosl(long double _X)
	{return (cos((double)_X)); }
inline long double coshl(long double _X)
	{return (cosh((double)_X)); }
inline long double expl(long double _X)
	{return (exp((double)_X)); }
inline long double fabsl(long double _X)
	{return (fabs((double)_X)); }
inline long double floorl(long double _X)
	{return (floor((double)_X)); }
inline long double fmodl(long double _X, long double _Y)
	{return (fmod((double)_X, (double)_Y)); }
inline long double frexpl(long double _X, int *_Y)
	{return (frexp((double)_X, _Y)); }
inline long double ldexpl(long double _X, int _Y)
	{return (ldexp((double)_X, _Y)); }
inline long double logl(long double _X)
	{return (log((double)_X)); }
inline long double log10l(long double _X)
	{return (log10((double)_X)); }
inline long double modfl(long double _X, long double *_Y)
	{double _Di, _Df = modf((double)_X, &_Di);
	*_Y = (long double)_Di;
	return (_Df); }
inline long double powl(long double _X, long double _Y)
	{return (pow((double)_X, (double)_Y)); }
inline long double sinl(long double _X)
	{return (sin((double)_X)); }
inline long double sinhl(long double _X)
	{return (sinh((double)_X)); }
inline long double sqrtl(long double _X)
	{return (sqrt((double)_X)); }
inline long double tanl(long double _X)
	{return (tan((double)_X)); }
inline long double tanhl(long double _X)
	{return (tanh((double)_X)); }

inline float frexpf(float _X, int *_Y)
	{return ((float)frexp((double)_X, _Y)); }
inline float ldexpf(float _X, int _Y)
	{return ((float)ldexp((double)_X, _Y)); }
#if !defined(_M_MRX000)
inline float acosf(float _X)
	{return ((float)acos((double)_X)); }
inline float asinf(float _X)
	{return ((float)asin((double)_X)); }
inline float atanf(float _X)
	{return ((float)atan((double)_X)); }
inline float atan2f(float _X, float _Y)
	{return ((float)atan2((double)_X, (double)_Y)); }
inline float ceilf(float _X)
	{return ((float)ceil((double)_X)); }
inline float cosf(float _X)
	{return ((float)cos((double)_X)); }
inline float coshf(float _X)
	{return ((float)cosh((double)_X)); }
inline float expf(float _X)
	{return ((float)exp((double)_X)); }
inline float fabsf(float _X)
	{return ((float)fabs((double)_X)); }
inline float floorf(float _X)
	{return ((float)floor((double)_X)); }
inline float fmodf(float _X, float _Y)
	{return ((float)fmod((double)_X, (double)_Y)); }
inline float logf(float _X)
	{return ((float)log((double)_X)); }
inline float log10f(float _X)
	{return ((float)log10((double)_X)); }
inline float modff(float _X, float *_Y)
	{ double _Di, _Df = modf((double)_X, &_Di);
	*_Y = (float)_Di;
	return ((float)_Df); }
inline float powf(float _X, float _Y)
	{return ((float)pow((double)_X, (double)_Y)); }
inline float sinf(float _X)
	{return ((float)sin((double)_X)); }
inline float sinhf(float _X)
	{return ((float)sinh((double)_X)); }
inline float sqrtf(float _X)
	{return ((float)sqrt((double)_X)); }
inline float tanf(float _X)
	{return ((float)tan((double)_X)); }
inline float tanhf(float _X)
	{return ((float)tanh((double)_X)); }
#endif  /* !defined(_M_MRX000) */
#endif	/* __cplusplus */
#endif  /* _M_M68K */
#endif  /* __assembler */

#if     !__STDC__

/* Non-ANSI names for compatibility */

#define DOMAIN      _DOMAIN
#define SING        _SING
#define OVERFLOW    _OVERFLOW
#define UNDERFLOW   _UNDERFLOW
#define TLOSS       _TLOSS
#define PLOSS       _PLOSS

#if !defined(_M_MPPC) && !defined(_M_M68K)
#define matherr     _matherr
#endif /* !defined(_M_MPPC) && !defined(_M_M68K) */

#ifndef __assembler /* Protect from assembler */

#ifdef  _NTSDK

/* Definitions and declarations compatible with NT SDK */

#ifdef  _DLL
#define HUGE    (*HUGE_dll)
extern double * HUGE_dll;
#else   /* ndef _DLL */
extern double HUGE;
#endif  /* _DLL */

#define cabs    _cabs
#define hypot   _hypot
#define j0      _j0
#define j1      _j1
#define jn      _jn
#define y0      _y0
#define y1      _y1
#define yn      _yn

#else   /* ndef _NTSDK */

/* Current definitions and declarations */

_CRTIMP extern double HUGE;

_CRTIMP double  __cdecl cabs(struct _complex);
_CRTIMP double  __cdecl hypot(double, double);
_CRTIMP double  __cdecl j0(double);
_CRTIMP double  __cdecl j1(double);
_CRTIMP double  __cdecl jn(int, double);
        int     __cdecl matherr(struct _exception *);
_CRTIMP double  __cdecl y0(double);
_CRTIMP double  __cdecl y1(double);
_CRTIMP double  __cdecl yn(int, double);

#endif  /* _NTSDK */
#endif  /* __assembler */

#endif  /* __STDC__ */

#ifdef _M_M68K
/* definition of _exceptionl struct - this struct is passed to the _matherrl
 * routine when a floating point exception is detected in a long double routine
 */

#ifndef _LD_EXCEPTION_DEFINED

struct _exceptionl {
        int type;           /* exception type - see below */
        char *name;         /* name of function where error occured */
        long double arg1;   /* first argument to function */
        long double arg2;   /* second argument (if any) to function */
        long double retval; /* value to be returned by function */
} ;
#define _LD_EXCEPTION_DEFINED
#endif


/* definition of a _complexl struct to be used by those who use _cabsl and
 * want type checking on their argument
 */

#ifndef _LD_COMPLEX_DEFINED
struct _complexl {
        long double x,y;    /* real and imaginary parts */
} ;
#define _LD_COMPLEX_DEFINED
#endif


long double  __cdecl acosl(long double);
long double  __cdecl asinl(long double);
long double  __cdecl atanl(long double);
long double  __cdecl atan2l(long double, long double);
long double  __cdecl _atold(const char  *);
long double  __cdecl _cabsl(struct _complexl);
long double  __cdecl ceill(long double);
long double  __cdecl cosl(long double);
long double  __cdecl coshl(long double);
long double  __cdecl expl(long double);
long double  __cdecl fabsl(long double);
long double  __cdecl floorl(long double);
long double  __cdecl fmodl(long double, long double);
long double  __cdecl frexpl(long double, int  *);
long double  __cdecl _hypotl(long double, long double);
long double  __cdecl _j0l(long double);
long double  __cdecl _j1l(long double);
long double  __cdecl _jnl(int, long double);
long double  __cdecl ldexpl(long double, int);
long double  __cdecl logl(long double);
long double  __cdecl log10l(long double);
int          __cdecl _matherrl(struct _exceptionl  *);
long double  __cdecl modfl(long double, long double  *);
long double  __cdecl powl(long double, long double);
long double  __cdecl sinl(long double);
long double  __cdecl sinhl(long double);
long double  __cdecl sqrtl(long double);
long double  __cdecl tanl(long double);
long double  __cdecl tanhl(long double);
long double  __cdecl _y0l(long double);
long double  __cdecl _y1l(long double);
long double  __cdecl _ynl(int, long double);

#endif  /* _M_M68K */


#ifdef __cplusplus
}

#if !defined(_M_M68K)

template<class _TYPE> inline
	_TYPE _Pow_int(_TYPE _X, int _Y)
	{int _N = _Y;
	if (_Y < 0)
		_N = -_N;
	for (_TYPE _Z = _TYPE(1); ; _X *= _X)
		{if ((_N & 1) != 0)
			_Z *= _X;
		if ((_N >>= 1) == 0)
			return (_Y < 0 ? _TYPE(1) / _Z : _Z); }}
inline double abs(double _X)
	{return (fabs(_X)); }
inline double pow(double _X, int _Y)
	{return (_Pow_int(_X, _Y)); }
inline double pow(int _X, int _Y)
	{return (_Pow_int(_X, _Y)); }
inline float abs(float _X)
	{return (fabsf(_X)); }
inline float acos(float _X)
	{return (acosf(_X)); }
inline float asin(float _X)
	{return (asinf(_X)); }
inline float atan(float _X)
	{return (atanf(_X)); }
inline float atan2(float _Y, float _X)
	{return (atan2f(_Y, _X)); }
inline float ceil(float _X)
	{return (ceilf(_X)); }
inline float cos(float _X)
	{return (cosf(_X)); }
inline float cosh(float _X)
	{return (coshf(_X)); }
inline float exp(float _X)
	{return (expf(_X)); }
inline float fabs(float _X)
	{return (fabsf(_X)); }
inline float floor(float _X)
	{return (floorf(_X)); }
inline float fmod(float _X, float _Y)
	{return (fmodf(_X, _Y)); }
inline float frexp(float _X, int * _Y)
	{return (frexpf(_X, _Y)); }
inline float ldexp(float _X, int _Y)
	{return (ldexpf(_X, _Y)); }
inline float log(float _X)
	{return (logf(_X)); }
inline float log10(float _X)
	{return (log10f(_X)); }
inline float modf(float _X, float * _Y)
	{return (modff(_X, _Y)); }
inline float pow(float _X, float _Y)
	{return (powf(_X, _Y)); }
inline float pow(float _X, int _Y)
	{return (_Pow_int(_X, _Y)); }
inline float sin(float _X)
	{return (sinf(_X)); }
inline float sinh(float _X)
	{return (sinhf(_X)); }
inline float sqrt(float _X)
	{return (sqrtf(_X)); }
inline float tan(float _X)
	{return (tanf(_X)); }
inline float tanh(float _X)
	{return (tanhf(_X)); }
inline long double abs(long double _X)
	{return (fabsl(_X)); }
inline long double acos(long double _X)
	{return (acosl(_X)); }
inline long double asin(long double _X)
	{return (asinl(_X)); }
inline long double atan(long double _X)
	{return (atanl(_X)); }
inline long double atan2(long double _Y, long double _X)
	{return (atan2l(_Y, _X)); }
inline long double ceil(long double _X)
	{return (ceill(_X)); }
inline long double cos(long double _X)
	{return (cosl(_X)); }
inline long double cosh(long double _X)
	{return (coshl(_X)); }
inline long double exp(long double _X)
	{return (expl(_X)); }
inline long double fabs(long double _X)
	{return (fabsl(_X)); }
inline long double floor(long double _X)
	{return (floorl(_X)); }
inline long double fmod(long double _X, long double _Y)
	{return (fmodl(_X, _Y)); }
inline long double frexp(long double _X, int * _Y)
	{return (frexpl(_X, _Y)); }
inline long double ldexp(long double _X, int _Y)
	{return (ldexpl(_X, _Y)); }
inline long double log(long double _X)
	{return (logl(_X)); }
inline long double log10(long double _X)
	{return (log10l(_X)); }
inline long double modf(long double _X, long double * _Y)
	{return (modfl(_X, _Y)); }
inline long double pow(long double _X, long double _Y)
	{return (powl(_X, _Y)); }
inline long double pow(long double _X, int _Y)
	{return (_Pow_int(_X, _Y)); }
inline long double sin(long double _X)
	{return (sinl(_X)); }
inline long double sinh(long double _X)
	{return (sinhl(_X)); }
inline long double sqrt(long double _X)
	{return (sqrtl(_X)); }
inline long double tan(long double _X)
	{return (tanl(_X)); }
inline long double tanh(long double _X)
	{return (tanhl(_X)); }

#endif	/* _M_M68K */ 
#endif	/* __cplusplus */

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif  /* _INC_MATH */

