/* @(#)wctype.h	1.9 17/08/05 Copyright 2009-2017 J. Schilling */
/*
 *	Abstraction from wctype.h
 *
 *	Copyright (c) 2009-2017 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef _SCHILY_WCTYPE_H
#define	_SCHILY_WCTYPE_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef	_SCHILY_WCHAR_H
#include <schily/wchar.h>
#endif

#ifdef	HAVE_WCTYPE_H
/*
 * Include wctype.h if wchars have been enabled in schily/wchar.h
 */
#ifdef	USE_WCHAR
#ifndef	_INCL_WCTYPE_H
#include <wctype.h>
#define	_INCL_WCTYPE_H
#endif
#ifndef	USE_WCTYPE
#define	USE_WCTYPE
#endif
#endif	/* USE_WCHAR */
#endif	/* HAVE_WCTYPE_H */

#if	defined(HAVE_ISWPRINT) && defined(USE_WCHAR)
#ifndef	USE_WCTYPE
#undef	USE_WCTYPE
#endif
#endif

#if	!defined(HAVE_WCTYPE_H) && !defined(HAVE_ISWPRINT)
#undef	USE_WCTYPE
#endif

#ifdef	NO_WCTYPE
#undef	USE_WCTYPE
#endif

#ifndef	USE_WCTYPE

#ifndef	_SCHILY_CTYPE_H
#include <schily/ctype.h>
#endif

#undef	iswalnum
#define	iswalnum(c)	isalnum(c)
#undef	iswalpha
#define	iswalpha(c)	isalpha(c)
#ifdef	HAVE_ISBLANK
#undef	iswblank
#define	iswblank(c)	isblank(c)
#endif
#undef	iswcntrl
#define	iswcntrl(c)	iscntrl(c)
#undef	iswcntrl
#define	iswcntrl(c)	iscntrl(c)
#undef	iswdigit
#define	iswdigit(c)	isdigit(c)
#undef	iswgraph
#define	iswgraph(c)	isgraph(c)
#undef	iswlower
#define	iswlower(c)	islower(c)
#undef	iswprint
#define	iswprint(c)	isprint(c)
#undef	iswpunct
#define	iswpunct(c)	ispunct(c)
#undef	iswspace
#define	iswspace(c)	isspace(c)
#undef	iswupper
#define	iswupper(c)	isupper(c)
#undef	iswxdigit
#define	iswxdigit(c)	isxdigit(c)

#undef	towlower
#define	towlower(c)	tolower(c)
#undef	towupper
#define	towupper(c)	toupper(c)

#endif	/* !USE_WCTYPE */

#endif	/* _SCHILY_WCTYPE_H */
