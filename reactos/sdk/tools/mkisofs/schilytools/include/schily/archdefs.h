/* @(#)archdefs.h	1.14 13/07/23 Copyright 2006-2013 J. Schilling */
/*
 *	Processor, instruction set and OS architecture specific defines.
 *	The information is fetched from compiler predefinitions only.
 *
 *	Copyright (c) 2006-2013 J. Schilling
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

#ifndef _SCHILY_ARCHDEFS_H
#define	_SCHILY_ARCHDEFS_H

/*
 * The code in this file only depends on compiler predefined macros.
 * For this reason, it does not need to include schily/mconfig.h
 */

#if	defined(sun) || defined(__sun) || defined(__sun__)
#ifndef	__sun
#define	__sun
#endif
#endif

#if	defined(sun2)
#ifndef	__mc68010
#define	__mc68010
#endif
#endif

#if	defined(mc68020) || defined(__mc68020) || defined(sun3)
#ifndef	__mc68020
#define	__mc68020
#endif
#endif

#if	defined(__mc68010) || defined(__mc68020)
#ifndef	__mc68000
#define	__mc68000
#endif
#endif

#if	defined(i386) || defined(__i386) || defined(__i386__) || defined(_M_IX86)
#ifndef	__i386
#define	__i386
#endif
#endif

#if	defined(__amd64) || defined(__amd64__) || \
	defined(__x86_64) || defined(__x86_64__) || defined(_M_AMD64)
#ifndef	__amd64
#define	__amd64
#endif
#endif

#if	defined(__i386) || defined(__amd64)
#ifndef	__x86
#define	__x86
#endif
#endif

#if	defined(sparc) || defined(__sparc) || defined(__sparc__)
#ifndef	__sparc
#define	__sparc
#endif
#endif

#if	defined(__sparcv9) || defined(__sparcv9__)
#ifndef	__sparc
#define	__sparc
#endif
#ifndef	__sparcv9
#define	__sparcv9
#endif
#endif

#if	defined(__sparc) && defined(__arch64__)
#ifndef	__sparcv9
#define	__sparcv9
#endif
#endif

#if	defined(SOL2) || defined(SOL2) || \
	defined(S5R4) || defined(__S5R4) || defined(SVR4)
#	ifndef	__SVR4
#		define	__SVR4
#	endif
#endif

#if	defined(sun2) || defined(sun3) || defined(__sun)
#ifndef	__sun
#define	__sun
#endif
#ifndef	__GNUC__
#ifndef	__SUNPRO_C
#ifdef	__SVR4
#define	__SUNPRO_C
#else
#define	__SUN_C
#endif	/* __SVR4	*/
#endif	/* !__SUNPRO_C	*/
#endif	/* !__GNUC__	*/
#endif	/* __sun	*/


/*
 * SunOS 4 specific defines
 */
#if	defined(__sun) && !defined(__SVR4)

#define	__SUNOS4

#if	defined(__mc68000)
#define	__SUNOS4_MC68000
#ifdef	__GNUC__
#define	__SUNOS4_MC68000_GCC32
#define	__JS_ARCH_DEFINED
#endif
#ifdef	__SUN_C
#define	__SUNOS4_MC68000_CC32
#define	__JS_ARCH_DEFINED
#endif
#endif	/* __mc68000	*/

#if	defined(__sparc)
#define	__SUNOS4_SPARC
#ifdef	__GNUC__
#define	__SUNOS4_SPARC_GCC32
#define	__JS_ARCH_DEFINED
#endif
#if	defined(__SUN_C) || defined(__SUNPRO_C)
#define	__SUNOS4_SPARC_CC32
#define	__JS_ARCH_DEFINED
#endif
#endif	/* __sparc	*/

#endif	/* SunOS 4 */


/*
 * SunOS 5 specific defines
 */
#if	defined(__sun) && defined(__SVR4)

#define	__SUNOS5

#if	defined(__sparc)
#ifdef	__GNUC__
#ifdef	__sparcv9
#define	__SUNOS5_SPARC_GCC64
#else
#define	__SUNOS5_SPARC_GCC32
#endif
#define	__JS_ARCH_DEFINED
#endif
#if	defined(__SUNPRO_C)
#ifdef	__sparcv9
#define	__SUNOS5_SPARC_CC64
#else
#define	__SUNOS5_SPARC_CC32
#endif
#define	__JS_ARCH_DEFINED
#endif
#endif	/* __sparc	*/

#if	defined(__x86)
#ifdef	__GNUC__
#ifdef	__amd64
#define	__SUNOS5_X86_GCC64
#else
#define	__SUNOS5_X86_GCC32
#endif
#define	__JS_ARCH_DEFINED
#endif
#if	defined(__SUNPRO_C)
#ifdef	__amd64
#define	__SUNOS5_X86_CC64
#else
#define	__SUNOS5_X86_CC32
#endif
#define	__JS_ARCH_DEFINED
#endif
#endif	/* __x86	*/

#endif	/* SunOS 5 */

/*
 * HP-UX specific defines
 */
#if	defined(__hpux)

#if	defined(__hppa)
#ifdef	__GNUC__
#ifdef	__LP64__		/* This may be wrong! */
#define	__HPUX_HPPA_GCC64
#else
#define	__HPUX_HPPA_GCC32
#endif
#define	__JS_ARCH_DEFINED
#endif
#if	!defined(__GNUC__)
#ifdef	__LP64__
#define	__HPUX_HPPA_CC64
#else
#define	__HPUX_HPPA_CC32
#endif
#define	__JS_ARCH_DEFINED
#endif
#endif	/* __hppa	*/

#endif	/* HP-UX */

/*
 * Linux specific defines
 */
#if	defined(__linux)

#if	defined(__arm__)
#ifdef	__GNUC__
#ifdef	__ARM_ARCH_5TE__
#define	__LINUX_ARMV5TEJL_GCC32
#define	__JS_ARCH_DEFINED
#endif
#ifndef	__JS_ARCH_DEFINED
#ifdef	__ARM_ARCH_6__
#define	__LINUX_ARMV6L_GCC32
#define	__JS_ARCH_DEFINED
#endif
#endif
#ifndef	__JS_ARCH_DEFINED
#ifdef	__ARM_ARCH_5__
#define	__LINUX_ARMV5L_GCC32
#define	__JS_ARCH_DEFINED
#endif
#endif
#endif	/* __GNUC__	*/
#endif	/* __arm__	*/

#if	defined(__i386__) || defined(i386)
#ifdef	__GNUC__
#define	__LINUX_I386_GCC32
#define	__JS_ARCH_DEFINED
#endif
#endif	/* __i386__	*/
#if	defined(__amd64__) || defined(__amd64)
#ifdef	__GNUC__
#define	__LINUX_amd64_GCC64
#define	__JS_ARCH_DEFINED
#endif
#endif	/* __amd64__	*/

#endif	/* Linux */

/*
 * MS-WIN specific defines
 *
 * cl defines one of:
 *	_M_ALPHA
 *	_M_IX86
 *	_M_AMD64
 *	_M_M68K
 *	_M_PPC		PPC in general
 *	_M_MPPC		Power Macintosh
 *	_M_MRX000	Mips
 *	_M_IA64		Itanium
 */
#if	defined(_MSC_VER) && (defined(_X86_) || defined(_M_IX86))
#define	__MSWIN_X86_CL32
#define	__JS_ARCH_DEFINED
#endif

#if defined(__CYGWIN32__) || defined(__CYGWIN__)
#define	__CYGWIN_X86_GCC
#define	__JS_ARCH_DEFINED
#endif

#endif	/* _SCHILY_ARCHDEFS_H */
