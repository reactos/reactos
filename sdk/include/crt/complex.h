/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
/*
 * complex.h
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Danny Smith <dannysmith@users.sourceforge.net>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _COMPLEX_H_
#define _COMPLEX_H_

#include <corecrt.h>

#if (defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
     || !defined __STRICT_ANSI__

/* These macros are specified by C99 standard */

#ifndef __cplusplus
#define complex _Complex
#endif

#define _Complex_I  (0.0F +  1.0iF)

/* GCC doesn't support _Imaginary type yet, so we don't
   define _Imaginary_I */

#define I _Complex_I

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RC_INVOKED

double creal (double _Complex);
double cimag (double _Complex);
double carg (double _Complex);
double cabs (double _Complex);
double _Complex conj (double _Complex);
double _Complex  cacos (double _Complex);
double _Complex  casin (double _Complex);
double _Complex  catan (double _Complex);
double _Complex  ccos (double _Complex);
double _Complex  csin (double _Complex);
double _Complex  ctan (double _Complex);
double _Complex  cacosh (double _Complex);
double _Complex  casinh (double _Complex);
double _Complex  catanh (double _Complex);
double _Complex  ccosh (double _Complex);
double _Complex  csinh (double _Complex);
double _Complex  ctanh (double _Complex);
double _Complex  cexp (double _Complex);
double _Complex  clog (double _Complex);
double _Complex  cpow (double _Complex, double _Complex);
double _Complex  csqrt (double _Complex);
double _Complex cproj (double _Complex);

float crealf (float _Complex);
float cimagf (float _Complex);
float cargf (float _Complex);
float cabsf (float _Complex);
float _Complex conjf (float _Complex);
float _Complex  cacosf (float _Complex);
float _Complex  casinf (float _Complex);
float _Complex  catanf (float _Complex);
float _Complex  ccosf (float _Complex);
float _Complex  csinf (float _Complex);
float _Complex  ctanf (float _Complex);
float _Complex  cacoshf (float _Complex);
float _Complex  casinhf (float _Complex);
float _Complex  catanhf (float _Complex);
float _Complex  ccoshf (float _Complex);
float _Complex  csinhf (float _Complex);
float _Complex  ctanhf (float _Complex);
float _Complex  cexpf (float _Complex);
float _Complex  clogf (float _Complex);
float _Complex  cpowf (float _Complex, float _Complex);
float _Complex  csqrtf (float _Complex);
float _Complex cprojf (float _Complex);

long double creall (long double _Complex);
long double cimagl (long double _Complex);
long double cargl (long double _Complex);
long double cabsl (long double _Complex);
long double _Complex conjl (long double _Complex);
long double _Complex  cacosl (long double _Complex);
long double _Complex  casinl (long double _Complex);
long double _Complex  catanl (long double _Complex);
long double _Complex  ccosl (long double _Complex);
long double _Complex  csinl (long double _Complex);
long double _Complex  ctanl (long double _Complex);
long double _Complex  cacoshl (long double _Complex);
long double _Complex  casinhl (long double _Complex);
long double _Complex  catanhl (long double _Complex);
long double _Complex  ccoshl (long double _Complex);
long double _Complex  csinhl (long double _Complex);
long double _Complex  ctanhl (long double _Complex);
long double _Complex  cexpl (long double _Complex);
long double _Complex  clogl (long double _Complex);
long double _Complex  cpowl (long double _Complex, long double _Complex);
long double _Complex  csqrtl (long double _Complex);
long double _Complex cprojl (long double _Complex);

#ifdef __GNUC__

/* double */
__CRT_INLINE double creal (double _Complex _Z)
{
  return __real__ _Z;
}

__CRT_INLINE double cimag (double _Complex _Z)
{
  return __imag__ _Z;
}

__CRT_INLINE double _Complex conj (double _Complex _Z)
{
  return __extension__ ~_Z;
}

__CRT_INLINE  double carg (double _Complex _Z)
{
  double res;
  __asm__  ("fpatan;"
	   : "=t" (res) : "0" (__real__ _Z), "u" (__imag__ _Z) : "st(1)");
  return res;
}


/* float */
__CRT_INLINE float crealf (float _Complex _Z)
{
  return __real__ _Z;
}

__CRT_INLINE float cimagf (float _Complex _Z)
{
  return __imag__ _Z;
}

__CRT_INLINE float _Complex conjf (float _Complex _Z)
{
  return __extension__ ~_Z;
}

__CRT_INLINE  float cargf (float _Complex _Z)
{
  float res;
  __asm__  ("fpatan;"
	   : "=t" (res) : "0" (__real__ _Z), "u" (__imag__ _Z) : "st(1)");
  return res;
}

/* long double */
__CRT_INLINE long double creall (long double _Complex _Z)
{
  return __real__ _Z;
}

__CRT_INLINE long double cimagl (long double _Complex _Z)
{
  return __imag__ _Z;
}

__CRT_INLINE long double _Complex conjl (long double _Complex _Z)
{
  return __extension__ ~_Z;
}

__CRT_INLINE  long double cargl (long double _Complex _Z)
{
  long double res;
  __asm__  ("fpatan;"
	   : "=t" (res) : "0" (__real__ _Z), "u" (__imag__ _Z) : "st(1)");
  return res;
}

#endif /* __GNUC__ */


#endif /* RC_INVOKED */

#ifdef __cplusplus
}
#endif

#endif /* __STDC_VERSION__ >= 199901L */


#endif /* _COMPLEX_H */
