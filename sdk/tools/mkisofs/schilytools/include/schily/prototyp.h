/* @(#)prototyp.h	1.17 15/12/26 Copyright 1995-2015 J. Schilling */
/*
 *	Definitions for dealing with ANSI / KR C-Compilers
 *
 *	Copyright (c) 1995-2015 J. Schilling
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

/*
 * <schily/mconfig.h> includes <schily/prototyp.h>
 * To be correct, we need to include <schily/mconfig.h> before we test
 * for _SCHILY_PROTOTYP_H
 *
 * In order to keep the silly Solaris hdrchk(1) quiet, we are forced to
 * have the _SCHILY_PROTOTYP_H first in <schily/prototyp.h>.
 * To keep hdrchk(1) quiet and be correct, we need to introduce a second
 * guard _SCHILY_PROTOTYP_X_H.
 */
#ifndef	_SCHILY_PROTOTYP_H
#define	_SCHILY_PROTOTYP_H

#ifndef _SCHILY_MCONFIG_H
#undef	_SCHILY_PROTOTYP_H
#include <schily/mconfig.h>
#endif

#ifndef	_SCHILY_PROTOTYP_X_H
#define	_SCHILY_PROTOTYP_X_H

#include <schily/ccomdefs.h>

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef	PROTOTYPES
	/*
	 * If this has already been defined,
	 * someone else knows better than us...
	 */
#	ifdef	__STDC__
#		if	__STDC__				/* ANSI C */
#			define	PROTOTYPES
#		endif
#		if	defined(sun) && __STDC__ - 0 == 0	/* Sun C */
#			define	PROTOTYPES
#		endif
#	endif
#endif	/* PROTOTYPES */

#if	!defined(PROTOTYPES) && (defined(__cplusplus) || defined(_MSC_VER))
	/*
	 * C++ always supports prototypes.
	 * Define PROTOTYPES so we are not forced to make
	 * a separtate autoconf run for C++
	 *
	 * Microsoft C has prototypes but does not define __STDC__
	 */
#	define	PROTOTYPES
#endif

/*
 * If we have prototypes, we should have stdlib.h string.h stdarg.h
 */
#ifdef	PROTOTYPES
#if	!(defined(SABER) && defined(sun))
#	ifndef	HAVE_STDARG_H
#		define	HAVE_STDARG_H
#	endif
#endif
#ifndef	JOS
#	ifndef	HAVE_STDLIB_H
#		define	HAVE_STDLIB_H
#	endif
#	ifndef	HAVE_STRING_H
#		define	HAVE_STRING_H
#	endif
#	ifndef	HAVE_STDC_HEADERS
#		define	HAVE_STDC_HEADERS
#	endif
#	ifndef	STDC_HEADERS
#		define	STDC_HEADERS	/* GNU name */
#	endif
#endif
#endif

#ifdef	NO_PROTOTYPES		/* Force not to use prototypes */
#	undef	PROTOTYPES
#endif

#ifdef	PROTOTYPES
#	define	__PR(a)	a
#else
#	define	__PR(a)	()
#endif

#if !defined(PROTOTYPES) && !defined(NO_CONST_DEFINE)
#	ifndef	const
#		define	const
#	endif
#	ifndef	signed
#		define	signed
#	endif
#	ifndef	volatile
#		define	volatile
#	endif
#endif

#ifdef	PROTOTYPES
#define	ALERT	'\a'
#else
#define	ALERT	'\07'
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_PROTOTYP_X_H */
#endif	/* _SCHILY_PROTOTYP_H */
