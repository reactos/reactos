/* @(#)locale.h	1.2 09/05/24 Copyright 2009 J. Schilling */
/*
 *	Copyright (c) 2009 J. Schilling
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

#ifndef	_SCHILY_LOCALE_H
#define	_SCHILY_LOCALE_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_LOCALE_H
#ifndef	_INCL_LOCALE_H
#include <locale.h>		/* LC_* definitions */
#define	_INCL_LOCALE_H
#endif
#ifndef	USE_LOCALE
#define	USE_LOCALE
#endif
#else
#undef	USE_LOCALE
#endif

#ifndef	HAVE_SETLOCALE
#undef	USE_LOCALE
#endif

#ifdef	NO_LOCALE
#undef	USE_LOCALE
#endif

#ifndef	USE_LOCALE
#undef	setlocale
#define	setlocale(n, s)		((void *)0)
#endif


#endif	/* _SCHILY_LOCALE_H */
