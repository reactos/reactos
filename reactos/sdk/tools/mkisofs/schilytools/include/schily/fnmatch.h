/* @(#)fnmatch.h	8.13 10/10/09 Copyright 2006-2010 J. Schilling from 8.1 (Berkeley) */

#ifndef	_SCHILY_FNMATCH_H
#define	_SCHILY_FNMATCH_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_FNMATCH_H
#include <fnmatch.h>
#else	/* !HAVE_FNMATCH_H */

#ifdef	__cplusplus
extern "C" {
#endif

extern int	 fnmatch __PR((const char *, const char *, int));

#ifdef	__cplusplus
}
#endif

#endif /* !HAVE_FNMATCH */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)fnmatch.h	8.13 (Berkeley) 10/09/10
 */

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef	FNM_NOMATCH
#define	FNM_NOMATCH	1	/* Match failed. */
#endif
#ifndef	FNM_ERROR
#define	FNM_ERROR	2	/* An error occured */
#endif
#ifndef	FNM_NOSYS
#define	FNM_NOSYS	3	/* Function (XPG4) not supported */
#endif

#ifndef	FNM_NOESCAPE
#define	FNM_NOESCAPE	0x01	/* Disable backslash escaping. */
#endif
#ifndef	FNM_PATHNAME
#define	FNM_PATHNAME	0x02	/* Slash must be matched by slash. */
#endif
#ifndef	FNM_PERIOD
#define	FNM_PERIOD	0x04	/* Period must be matched by period. */
#endif
#if	!defined(FNM_IGNORECASE) && !defined(FNM_CASEFOLD)
#define	FNM_IGNORECASE	0x10	/* Ignore case when making comparisons */
#endif
#if	!defined(FNM_IGNORECASE) && defined(FNM_CASEFOLD)
#define	FNM_IGNORECASE	FNM_CASEFOLD
#endif
#ifndef	FNM_CASEFOLD
#define	FNM_CASEFOLD	FNM_IGNORECASE
#endif
#ifndef	FNM_LEADING_DIR
#define	FNM_LEADING_DIR	0x20	/* Ignore /<tail> after Imatch. */
#endif

extern int	 js_fnmatch __PR((const char *, const char *, int));

#if	!defined(HAVE_FNMATCH_IGNORECASE)
#define	fnmatch	js_fnmatch
#endif

#ifdef	__cplusplus
}
#endif

#endif /* !_SCHILY_FNMATCH_H */
