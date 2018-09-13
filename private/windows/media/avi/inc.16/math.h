/***
*math.h - definitions and declarations for math library
*
*   Copyright (c) 1985-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*   This file contains constant definitions and external subroutine
*   declarations for the math subroutine library.
*   [ANSI/System V]
*
****/

#ifndef _INC_MATH

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#define __near      _near
#define __pascal    _pascal
#endif 

/* definition of _exception struct - this struct is passed to the _matherr
 * routine when a floating point exception is detected
 */

#ifndef _EXCEPTION_DEFINED
#pragma pack(2)

struct _exception {
    int type;       /* exception type - see below */
    char *name;   /* name of function where error occured */
    double arg1;        /* first argument to function */
    double arg2;        /* second argument (if any) to function */
    double retval;      /* value to be returned by function */
    } ;

#ifndef __STDC__
/* Non-ANSI name for compatibility */
#define exception _exception
#endif 

#pragma pack()
#define _EXCEPTION_DEFINED
#endif 


/* definition of a _complex struct to be used by those who use cabs and
 * want type checking on their argument
 */

#ifndef _COMPLEX_DEFINED

struct _complex {
    double x,y; /* real and imaginary parts */
    } ;

#ifndef __cplusplus
#ifndef __STDC__
/* Non-ANSI name for compatibility */
struct complex {
    double x,y; /* real and imaginary parts */
    } ;
#endif 
#endif 

#define _COMPLEX_DEFINED
#endif 


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


/* definitions of _HUGE (XENIX) and HUGE_VAL (ANSI) error return values used
 * by several floating point math routines
 */

extern double __near __cdecl _HUGE;
#define HUGE_VAL _HUGE


/* function prototypes */

#ifdef _MT
int __cdecl  abs(int);
double  __pascal acos(double);
double  __pascal asin(double);
double  __pascal atan(double);
double  __pascal atan2(double, double);
double  __pascal atof(const char *);
double  __pascal _cabs(struct _complex);
double  __pascal ceil(double);
double  __pascal cos(double);
double  __pascal cosh(double);
int __cdecl  _dieeetomsbin(double *, double *);
int __cdecl  _dmsbintoieee(double *, double *);
double  __pascal exp(double);
double  __pascal fabs(double);
int __cdecl  _fieeetomsbin(float *, float *);
double  __pascal floor(double);
double  __pascal fmod(double, double);
int __cdecl  _fmsbintoieee(float *, float *);
double  __pascal frexp(double, int *);
double  __pascal _hypot(double, double);
double  __pascal _j0(double);
double  __pascal _j1(double);
double  __pascal _jn(int, double);
long    __cdecl  labs(long);
double  __pascal ldexp(double, int);
double  __pascal log(double);
double  __pascal log10(double);
int __cdecl  _matherr(struct _exception *);
double  __pascal modf(double, double *);
double  __pascal pow(double, double);
double  __pascal sin(double);
double  __pascal sinh(double);
double  __pascal sqrt(double);
double  __pascal tan(double);
double  __pascal tanh(double);
double  __pascal _y0(double);
double  __pascal _y1(double);
double  __pascal _yn(int, double);

#else 
int __cdecl abs(int);
double  __cdecl acos(double);
double  __cdecl asin(double);
double  __cdecl atan(double);
double  __cdecl atan2(double, double);
double  __cdecl atof(const char *);
double  __cdecl _cabs(struct _complex);
double  __cdecl ceil(double);
double  __cdecl cos(double);
double  __cdecl cosh(double);
int __cdecl _dieeetomsbin(double *, double *);
int __cdecl _dmsbintoieee(double *, double *);
double  __cdecl exp(double);
double  __cdecl fabs(double);
int __cdecl _fieeetomsbin(float *, float *);
double  __cdecl floor(double);
double  __cdecl fmod(double, double);
int __cdecl _fmsbintoieee(float *, float *);
double  __cdecl frexp(double, int *);
double  __cdecl _hypot(double, double);
double  __cdecl _j0(double);
double  __cdecl _j1(double);
double  __cdecl _jn(int, double);
long    __cdecl labs(long);
double  __cdecl ldexp(double, int);
double  __cdecl log(double);
double  __cdecl log10(double);
int __cdecl _matherr(struct _exception *);
double  __cdecl modf(double, double *);
double  __cdecl pow(double, double);
double  __cdecl sin(double);
double  __cdecl sinh(double);
double  __cdecl sqrt(double);
double  __cdecl tan(double);
double  __cdecl tanh(double);
double  __cdecl _y0(double);
double  __cdecl _y1(double);
double  __cdecl _yn(int, double);
#endif 


