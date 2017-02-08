/* @(#)btorder.h	1.22 12/12/03 Copyright 1996-2012 J. Schilling */
/*
 *	Definitions for Bit and Byte ordering
 *
 *	Copyright (c) 1996-2012 J. Schilling
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


#ifndef	_SCHILY_BTORDER_H
#define	_SCHILY_BTORDER_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>		/* load bit/byte-oder from xmconfig.h */
#endif

#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>		/* try to load isa_defs.h on Solaris */
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Convert bit-order definitions from xconfig.h into our values
 * and verify them.
 */
#if defined(HAVE_C_BITFIELDS)	&& \
    defined(BITFIELDS_LTOH)
#define	_BIT_FIELDS_LTOH
#endif

#if defined(HAVE_C_BITFIELDS)	&& \
    defined(BITFIELDS_HTOL)
#define	_BIT_FIELDS_HTOL
#endif

#if defined(HAVE_C_BITFIELDS) && \
	!defined(BITFIELDS_HTOL)
#define	BITFIELDS_LTOH
#define	_BIT_FIELDS_LTOH
#endif

#if	defined(_BIT_FIELDS_LTOH) && defined(_BIT_FIELDS_HTOL)
/*
 * #error will not work for all compilers (e.g. sunos4)
 * The following line will abort compilation on all compilers
 * if none of the above is defined. And that's  what we want.
 */
error  Only one of _BIT_FIELDS_LTOH or _BIT_FIELDS_HTOL may be defined

#include <schily/err_bit.h>	/* Avoid "unknown directive" with K&R */
#endif


/*
 * Convert byte-order definitions from xconfig.h into our values
 * and verify them.
 * Note that we cannot use the definitions _LITTLE_ENDIAN and _BIG_ENDIAN
 * because they are used on IRIX-6.5 with different meaning.
 */
#if defined(HAVE_C_BIGENDIAN) && \
	!defined(WORDS_BIGENDIAN)
#define	WORDS_LITTLEENDIAN
/* #define	_LITTLE_ENDIAN */
#endif

#if defined(HAVE_C_BIGENDIAN)	&& \
    defined(WORDS_BIGENDIAN)
#undef	WORDS_LITTLEENDIAN
/* #define	_BIG_ENDIAN */
#endif

#if	defined(_BIT_FIELDS_LTOH) || defined(_BIT_FIELDS_HTOL)
/*
 * Bitorder is already known.
 */
#else
/*
 * Bitorder not yet known.
 */
#	if defined(sun3) || defined(mc68000) || \
	    defined(sun4) || defined(__sparc) || defined(sparc) || \
	    defined(__hppa) || defined(_ARCH_PPC) || defined(_IBMR2)
#		define _BIT_FIELDS_HTOL
#	endif

#	if defined(__sgi) && defined(__mips)
#		define _BIT_FIELDS_HTOL
#	endif

#	if defined(__i386__) || defined(__i386) || defined(i386) || \
	    defined(__alpha__) || defined(__alpha) || defined(alpha) || \
	    defined(__arm__) || defined(__arm) || defined(arm)
#		define _BIT_FIELDS_LTOH
#	endif

#	if defined(__ppc__) || defined(ppc) || defined(__ppc) || \
	    defined(__PPC) || defined(powerpc) || defined(__powerpc__)

#		if	defined(__BIG_ENDIAN__)
#			define _BIT_FIELDS_HTOL
#		else
#			define _BIT_FIELDS_LTOH
#		endif
#	endif
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_BTORDER_H */
