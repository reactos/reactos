/* 
 * math.h
 *
 * Mathematical functions.
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.1 $
 * $Author: ekohl $
 * $Date: 2001/07/02 21:52:25 $
 *
 */
// added modfl 

#ifndef _MATH_H_
#define _MATH_H_

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
#if __MSVCRT__
extern double*	__imp__HUGE;
#define	HUGE_VAL	(*__imp__HUGE)
#else
/* CRTDLL */
extern double*	_HUGE_dll;
#define	HUGE_VAL	(*_HUGE_dll)
#endif


struct _exception
{
	int	type;
	char	*name;
	double	arg1;
	double	arg2;
	double	retval;
};

/*
 * Types for the above _exception structure.
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


double	sin (double x);
double	cos (double x);
double	tan (double x);
double	sinh (double x);
double	cosh (double x);
double	tanh (double x);
double	asin (double x);
double	acos (double x);
double	atan (double x);
double	atan2 (double y, double x);
double	exp (double x);
double	log (double x);
double	log10 (double x);
double	pow (double x, double y);
long double	powl (long double x,long double y);
double	sqrt (double x);
double	ceil (double x);
double	floor (double x);
double	fabs (double x);
double	ldexp (double x, int n);
double	frexp (double x, int* exp);
double	modf (double x, double* ip);
long double modfl (long double x,long double* ip);
double	fmod (double x, double y);


#ifndef	__STRICT_ANSI__

/* Complex number (for cabs) */
struct _complex
{
	double	x;	/* Real part */
	double	y;	/* Imaginary part */
};

double	_cabs (struct _complex x);
double	_hypot (double x, double y);
double	_j0 (double x);
double	_j1 (double x);
double	_jn (int n, double x);
double	_y0 (double x);
double	_y1 (double x);
double	_yn (int n, double x);

#ifndef	_NO_OLDNAMES

/*
 * Non-underscored versions of non-ANSI functions. These reside in
 * liboldnames.a. Provided for extra portability.
 */
double cabs (struct _complex x);
double hypot (double x, double y);
double j0 (double x);
double j1 (double x);
double jn (int n, double x);
double y0 (double x);
double y1 (double x);
double yn (int n, double x);

#endif	/* Not _NO_OLDNAMES */

#endif	/* Not __STRICT_ANSI__ */

#ifdef __cplusplus
}
#endif

#endif /* Not _MATH_H_ */

