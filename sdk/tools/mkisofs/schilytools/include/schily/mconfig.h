/* @(#)mconfig.h	1.71 15/08/14 Copyright 1995-2015 J. Schilling */
/*
 *	definitions for machine configuration
 *
 *	Copyright (c) 1995-2015 J. Schilling
 *
 *	This file must be included before any other file.
 *	If this file is not included before stdio.h you will not be
 *	able to get LARGEFILE support
 *
 *	Use only cpp instructions.
 *
 *	NOTE: SING: (Schily Is Not Gnu)
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

#ifndef _SCHILY_MCONFIG_H
#define	_SCHILY_MCONFIG_H

/*
 * Tell our users that this is a Schily SING compile environment.
 */
#define	IS_SCHILY

/*
 * We need to do this before we include xconfig.h
 */
#ifdef	NO_LARGEFILES
#undef	USE_LARGEFILES
#endif
#ifdef	NO_ACL
#undef	USE_ACL
#endif

/*
 * Inside <schily/archdefs.h> we get architecture specific Processor defines
 * fetched from compiler predefinitions only.
 */
#include <schily/archdefs.h>

/*
 * Inside <schily/xconfig.h> we decide whether to use static or dynamic
 * autoconf stuff.
 */
#include <schily/xconfig.h>

/*
 * Make sure that neither HAVE_LARGEFILES nor USE_LARGEFILES is defined
 * if the platform does not support large files.
 */
#ifndef	HAVE_LARGEFILES
#undef	USE_LARGEFILES
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The NetBSD people want to bother us.
 * They removed the definition for 'unix' and are bleating for every test
 * for #if defined(unix). So we need to check for NetBSD early.
 */
#ifndef	IS_UNIX
#	if defined(__NetBSD__)
#		define	IS_UNIX
#	endif
#endif

#ifndef	IS_UNIX
#	if (defined(unix) || defined(__unix) || defined(__unix__)) && \
	!defined(__DJGPP__)
#		define	IS_UNIX
#	endif
#endif

#ifdef	__MSDOS__
#	define	IS_MSDOS
#endif

#if defined(tos) || defined(__tos)
#	define	IS_TOS
#endif

#ifdef	THINK_C
#	define	IS_MAC
#endif

#if defined(sun) || defined(__sun) || defined(__sun__)
#	define	IS_SUN
#endif

#if defined(__CYGWIN32__) || defined(__CYGWIN__)
#	define	IS_GCC_WIN32
#	define	IS_CYGWIN

#if	defined(unix) || defined(_X86)
#	define	IS_CYGWIN_1
#endif
#endif

/* ------------------------------------------------------------------------- */
/*
 * Some magic that cannot (yet) be figured out with autoconf.
 */

#if defined(sun3) || defined(mc68000) || defined(mc68020)
#	ifndef	HAVE_SCANSTACK
#	define	HAVE_SCANSTACK
#	endif
#endif
#ifdef sparc
#	ifndef	HAVE_LDSTUB
#	define	HAVE_LDSTUB
#	endif
#	ifndef	HAVE_SCANSTACK
#	define	HAVE_SCANSTACK
#	endif
#endif
#if	defined(__i386_) || defined(i386)
#	ifndef	HAVE_XCHG
#	define	HAVE_XCHG
#	endif
#	ifndef	HAVE_SCANSTACK
#	define	HAVE_SCANSTACK
#	endif
#endif

/*
 * Use of SCANSTACK is disabled by default
 */
#ifndef	USE_SCANSTACK
#	undef	HAVE_SCANSTACK
#else
/*
 * But ....
 * The tests are much better now, so always give it a chance.
 */
#ifndef	HAVE_SCANSTACK
#	define	HAVE_SCANSTACK
#endif
#endif

/*
 * Allow to overwrite the defines in the makefiles by calling
 *
 *	make COPTX=-DFORCE_SCANSTACK
 */
#ifdef	FORCE_SCANSTACK
#	undef	NO_SCANSTACK
#ifndef	HAVE_SCANSTACK
#	define	HAVE_SCANSTACK
#endif
#ifndef	USE_SCANSTACK
#	define	USE_SCANSTACK
#endif
#endif

/*
 * This is the global switch to deactivate stack scanning
 */
#ifdef	NO_SCANSTACK
#	ifdef	HAVE_SCANSTACK
#	undef	HAVE_SCANSTACK
#	endif
#endif

/*
 * This is the global switch to deactivate using #pragma weak
 */
#ifdef	NO_PRAGMA_WEAK
#	ifdef	HAVE_PRAGMA_WEAK
#	undef	HAVE_PRAGMA_WEAK
#	endif
#endif

#ifdef	NO_FORK
#	ifdef	HAVE_FORK
#	undef	HAVE_FORK
#	endif
#	ifdef	HAVE_VFORK
#	undef	HAVE_VFORK
#	endif
#endif
#ifdef	NO_VFORK
#	ifdef	HAVE_VFORK
#	undef	HAVE_VFORK
#	endif
#endif

#if	defined(SOL2) || defined(SOL2) || \
	defined(S5R4) || defined(__S5R4) || defined(SVR4)