/* definition of _exceptionl struct - this struct is passed to the _matherrl
 * routine when a floating point exception is detected in a long double routine
 */

#ifndef _LD_EXCEPTION_DEFINED
#pragma pack(2)
struct _exceptionl {
    int type;       /* exception type - see below */
    char *name;   /* name of function where error occured */
    long double arg1;   /* first argument to function */
    long double arg2;   /* second argument (if any) to function */
    long double retval; /* value to be returned by function */
    } ;
#pragma pack()
#define _LD_EXCEPTION_DEFINED
#endif 


/* definition of a _complexl struct to be used by those who use _cabsl and
 * want type checking on their argument
 */

#ifndef _LD_COMPLEX_DEFINED
#pragma pack(2)
struct _complexl {
    long double x,y;    /* real and imaginary parts */
    } ;
#pragma pack()
#define _LD_COMPLEX_DEFINED
#endif 

extern long double __near __cdecl _LHUGE;
#define _LHUGE_VAL _LHUGE


long double  __cdecl acosl(long double);
long double  __cdecl asinl(long double);
long double  __cdecl atanl(long double);
long double  __cdecl atan2l(long double, long double);
long double  __cdecl _atold(const char *);
long double  __cdecl _cabsl(struct _complexl);
long double  __cdecl ceill(long double);
long double  __cdecl cosl(long double);
long double  __cdecl coshl(long double);
long double  __cdecl expl(long double);
long double  __cdecl fabsl(long double);
long double  __cdecl floorl(long double);
long double  __cdecl fmodl(long double, long double);
long double  __cdecl frexpl(long double, int *);
long double  __cdecl _hypotl(long double, long double);
long double  __cdecl _j0l(long double);
long double  __cdecl _j1l(long double);
long double  __cdecl _jnl(int, long double);
long double  __cdecl ldexpl(long double, int);
long double  __cdecl logl(long double);
long double  __cdecl log10l(long double);
int      __cdecl _matherrl(struct _exceptionl *);
long double  __cdecl modfl(long double, long double *);
long double  __cdecl powl(long double, long double);
long double  __cdecl sinl(long double);
long double  __cdecl sinhl(long double);
long double  __cdecl sqrtl(long double);
long double  __cdecl tanl(long double);
long double  __cdecl tanhl(long double);
long double  __cdecl _y0l(long double);
long double  __cdecl _y1l(long double);
long double  __cdecl _ynl(int, long double);


#ifndef __STDC__
/* Non-ANSI names for compatibility */

#define DOMAIN      _DOMAIN
#define SING        _SING
#define OVERFLOW    _OVERFLOW
#define UNDERFLOW   _UNDERFLOW
#define TLOSS       _TLOSS
#define PLOSS       _PLOSS

#define matherr _matherr

extern double __near __cdecl HUGE;

#ifdef _MT
#ifndef __cplusplus
double  __pascal cabs(struct complex);
#endif 
double  __pascal hypot(double, double);
double  __pascal j0(double);
double  __pascal j1(double);
double  __pascal jn(int, double);
double  __pascal y0(double);
double  __pascal y1(double);
double  __pascal yn(int, double);
#else 
#ifndef __cplusplus
double  __cdecl cabs(struct complex);
#endif 
double  __cdecl hypot(double, double);
double  __cdecl j0(double);
double  __cdecl j1(double);
double  __cdecl jn(int, double);
double  __cdecl y0(double);
double  __cdecl y1(double);
double  __cdecl yn(int, double);
#endif 

int __cdecl dieeetomsbin(double *, double *);
int __cdecl dmsbintoieee(double *, double *);
int __cdecl fieeetomsbin(float *, float *);
int __cdecl fmsbintoieee(float *, float *);

long double  __cdecl cabsl(struct _complexl);
long double  __cdecl hypotl(long double, long double);

#endif 


#ifdef __cplusplus
}
#endif 

#define _INC_MATH
#endif 
