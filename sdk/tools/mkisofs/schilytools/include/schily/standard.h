/* @(#)standard.h	1.40 13/05/01 Copyright 1985-2013 J. Schilling */
/*
 *	standard definitions
 *
 *	This file should be included past:
 *
 *	mconfig.h / config.h
 *	stdio.h
 *	stdlib.h	(better use schily/stdlib.h)
 *	unistd.h	(better use schily/unistd.h) needed f. LARGEFILE support
 *
 *	If you need stdio.h, you must include it before standard.h
 *
 *	Copyright (c) 1985-2013 J. Schilling
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

#ifndef _SCHILY_STANDARD_H
#define	_SCHILY_STANDARD_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef	M68000
#	ifndef	tos
#		define	JOS	1
#	endif
#endif

/*
 *	fundamental constants
 */
#ifndef	NULL
#	define	NULL		0
#endif
#ifndef	TRUE
#	define	TRUE		1
#	define	FALSE		0
#endif

/*
 *	Program exit codes used with comerr(), comexit() and similar.
 *
 *	Exit codes between -2 and -63 are currently available to flag
 *	program specific error conditions.
 */
#define	EX_BAD			(-1)	/* Default error exit code	    */
#define	EX_CLASH		(-64)	/* Exit code used with exit clashes */

/*
 *	standard storage class definitions
 */
#define	GLOBAL	extern
#define	IMPORT	extern
#define	EXPORT
#define	INTERN	static
#define	LOCAL	static
#define	FAST	register

#ifndef	PROTOTYPES
#	ifndef	const
#		define	const
#	endif
#	ifndef	signed
#		define	signed
#	endif
#	ifndef	volatile
#		define	volatile
#	endif
#endif	/* PROTOTYPES */

/*
 *	standard type definitions
 *
 *	The hidden Schily BOOL definition is used in case we need to deal
 *	with other BOOL defines on systems we like to port to.
 */
typedef int __SBOOL;
typedef int BOOL;
#ifdef	JOS
#	ifndef	__GNUC__
#	define	NO_VOID
#	endif
#endif
#ifdef	NO_VOID
#	ifndef	lint
		typedef int void;
#	endif
#endif

#if	defined(_INCL_SYS_TYPES_H) || defined(_INCL_TYPES_H) || defined(off_t)
#	ifndef	FOUND_OFF_T
#	define	FOUND_OFF_T
#	endif
#endif
#if	defined(_INCL_SYS_TYPES_H) || defined(_INCL_TYPES_H) || defined(size_t)
#	ifndef	FOUND_SIZE_T
#	define	FOUND_SIZE_T
#	endif
#endif
#if	defined(_MSC_VER) && !defined(_SIZE_T_DEFINED)
#	undef	FOUND_SIZE_T
#endif

#ifdef	__never_def__
/*
 * It turns out that we cannot use the folloginw definition because there are
 * some platforms that do not behave application friendly. These are mainly
 * BSD-4.4 based systems (which #undef a definition when size_t is available.
 * We actually removed this code because of a problem with QNX Neutrino.
 * For this reason, it is important not to include <sys/types.h> directly but
 * via the Schily SING include files so we know whether it has been included
 * before we come here.
 */
#if	defined(_SIZE_T)	|| defined(_T_SIZE_)	|| defined(_T_SIZE) || \
	defined(__SIZE_T)	|| defined(_SIZE_T_)	|| \
	defined(_GCC_SIZE_T)	|| defined(_SIZET_)	|| \
	defined(__sys_stdtypes_h) || defined(___int_size_t_h) || defined(size_t)

#ifndef	FOUND_SIZE_T
#	define	FOUND_SIZE_T	/* We already included a size_t definition */
#endif
#endif
#endif	/* __never_def__ */

#ifdef	__cplusplus
}
#endif

#if defined(_JOS) || defined(JOS)
#	ifndef	_SCHILY_SCHILY_H
#	include <schily/schily.h>
#	endif

#	ifndef	_SCHILY_JOS_DEFS_H
#	include <schily/jos_defs.h>
#	endif

#	ifndef	_SCHILY_JOS_IO_H
#	include <schily/jos_io.h>
#	endif
#endif

#endif	/* _SCHILY_STANDARD_H */
