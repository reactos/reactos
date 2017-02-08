/* @(#)fconv.c	1.45 10/11/06 Copyright 1985, 1995-2010 J. Schilling */
/*
 *	Convert floating point numbers to strings for format.c
 *	Should rather use the MT-safe routines [efg]convert()
 *
 *	Copyright (c) 1985, 1995-2010 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/mconfig.h>	/* <- may define NO_FLOATINGPOINT */
#ifndef	NO_FLOATINGPOINT

#ifndef	__DO_LONG_DOUBLE__
#include <schily/stdlib.h>
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/math.h>	/* The default place for isinf()/isnan() */
#include <schily/nlsdefs.h>

#if	!defined(HAVE_STDLIB_H) || defined(HAVE_DTOA)
extern	char	*ecvt __PR((double, int, int *, int *));
extern	char	*fcvt __PR((double, int, int *, int *));
#endif

#if	defined(HAVE_ISNAN) && defined(HAVE_ISINF)
/*
 * *BSD alike libc
 */
#define	FOUND_ISNAN
#define	FOUND_ISINF
#define	FOUND_ISXX
#endif

#if	defined(HAVE_C99_ISNAN) && defined(HAVE_C99_ISINF)
#ifndef	FOUND_ISXX
#define	FOUND_ISXX
#endif
#define	FOUND_C99_ISNAN
#define	FOUND_C99_ISINF
#define	FOUND_C99_ISXX
#endif

#if	defined(HAVE_FP_H) && !defined(FOUND_ISXX)
/*
 * WAS:
 * #if	defined(__osf__) || defined(_IBMR2) || defined(_AIX)
 */
/*
 * Moved before HAVE_IEEEFP_H for True64 due to a hint
 * from Bert De Knuydt <Bert.Deknuydt@esat.kuleuven.ac.be>
 *
 * True64 has both fp.h & ieeefp.h but the functions
 * isnand() & finite() seem to be erreneously not implemented
 * as a macro and the function lives in libm.
 * Let's hope that we will not get problems with the new order.
 */
#include <fp.h>
#if	!defined(isnan) && defined(IS_NAN)
#define	isnan	IS_NAN
#define	FOUND_ISNAN
#endif
#if	!defined(isinf) && defined(FINITE)
#define	isinf	!FINITE
#define	FOUND_ISINF
#endif
#if	defined(FOUND_ISNAN) && defined(FOUND_ISINF)
#define	FOUND_ISXX
#endif
#endif

#if	defined(HAVE_IEEEFP_H) && !defined(FOUND_ISXX) && \
	!defined(FOUND_C99_ISXX)
/*
 * SVR4
 */
#include <ieeefp.h>
#ifdef	HAVE_ISNAND
#ifndef	isnan
#define	isnan	isnand
#define	FOUND_ISNAN
#endif
#endif
#ifdef	HAVE_FINITE
#ifndef	isinf
#define	isinf	!finite
#define	FOUND_ISINF
#endif
#endif
#if	defined(FOUND_ISNAN) && defined(FOUND_ISINF)
#define	FOUND_ISXX
#endif
#endif

/*
 * WAS:
 * #if	defined(__hpux) || defined(VMS) || defined(_SCO_DS) || defined(__QNX__)
 */
#ifdef	__nneded__
#if	defined(__hpux) || defined(__QNX__) || defined(__DJGPP__)
#ifndef	FOUND_C99_ISXX
#undef	isnan
#undef	isinf
#endif
#endif
#endif	/* __needed__ */

/*
 * As we no longer check for defined(isnan)/defined(isinf), the next block
 * should also handle the problems with DJGPP, HP-UX, QNX and VMS.
 */
#if	!defined(FOUND_ISNAN) && !defined(HAVE_C99_ISNAN)
#undef	isnan
#define	isnan(val)	(0)
#define	NO_ISNAN
#endif
#if	!defined(FOUND_ISINF) && !defined(HAVE_C99_ISINF)
#undef	isinf
#define	isinf(val)	(0)
#define	NO_ISINF
#endif

#if	defined(NO_ISNAN) || defined(NO_ISINF)
#include <schily/float.h>	/* For values.h */
#if	(_IEEE - 0) > 0		/* We know that there is IEEE FP */
/*
 * Note that older HP-UX versions have different #defines for MAXINT in
 * values.h and sys/param.h
 */
