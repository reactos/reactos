/* @(#)wchar.h	1.21 11/07/19 Copyright 2007-2011 J. Schilling */
/*
 *	Abstraction from wchar.h
 *
 *	Copyright (c) 2007-2011 J. Schilling
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

#ifndef _SCHILY_WCHAR_H
#define	_SCHILY_WCHAR_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef	_SCHILY_STDLIB_H
#include <schily/stdlib.h>	/* for MB_CUR_MAX, mbtowc()/wctomb() */
#endif
#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif
#ifdef	HAVE_STDDEF_H
#ifndef	_INCL_STDDEF_H
#include <stddef.h>		/* Needed for e.g. size_t (POSIX)  */
#define	_INCL_STDDEF_H
#endif
#endif
#ifndef _SCHILY_STDIO_H
#include <schily/stdio.h>	/* Needed for e.g. FILE (POSIX)	   */
#endif
#ifndef	_SCHILY_VARARGS_H
#include <schily/varargs.h>	/* Needed for e.g. va_list (POSIX) */
#endif


#if	!defined(HAVE_MBTOWC) || !defined(HAVE_WCTOMB)
#if	defined(HAVE_MBRTOWC) && defined(HAVE_WCRTOMB)
#	define	mbtowc(wp, cp, len)	mbrtowc(wp, cp, len, (mbstate_t *)0)
#	define	wctomb(cp, wc)		wcrtomb(cp, wc, (mbstate_t *)0)
#else
#	define	NO_WCHAR
#endif
#endif

#ifdef	HAVE_WCHAR_H

#ifndef	_INCL_WCHAR_H
#include <wchar.h>
#define	_INCL_WCHAR_H
#endif

#ifndef	HAVE_MBSINIT
#define	mbsinit(sp)			((int)((sp) == 0))
#endif
#ifndef	HAVE_MBRTOWC
#define	mbrtowc(wp, cp, len, sp)	mbtowc(wp, cp, len)
#endif
#ifndef	HAVE_WCRTOMB
#define	wcrtomb(cp, wc, sp)		wctomb(cp, wc)
#endif

#ifndef	USE_WCHAR
#define	USE_WCHAR
#endif

#else	/* HAVE_WCHAR_H */

#undef	USE_WCHAR
#endif	/* !HAVE_WCHAR_H */

#if	!defined(HAVE_WCTYPE_H) && !defined(HAVE_ISWPRINT)
#undef	USE_WCHAR
#undef	USE_WCTYPE
#endif

#ifdef	NO_WCTYPE
#undef	USE_WCHAR
#undef	USE_WCTYPE
#endif

#ifdef	NO_WCHAR
#undef	USE_WCHAR
#undef	USE_WCTYPE
#endif

#ifndef	USE_WCHAR

/*
 * We either don't have wide chars or we don't use them...
 */
#undef	wchar_t
#define	wchar_t	char
#undef	wint_t
#define	wint_t	int
/*
 * We cannot define wctype_t here because of a bug in Linux (missing xctype_t
 * definition in wchar.h
 */
#ifdef	__never__
#undef	wctype_t
#define	wctype_t	int
#endif

#undef	WEOF
#define	WEOF	((wint_t)-1)

#ifndef	_SCHILY_UTYPES_H
#include <schily/utypes.h>	/* For TYPE_MAXVAL() */
#endif

#undef	WCHAR_MAX
#define	WCHAR_MAX	TYPE_MAXVAL(wchar_t)
#undef	WCHAR_MIN
#define	WCHAR_MIN	TYPE_MINVAL(wchar_t)

#undef	WINT_MAX
#define	WINT_MAX	TYPE_MAXVAL(wint_t)
#undef	WINT_MIN
#define	WINT_MIN	TYPE_MINVAL(wint_t)

#undef	WCTYPE_MAX
#define	WCTYPE_MAX	TYPE_MAXVAL(wctype_t)
#undef	WCTYPE_MIN
#define	WCTYPE_MIN	TYPE_MINVAL(wctype_t)

#undef	SIZEOF_WCHAR_T
#define	SIZEOF_WCHAR_T	SIZEOF_CHAR

#undef	MB_CUR_MAX
#define	MB_CUR_MAX	1
#undef	MB_LEN_MAX
#define	MB_LEN_MAX	1

/*
 * The mbtowc() for the non-multibyte case could be as simple as
 *
 * #define	mbtowc(wp, cp, len)	(*(wp) = *(cp), 1)
 *
 * but Mac OS X forces us to do many mbtowc(NULL, NULL, 0) calls in order
 * to reset the internal state. On other platforms that do not support
 * wide chars, NULL may be defined as (void *)0, so we need to check
 * for "wp" != NULL and to cast "wp" and "cp" to their expected types.
 */
#undef	mbtowc
#define	mbtowc(wp, cp, len)	((void)((wp) ? \
				*(wchar_t *)(wp) = *(char *)(cp) : 1), 1)
#undef	mbrtowc
#define	mbrtowc(wp, cp, len, sp) ((void)((wp) ? \
				*(wchar_t *)(wp) = *(char *)(cp) : 1), 1)
#undef	wctomb
#define	wctomb(cp, wc)		(*(cp) = wc, 1)
#undef	wcrtomb
#define	wcrtomb(cp, wc, sp)	(*(cp) = wc, 1)

#undef	mbsinit
#define	mbsinit(sp)		((int)((sp) == 0))

#undef	wcwidth
#define	wcwidth(wc)		(1)

#else	/* USE_WCHAR */

#ifndef	HAVE_WCWIDTH
#undef	wcwidth
#define	wcwidth(wc)		(1)
#endif

#endif	/* USE_WCHAR */

#endif	/* _SCHILY_WCHAR_H */
