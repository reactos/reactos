/* @(#)windows.h	1.4 13/10/26 Copyright 2011-2013 J. Schilling */
/*
 *	Definitions for windows.h
 *
 *	Copyright (c) 2011-2013 J. Schilling
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

#ifndef _SCHILY_WINDOWS_H
#define	_SCHILY_WINDOWS_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_WINDOWS_H
#ifndef	_INCL_WINDOWS_H

/*
 * configure believes they are missing and #define's them:
 */
#if defined(_MSC_VER) || defined(__MINGW32__)
/* BEGIN CSTYLED */
#undef	u_char
#undef	u_short
#undef	u_int
#undef	u_long
/* END CSTYLED */
#endif

#if defined(__CYGWIN32__) || defined(__CYGWIN__)

/*
 * Cygwin-1.7.17 (Autumn 2012) makes life hard as it prevents to rename
 * the Cygwin BOOL definition. Note that we have our own BOOL definition
 * in schily/standard.h that exists since 1982 which happened before Microsoft
 * introduced their BOOL.
 *
 * Previous Cygwin versions have been compatible to the original MS include
 * files and allowed to rename the BOOL from windows.h (windef.h) by just using
 * #define BOOL WBOOL before #include <windows.h>.
 *
 * Recent Cygwin version are unfriendly to us and prevent this.
 * We now need a two level #define to redirect the BOOL from windows.h to the
 * Cygwin specific WINBOOL typedef.
 *
 * If we do not include schily/standard.h with newer Cygwin, we cannot get
 * working typedefs for "PBOOL" and "LPBOOL".
 */
#include <schily/standard.h>	/* Get our BOOL typedef */

#define	_NO_BOOL_TYPEDEF	/* Avoid 2nd BOOL typedef on Cygwin-1.7.17 */

#define	WBOOL	WINBOOL		/* Cygwin-1.7.17 prevents to avoid BOOL */
#endif	/* defined(__CYGWIN32__) || defined(__CYGWIN__) */


#define	BOOL	WBOOL		/* This is the Win BOOL		*/
#define	format	__ms_format	/* Avoid format parameter hides global ... */

#ifdef	timerclear		/* struct timeval has already been declared */
#define	timeval	__ms_timeval
#endif

#include <windows.h>

#undef	BOOL			/* MS Code uses WBOOL or #define BOOL WBOOL */
#undef	format			/* Return to previous definition */
#undef	timeval

#define	_INCL_WINDOWS_H
#endif

#endif	/* HAVE_WINDOWS_H */

#endif	/* _SCHILY_WINDOWS_H */
