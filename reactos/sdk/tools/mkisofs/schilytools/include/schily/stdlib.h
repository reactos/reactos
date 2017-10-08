/* @(#)stdlib.h	1.11 15/11/28 Copyright 1996-2015 J. Schilling */
/*
 *	Definitions for stdlib
 *
 *	Copyright (c) 1996-2015 J. Schilling
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

#ifndef _SCHILY_STDLIB_H
#define	_SCHILY_STDLIB_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_STDLIB_H
#ifndef	_INCL_STDLIB_H
#include <stdlib.h>
#define	_INCL_STDLIB_H
#endif
#endif	/* HAVE_STDLIB_H */


#ifdef	HAVE_POSIX_MALLOC_H	/* Haiku */
#ifndef	_INCL_POSIX_MALLOC_H
#include <posix/malloc.h>
#define	_INCL_POSIX_MALLOC_H
#endif
#endif	/* HAVE_POSIX_MALLOC_H */

#ifdef	__cplusplus
extern "C" {
#endif

#if !defined(_INCL_STDLIB_H) && !defined(_INCL_POSIX_MALLOC_H)
extern	char	*malloc();
extern	char	*realloc();
#endif

#ifndef	_INCL_STDLIB_H
extern	double	atof();
#endif

#ifdef	__cplusplus
}
#endif



#ifndef	EXIT_FAILURE
#define	EXIT_FAILURE	1
#endif
#ifndef	EXIT_SUCCESS
#define	EXIT_SUCCESS	0
#endif
#ifndef	RAND_MAX
#define	RAND_MAX	32767
#endif

#endif	/* _SCHILY_STDLIB_H */
