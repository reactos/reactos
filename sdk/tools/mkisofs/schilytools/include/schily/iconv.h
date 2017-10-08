/* @(#)iconv.h	1.4 08/01/02 Copyright 2007 J. Schilling */
/*
 *	Abstraction from iconv.h
 *
 *	Copyright (c) 2007 J. Schilling
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

#ifndef _SCHILY_ICONV_H
#define	_SCHILY_ICONV_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_ICONV_H
#ifndef	_INCL_ICONV_H
#include <iconv.h>
#define	_INCL_ICONV_H
#endif
#else
#undef	USE_ICONV
#endif

/*
 * Libiconv on Cygwin is not autoconf-friendly.
 * iconv.h #defined iconv to libiconv
 * We would need a AC_CHECK_FUNC() macro that allows to specify includefiles.
 */
#if	defined(HAVE_LIBICONV) && defined(HAVE_LIBICONV_OPEN) && \
	defined(HAVE_LIBICONV_CLOSE) && \
	defined(iconv) && defined(iconv_open) && defined(iconv_close)
#	ifndef	HAVE_ICONV
#	define	HAVE_ICONV
#	endif
#	ifndef	HAVE_ICONV_OPEN
#	define	HAVE_ICONV_OPEN
#	endif
#	ifndef	HAVE_ICONV_CLOSE
#	define	HAVE_ICONV_CLOSE
#	endif
#endif

#if	!defined(HAVE_ICONV_OPEN) || !defined(HAVE_ICONV_CLOSE)
#	undef	HAVE_ICONV
#endif

#ifndef	HAVE_ICONV
#undef	USE_ICONV
#endif

#ifdef	NO_ICONV
#undef	USE_ICONV
#endif

#ifndef	USE_ICONV
#undef	iconv_t
#define	iconv_t		char *
#endif


#endif	/* _SCHILY_ICONV_H */