#	ifndef	__SVR4
#		define	__SVR4
#	endif
#endif

#ifdef	__SVR4
#	ifndef	SVR4
#		define	SVR4
#	endif
#endif

/*
 * SunOS 4.x / SunOS 5.x
 */
#if defined(IS_SUN)
#	define	HAVE_GETAV0
#endif

/*
 * AIX
 */
#if	defined(_IBMR2) || defined(_AIX)
#	ifndef	IS_UNIX
#	define	IS_UNIX		/* ??? really ??? */
#	endif
#endif

/*
 * QNX
 */
#if defined(__QNX__)
#	ifndef	IS_UNIX
#	define	IS_UNIX
#	endif
#endif

/*
 * Silicon Graphics	(must be before SVR4)
 */
#if defined(sgi) || defined(__sgi)
#	define	__NOT_SVR4__	/* Not a real SVR4 implementation */
#endif

/*
 * Data General
 */
#if defined(__DGUX__)
#ifdef	XXXXXXX
#	undef	HAVE_MTGET_DSREG
#	undef	HAVE_MTGET_RESID
#	undef	HAVE_MTGET_FILENO
#	undef	HAVE_MTGET_BLKNO
#endif
#	define	mt_type		mt_model
#	define	mt_dsreg	mt_status1
#	define	mt_erreg	mt_status2
	/*
	 * DGUX hides its flock as dg_flock.
	 */
#	define	HAVE_FLOCK
#	define	flock	dg_flock
	/*
	 * Use the BSD style wait on DGUX to get the resource usages of child
	 * processes.
	 */
#	define	_BSD_WAIT_FLAVOR
#endif

/*
 * Apple Rhapsody (This is the name for Mac OS X beta)
 */
#if defined(__NeXT__) && defined(__TARGET_OSNAME) && __TARGET_OSNAME == rhapsody
#	define	HAVE_OSDEF /* prevent later definitions to overwrite current */
#	ifndef	IS_UNIX
#	define	IS_UNIX
#	endif
#endif

/*
 * NextStep
 */
#if defined(__NeXT__) && !defined(HAVE_OSDEF)
#define	NO_PRINT_OVR
#undef	HAVE_USG_STDIO
				/*
				 * NeXT Step 3.x uses
				 * __flsbuf(unsigned char, FILE *)
				 * instead of __flsbuf(int, FILE *)
				 */
#	ifndef	IS_UNIX
#	define	IS_UNIX
#	endif
#endif

/*
 * Mac OS X
 */
#if defined(__APPLE__) && defined(__MACH__)
#	ifndef	IS_UNIX
#	define	IS_UNIX
#	endif
#	define	IS_MACOS_X
#endif

/*
 * NextStep 3.x has a broken linker that does not allow us to override
 * these functions.
 */
#ifndef	__OPRINTF__

#ifdef	NO_PRINT_OVR
#	define	printf	Xprintf
#	define	fprintf	Xfprintf
#	define	sprintf	Xsprintf
#endif

#endif	/* __OPRINTF__ */

/* ------------------------------------------------------------------------- */

#ifndef	_SCHILY_PROTOTYP_H
#include <schily/prototyp.h>
#endif

/*
 * We use HAVE_LONGLONG as generalized test on whether "long long", "__in64" or
 * something similar exist.
 *
 * In case that HAVE_LONGLONG is defined here, this is an indication that
 * "long long" works. We define HAVE_LONG_LONG to keep this knowledge.
 */
#ifdef	HAVE_LONGLONG
#	define	HAVE_LONG_LONG
#endif

/*
 * Microsoft C defines _MSC_VER
 * use __int64 instead of long long and use 0i64 for a signed long long const
 * and 0ui64 for an unsigned long long const.
 *
 * #if defined(HAVE___INT64)
 *	use __int64
 * #elif defined(HAVE_LONGLONG)
 *	use long long
 * #endif
 *
 * Be very careful here as older MSVC versions do not implement long long but
 * rather __int64 and once someone makes 'long long' 128 bits on a 64 bit
 * machine, we may need to check for a MSVC __int128 type.
 */
#ifndef	HAVE_LONGLONG
#	if	defined(HAVE___INT64)
#		define	HAVE_LONGLONG
#	endif
#endif

/*
 * gcc 2.x generally implements the "long long" type.
 */
#ifdef	__GNUC__
#	if	__GNUC__ > 1
#		ifndef	HAVE_LONGLONG
#			define	HAVE_LONGLONG
#		endif
#		ifndef	HAVE_LONG_LONG
#			define	HAVE_LONG_LONG
#		endif
#	endif
#endif

#ifdef	__CHAR_UNSIGNED__	/* GNU GCC define (dynamic)	*/
#ifndef CHAR_IS_UNSIGNED
#define	CHAR_IS_UNSIGNED	/* Sing Schily define (static)	*/
#endif
#endif

/*
 * Convert to GNU name
 */
#ifdef	HAVE_STDC_HEADERS
#	ifndef	STDC_HEADERS
#		define	STDC_HEADERS
#	endif
#endif
/*
 * Convert to SCHILY name
 */
