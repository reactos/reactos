/* @(#)ccomdefs.h	1.12 11/07/21 Copyright 2000-2011 J. Schilling */
/*
 *	Various compiler dependant macros.
 *
 *	Copyright (c) 2000-2011 J. Schilling
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

#ifndef _SCHILY_CCOMDEFS_H
#define	_SCHILY_CCOMDEFS_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Compiler-dependent macros to declare that functions take printf-like
 * or scanf-like arguments. They are defined to nothing for versions of gcc
 * that are not known to support the features properly (old versions of gcc-2
 * didn't permit keeping the keywords out of the application namespace).
 */
#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7) || \
	defined(NO_PRINTFLIKE)

#define	__printflike__(fmtarg, firstvararg)
#define	__printf0like__(fmtarg, firstvararg)
#define	__scanflike__(fmtarg, firstvararg)

#else /* We found GCC that supports __attribute__ */

#define	__printflike__(fmtarg, firstvararg) \
		__attribute__((__format__(__printf__, fmtarg, firstvararg)))
#define	__printf0like__(fmtarg, firstvararg) \
		__attribute__((__format__(__printf0__, fmtarg, firstvararg)))

/*
 * FreeBSD GCC implements printf0 that allows the format string to
 * be a NULL pointer.
 */
#if	__FreeBSD_cc_version < 300001
#undef	__printf0like__
#define	__printf0like__	__printflike__
#endif

#define	__scanflike__(fmtarg, firstvararg) \
		__attribute__((__format__(__scanf__, fmtarg, firstvararg)))

#endif /* GNUC */

#if __GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ > 2
/* GCC-3.3 or more */

/* CSTYLED */
#define	UConst	__attribute__ ((__used__)) const

#else	/* less than GNUC 3.3 */

#define	UConst	const

#endif /* less than GNUC 3.3 */

#ifdef	__PCC__
/*
 * Hack until pcc supports __attribute__ ((__used__))
 */
#ifdef	UConst
#undef	UConst
#define	UConst	const
#endif
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_CCOMDEFS_H */