#include <schily/utypes.h>
#include <schily/btorder.h>

#ifdef	WORDS_BIGENDIAN
#define	fpw_high(x)	((UInt32_t *)&x)[0]
#define	fpw_low(x)	((UInt32_t *)&x)[1]
#else
#define	fpw_high(x)	((UInt32_t *)&x)[1]
#define	fpw_low(x)	((UInt32_t *)&x)[0]
#endif
#define	FP_EXP		0x7FF00000
#define	fp_exp(x)	(fpw_high(x) & FP_EXP)
#define	fp_exc(x)	(fp_exp(x) == FP_EXP)

#ifdef	NO_ISNAN
#undef	isnan
#define	isnan(val)	(fp_exc(val) && \
			(fpw_low(val) != 0 || (fpw_high(val) & 0xFFFFF) != 0))
#endif
#ifdef	NO_ISINF
#undef	isinf
#define	isinf(val)	(fp_exc(val) && \
			fpw_low(val) == 0 && (fpw_high(val) & 0xFFFFF) == 0)
#endif
#endif	/* We know that there is IEEE FP */
#endif	/* defined(NO_ISNAN) || defined(NO_ISINF) */


#if !defined(HAVE_ECVT) || !defined(HAVE_FCVT) || !defined(HAVE_GCVT)

#ifdef	NO_USER_XCVT
	/*
	 * We cannot define our own ecvt()/fcvt()/gcvt() so we need to use
	 * local names instead.
	 */
#ifndef	HAVE_ECVT
#define	ecvt	js_ecvt
#endif
#ifndef	HAVE_FCVT
#define	fcvt	js_fcvt
#endif
#ifndef	HAVE_GCVT
#define	gcvt	js_gcvt
#endif
#endif

#include "cvt.c"
#endif

static	char	_js_nan[] = "(NaN)";
static	char	_js_inf[] = "(Infinity)";

static	int	_ferr __PR((char *, double));
#endif	/* __DO_LONG_DOUBLE__ */

#ifdef	__DO_LONG_DOUBLE__
#undef	MDOUBLE
#define	MDOUBLE	long double
#else
#undef	MDOUBLE
#define	MDOUBLE	double
#endif

#ifdef	abs
#undef	abs
#endif
#define	abs(i)	((i) < 0 ? -(i) : (i))

EXPORT int
ftoes(s, val, fieldwidth, ndigits)
	register	char 	*s;
			MDOUBLE	val;
	register	int	fieldwidth;
	register	int	ndigits;
{
	register	char	*b;
	register	char	*rs;
	register	int	len;
	register	int	rdecpt;
			int 	decpt;
			int	sign;

#ifndef	__DO_LONG_DOUBLE__
	if ((len = _ferr(s, val)) > 0)
		return (len);
#endif
	rs = s;
#ifdef	V7_FLOATSTYLE
	b = ecvt(val, ndigits, &decpt, &sign);
	rdecpt = decpt;
#else
	b = ecvt(val, ndigits+1, &decpt, &sign);
	rdecpt = decpt-1;
#endif
#ifdef	__DO_LONG_DOUBLE__
	len = *b;
	if (len < '0' || len > '9') {		/* Inf/NaN */
		strcpy(s, b);
		return (strlen(b));
	}
#endif
	len = ndigits + 6;			/* Punkt e +/- nnn */
	if (sign)
		len++;
	if (fieldwidth > len)
		while (fieldwidth-- > len)
			*rs++ = ' ';
	if (sign)
		*rs++ = '-';
#ifndef	V7_FLOATSTYLE
	if (*b)
		*rs++ = *b++;
#endif
#if defined(HAVE_LOCALECONV) && defined(USE_LOCALE)
	*rs++ = *(localeconv()->decimal_point);
#else
	*rs++ = '.';
#endif
	while (*b && ndigits-- > 0)
		*rs++ = *b++;
	*rs++ = 'e';
	*rs++ = rdecpt >= 0 ? '+' : '-';
	rdecpt = abs(rdecpt);
#ifdef	__DO_LONG_DOUBLE__
	if (rdecpt >= 1000) {			/* Max-Exp is > 4000 */
		*rs++ = rdecpt / 1000 + '0';
		rdecpt %= 1000;
	}
#endif
#ifndef	V7_FLOATSTYLE
	if (rdecpt >= 100)
#endif
	{
		*rs++ = rdecpt / 100 + '0';
		rdecpt %= 100;
	}
	*rs++ = rdecpt / 10 + '0';
	*rs++ = rdecpt % 10 + '0';
	*rs = '\0';
	return (rs - s);
}

