/* @(#)dirent.h	1.29 11/08/04 Copyright 1987, 1998, 2000-2011 J. Schilling */
/*
 *	Copyright (c) 1987, 1998, 2000-2011 J. Schilling
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

#ifndef	_SCHILY_DIRENT_H
#define	_SCHILY_DIRENT_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef JOS
#	ifdef	HAVE_SYS_STYPES_H
#		ifndef	_INCL_SYS_STYPES_H
#		include <sys/stypes.h>
#		define	_INCL_SYS_STYPES_H
#		endif
#	endif
#	ifdef	HAVE_SYS_FILEDESC_H
#		ifndef	_INCL_SYS_FILEDESC_H
#		include <sys/filedesc.h>
#		define	_INCL_SYS_FILEDESC_H
#		endif
#	endif
#	define	NEED_READDIR
#	define 	dirent			_direct
#	define	DIR_NAMELEN(dirent)	strlen((dirent)->d_name)
#	define	DIRSIZE	30
#	define	FOUND_DIRSIZE
	typedef struct _dirent {
		char	name[DIRSIZE];
		short	ino;
	} dirent;

#else	/* !JOS */

#	ifndef	_SCHILY_TYPES_H
#		include <schily/types.h>
#	endif
#	ifndef	_SCHILY_STAT_H
#		include <schily/stat.h>
#	endif
#	ifndef	_SCHILY_LIMITS_H
#		include	<schily/limits.h>
#	endif
#	ifndef	_SCHILY_PARAM_H
#		include <schily/param.h>
#	endif
#	ifndef	_SCHILY_STDLIB_H
#	include <schily/stdlib.h>	/* MSVC: get _MAX_DIR */
#	endif

#	ifdef	HAVE_DIRENT_H		/* This a POSIX compliant system */
#		ifndef	_INCL_DIRENT_H
#		include <dirent.h>
#		define	_INCL_DIRENT_H
#		endif
#		define	DIR_NAMELEN(dirent)	strlen((dirent)->d_name)
#		define	_FOUND_DIR_
#	else				/* This is a Pre POSIX system	 */

#	define 	dirent	direct
#	define	DIR_NAMELEN(dirent)	(dirent)->d_namlen

#	if	defined(HAVE_SYS_DIR_H)
#		ifndef	_INCL_SYS_DIR_H
#		include <sys/dir.h>
#		define	_INCL_SYS_DIR_H
#		endif
#		define	_FOUND_DIR_
#	endif

#	if	defined(HAVE_NDIR_H) && !defined(_FOUND_DIR_)
#		ifndef	_INCL_NDIR_H
#		include <ndir.h>
#		define	_INCL_NDIR_H
#		endif
#		define	_FOUND_DIR_
#	endif

#	if	defined(HAVE_SYS_NDIR_H) && !defined(_FOUND_DIR_)
#		ifndef	_INCL_SYS_NDIR_H
#		include <sys/ndir.h>
#		define	_INCL_SYS_NDIR_H
#		endif
#		define	_FOUND_DIR_
#	endif
#	endif	/* HAVE_DIRENT_H */

#	if	defined(_FOUND_DIR_)
/*
 * Don't use defaults here to allow recognition of problems.
 */
#	if !defined(FOUND_DIRSIZE) && defined(MAXNAMELEN)
#		define	DIRSIZE		MAXNAMELEN	/* From sys/param.h */
#		define	FOUND_DIRSIZE
#	endif

#	if !defined(FOUND_DIRSIZE) && defined(MAXNAMLEN)
#		define	DIRSIZE		MAXNAMLEN	/* From dirent.h    */
#		define	FOUND_DIRSIZE
#	endif

#	ifdef	__never__
	/*
	 * DIRSIZ(dp) is a parameterized macro, we cannot use it here.
	 */
#	if !defined(FOUND_DIRSIZE) && defined(DIRSIZ)
#		define	DIRSIZE		DIRSIZ		/* From sys/dir.h   */
#		define	FOUND_DIRSIZE
#	endif
#	endif	/* __never__ */

#	if !defined(FOUND_DIRSIZE) && defined(NAME_MAX)
#		define	DIRSIZE		NAME_MAX	/* From limits.h    */
#		define	FOUND_DIRSIZE
#	endif

#	else	/* !_FOUND_DIR_ */

#	if !defined(FOUND_DIRSIZE) && defined(_MAX_DIR)
#	if	defined(__MINGW32__) || defined(_MSC_VER)
#		define	DIRSIZE		_MAX_DIR	/* From stdlib.h    */
#		define	FOUND_DIRSIZE
#		define	NEED_READDIR
#		undef	dirent
#		define 	dirent			_direct
#		undef	DIR_NAMELEN
#		define	DIR_NAMELEN(dirent)	strlen((dirent)->d_name)
#	endif
#	endif

#		ifndef	NEED_READDIR
#		define	NEED_DIRENT
#		define	NEED_READDIR
#		undef	dirent
#		define 	dirent			_direct
#		undef	DIR_NAMELEN
#		define	DIR_NAMELEN(dirent)	strlen((dirent)->d_name)
#		endif	/* !NEED_READDIR */

#	endif	/* _FOUND_DIR_ */


#ifdef	NEED_DIRENT

#ifndef	FOUND_DIRSIZE
#define	DIRSIZE		14	/* The old UNIX standard value */
#define	FOUND_DIRSIZE
#endif

typedef struct _dirent {
	short	ino;
	char	name[DIRSIZE];
} dirent;

#endif	/* NEED_DIRENT */

#endif	/* !JOS */

#ifdef	NEED_READDIR

#ifndef _SCHILY_STDIO_H
#include <schily/stdio.h>
#endif
#if	defined(__MINGW32__) || defined(_MSC_VER)
#ifndef	_SCHILY_IO_H
#include <schily/io.h>		/* for _findfirst() */
#endif
#ifndef _SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif
#endif

	struct _direct {
		unsigned long	d_ino;
		unsigned short	d_reclen;
		unsigned short	d_namlen;
		char		d_name[DIRSIZE +1];
	};
#define	HAVE_DIRENT_D_INO

	typedef struct __dirdesc {
		FILE		*dd_fd;

#if	defined(__MINGW32__) || defined(_MSC_VER)
		struct _direct	dd_dir;		/* dirent for this dir	*/
		struct _finddata_t dd_data;	/* _findnext() results	*/
		intptr_t	dd_handle;	/* _findnext() handle	*/
		int		dd_state;	/* Current Dir state	*/
		char		dd_dirname[1];	/* Dir to open		*/
#endif
	} DIR;

extern	DIR		*opendir __PR((const char *));
extern	int		closedir __PR((DIR *));
extern	struct dirent	*readdir __PR((DIR *));

#endif	/* NEED_READDIR */

#if	!defined(HAVE_DIRFD) && defined(HAVE_DIR_DD_FD)
#	define	dirfd(dirp)	((dirp)->dd_fd)
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_DIRENT_H */