#ifdef	STDC_HEADERS
#	ifndef	HAVE_STDC_HEADERS
#		define	HAVE_STDC_HEADERS
#	endif
#endif

#ifdef	IS_UNIX
#	define	HAVE_PATH_DELIM
#	define	PATH_DELIM		'/'
#	define	PATH_DELIM_STR		"/"
#	define	PATH_ENV_DELIM		':'
#	define	PATH_ENV_DELIM_STR	":"
#	define	far
#	define	near
#endif

/*
 * Win32 with Gygwin
 */
#ifdef	IS_GCC_WIN32
#	define	HAVE_PATH_DELIM
#	define	PATH_DELIM		'/'
#	define	PATH_DELIM_STR		"/"
#	define	PATH_ENV_DELIM		':'
#	define	PATH_ENV_DELIM_STR	":"
#	define	HAVE_DOS_DRIVELETTER
#	define	far
#	define	near
#	define	NEED_O_BINARY
#endif

/*
 * Win32 with Mingw32
 */
#ifdef	__MINGW32__
#	define	HAVE_PATH_DELIM
#	define	PATH_DELIM		'/'
#	define	PATH_DELIM_STR		"/"
#	define	PATH_ENV_DELIM		';'
#	define	PATH_ENV_DELIM_STR	";"
#	define	HAVE_DOS_DRIVELETTER
#	define	far
#	define	near
#	define	NEED_O_BINARY
#endif

/*
 * OS/2 EMX
 */
#ifdef	__EMX__				/* We don't want to call it UNIX */
#	define	HAVE_PATH_DELIM
#	define	PATH_DELIM		'/'
#	define	PATH_DELIM_STR		"/"
#	define	PATH_ENV_DELIM		';'
#	define	PATH_ENV_DELIM_STR	";"
#	define	HAVE_DOS_DRIVELETTER
#	define	far
#	define	near
#	define	NEED_O_BINARY
#endif

#ifdef	__BEOS__			/* We don't want to call it UNIX */
#	define	HAVE_PATH_DELIM
#	define	PATH_DELIM		'/'
#	define	PATH_DELIM_STR		"/"
#	define	PATH_ENV_DELIM		':'
#	define	PATH_ENV_DELIM_STR	":"
#	define	far
#	define	near
#endif

/*
 * DOS with DJGPP
 */
#ifdef	__DJGPP__			/* We don't want to call it UNIX */
#	define	HAVE_PATH_DELIM
#	define	PATH_DELIM		'/'
#	define	PATH_DELIM_STR		"/"
#	define	PATH_ENV_DELIM		';'
#	define	PATH_ENV_DELIM_STR	";"
#	define	HAVE_DOS_DRIVELETTER

#	define	NEED_O_BINARY
#endif

/*
 * Vanilla DOS
 */
#if	defined(IS_MSDOS) && !defined(__DJGPP__)
#	define	HAVE_PATH_DELIM
#	define	PATH_DELIM		'\\'
#	define	PATH_DELIM_STR		"\\"
#	define	PATH_ENV_DELIM		';'
#	define	PATH_ENV_DELIM_STR	";"
#	define	HAVE_DOS_DRIVELETTER

#	define	NEED_O_BINARY
#endif

/*
 * ATARI TOS
 */
#ifdef	IS_TOS
#	define	HAVE_PATH_DELIM
#	define	PATH_DELIM		'\\'
#	define	PATH_DELIM_STR		"\\"
#	define	PATH_ENV_DELIM		','
#	define	PATH_ENV_DELIM_STR	","
#	define	HAVE_DOS_DRIVELETTER
#	define	far
#	define	near
#endif

/*
 * Mac OS 9
 */
#ifdef	IS_MAC
#	define	HAVE_PATH_DELIM
#	define	PATH_DELIM		':'
#	define	PATH_DELIM_STR		":"
#	define	PATH_ENV_DELIM		';'	/* ??? */
#	define	PATH_ENV_DELIM_STR	";"	/* ??? */
#	define	far
#	define	near
#endif

/*
 * I hope this will make compilation on unknown OS easier.
 */
#ifndef	HAVE_PATH_DELIM			/* Default to POSIX rules */
#	define	HAVE_PATH_DELIM
#	define	PATH_DELIM		'/'
#	define	PATH_DELIM_STR		"/"
#	define	PATH_ENV_DELIM		':'
#	define	PATH_ENV_DELIM_STR	":"
#	define	far
#	define	near
#endif

/*
 * Is there a solution for /dev/tty and similar?
 */
#ifdef	HAVE__DEV_NULL
#	define	DEV_NULL		"/dev/null"
#else
#if defined(_MSC_VER) || defined(__MINGW32__)
#	define	DEV_NULL		"NUL"
#else
/*
 * What to do here?
 */
#endif
#endif

#ifdef	DBG_MALLOC
/*
 * We need to include this here already in order to make sure that
 * every program that is based on mconfig.h will include schily/dbgmalloc.h
 * in case that we specify -DDBG_MALLOC
 */
#include <schily/dbgmalloc.h>
#endif

#ifdef __cplusplus
}
#endif

#endif /* _SCHILY_MCONFIG_H */
