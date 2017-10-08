/* @(#)maxpath.h	1.13 17/04/27 Copyright 1985, 1995, 1998, 2001-2017 J. Schilling */
/*
 *	Definitions for dealing with statically limitations on pathnames
 *
 *	Copyright (c) 1985, 1995, 1998, 2001-2017 J. Schilling
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

#ifndef	_SCHILY_MAXPATH_H
#define	_SCHILY_MAXPATH_H

#ifndef	_SCHILY_DIRENT_H
#include <schily/dirent.h>		/* Includes mconfig.h if needed	*/
#endif

#ifdef	JOS
#ifndef	_SCHILY_JOS_MAXP_H
#	include <schily/jos_maxp.h>
#endif
#	ifndef	FOUND_MAXPATHNAME
#	define	FOUND_MAXPATHNAME
#	endif
#	ifndef	FOUND_MAXFILENAME
#	define	FOUND_MAXFILENAME
#	endif
#endif	/* JOS */

#if !defined(FOUND_MAXPATHNAME) && defined(MAXPATHLEN)
#	define	MAXPATHNAME	MAXPATHLEN	/* From sys/param.h */
#	define	FOUND_MAXPATHNAME
#endif

#if !defined(FOUND_MAXPATHNAME) && defined(PATH_MAX)
#	define	MAXPATHNAME	PATH_MAX	/* From limits.h    */
#	define	FOUND_MAXPATHNAME
#endif

#if !defined(FOUND_MAXPATHNAME)
#include <schily/stdlib.h>
#endif
#if !defined(FOUND_MAXPATHNAME) && defined(_MAX_PATH)
#	define	MAXPATHNAME	_MAX_PATH	/* From MS stdlib.h */
#	define	FOUND_MAXPATHNAME
#endif

#if !defined(FOUND_MAXPATHNAME)
#	define	MAXPATHNAME	1024		/* Is there a limit? */
#endif

#ifndef	PATH_MAX
#define	PATH_MAX	MAXPATHNAME
#endif


/*
 * Don't use defaults here to allow recognition of problems.
 */
#if !defined(FOUND_MAXFILENAME) && defined(MAXNAMELEN)
#	define	MAXFILENAME	MAXNAMELEN	/* From sys/param.h */
#	define	FOUND_MAXFILENAME
#endif

#if !defined(FOUND_MAXFILENAME) && defined(MAXNAMLEN)
#	define	MAXFILENAME	MAXNAMLEN	/* From dirent.h    */
#	define	FOUND_MAXFILENAME
#endif

#ifdef	__never__
/*
 * DIRSIZ(dp) is a parameterized macro, we cannot use it here.
 */
#if !defined(FOUND_MAXFILENAME) && defined(DIRSIZ)
#	define	MAXFILENAME	DIRSIZ		/* From sys/dir.h   */
#	define	FOUND_MAXFILENAME
#endif
#endif	/* __never__ */

#if !defined(FOUND_MAXFILENAME) && defined(NAME_MAX)
#	define	MAXFILENAME	NAME_MAX	/* From limits.h    */
#	define	FOUND_MAXFILENAME
#endif

#if !defined(FOUND_MAXFILENAME) && defined(FOUND_DIRSIZE)
#	define	MAXFILENAME	DIRSIZE		/* From schily/dirent.h    */
#	define	FOUND_MAXFILENAME
#endif

#if !defined(FOUND_MAXFILENAME)
#include <schily/stdlib.h>
#endif
#if !defined(FOUND_MAXFILENAME) && defined(_MAX_FNAME)
#	define	MAXFILENAME	_MAX_FNAME	/* From MS stdlib.h */
#	define	FOUND_MAXFILENAME
#endif

#if !defined(FOUND_MAXFILENAME)
#	define	MAXFILENAME	256
#endif

#ifndef	NAME_MAX
#define	NAME_MAX	MAXFILENAME
#endif
#ifndef	MAXNAMLEN
#define	MAXNAMLEN	MAXFILENAME
#endif
#ifndef	MAXNAMELEN
#define	MAXNAMELEN	MAXFILENAME
#endif

#endif	/* _SCHILY_MAXPATH_H */