/*
 * fcvt() from Cygwin32 is buggy.
 */
#if	!defined(HAVE_FCVT) && defined(HAVE_ECVT)
#define	USE_ECVT
#endif

EXPORT int
ftofs(s, val, fieldwidth, ndigits)
	register	char 	*s;
			MDOUBLE	val;
	register	int	fieldwidth;
	register	int	ndigits;
{
	register	char	*b;
	register	char	*rs;
	register	int	len;
	register	int	rdecpt;
			int 	decpt;
			int	sign;

#ifndef	__DO_LONG_DOUBLE__
	if ((len = _ferr(s, val)) > 0)
		return (len);
#endif
	rs = s;
#ifdef	USE_ECVT
	/*
	 * Needed on systems with broken fcvt() implementation
	 * (e.g. Cygwin32)
	 */
	b = ecvt(val, ndigits, &decpt, &sign);
	/*
	 * The next call is needed to force higher precision.
	 */
	if (decpt > 0)
		b = ecvt(val, ndigits+decpt, &decpt, &sign);
#else
	b = fcvt(val, ndigits, &decpt, &sign);
#endif
#ifdef	__DO_LONG_DOUBLE__
	len = *b;
	if (len < '0' || len > '9') {		/* Inf/NaN */
		strcpy(s, b);
		return (strlen(b));
	}
#endif
	rdecpt = decpt;
	len = rdecpt + ndigits + 1;
	if (rdecpt < 0)
		len -= rdecpt;
	if (sign)
		len++;
	if (fieldwidth > len)
		while (fieldwidth-- > len)
			*rs++ = ' ';
	if (sign)
		*rs++ = '-';
	if (rdecpt > 0) {
		len = rdecpt;
		while (*b && len-- > 0)
			*rs++ = *b++;
#ifdef	USE_ECVT
		while (len-- > 0)
			*rs++ = '0';
#endif
	}
#ifndef	V7_FLOATSTYLE
	else {
		*rs++ = '0';
	}
#endif
#if defined(HAVE_LOCALECONV) && defined(USE_LOCALE)
	*rs++ = *(localeconv()->decimal_point);
#else
	*rs++ = '.';
#endif
	if (rdecpt < 0) {
		len = rdecpt;
		while (len++ < 0 && ndigits-- > 0)
			*rs++ = '0';
	}
	while (*b && ndigits-- > 0)
		*rs++ = *b++;
#ifdef	USE_ECVT
	while (ndigits-- > 0)
		*rs++ = '0';
#endif
	*rs = '\0';
	return (rs - s);
}

#ifndef	__DO_LONG_DOUBLE__

#ifdef	HAVE_LONGDOUBLE
#ifdef	HAVE__LDECVT
#define	qecvt(ld, n, dp, sp)	_ldecvt(*(long_double *)&ld, n, dp, sp)
#endif
#ifdef	HAVE__LDFCVT
#define	qfcvt(ld, n, dp, sp)	_ldfcvt(*(long_double *)&ld, n, dp, sp)
#endif

#if	(defined(HAVE_QECVT) || defined(HAVE__LDECVT)) && \
	(defined(HAVE_QFCVT) || defined(HAVE__LDFCVT))
#define	__DO_LONG_DOUBLE__
#define	ftoes	qftoes
#define	ftofs	qftofs
#define	ecvt	qecvt
#define	fcvt	qfcvt
#include "fconv.c"
#undef	__DO_LONG_DOUBLE__
#endif
#endif	/* HAVE_LONGDOUBLE */

LOCAL int
_ferr(s, val)
	char	*s;
	double	val;
{
	if (isnan(val)) {
		strcpy(s, _js_nan);
		return (sizeof (_js_nan) - 1);
	}

	/*
	 * Check first for NaN because finite() will return 1 on Nan too.
	 */
	if (isinf(val)) {
		strcpy(s, _js_inf);
		return (sizeof (_js_inf) - 1);
	}
	return (0);
}
#endif	/* __DO_LONG_DOUBLE__ */
#endif	/* NO_FLOATINGPOINT */
