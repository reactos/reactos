/* @(#)fcntl.h	1.19 11/10/19 Copyright 1996-2011 J. Schilling */
/*
 *	Generic header for users of open(), creat() and chmod()
 *
 *	Copyright (c) 1996-2011 J. Schilling
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

#ifndef _SCHILY_FCNTL_H
#define	_SCHILY_FCNTL_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>	/* Needed for fcntl.h			*/
#endif

#ifndef	_SCHILY_STAT_H
#include <schily/stat.h>	/* For 3rd arg of open() and chmod()	*/
#endif

#ifdef	HAVE_SYS_FILE_H
/*
 * Historical systems with flock() only need sys/file.h
 */
#	ifndef	_INCL_SYS_FILE_H
#	include <sys/file.h>
#	define	_INCL_SYS_FILE_H
#	endif
#endif
#ifdef	HAVE_FCNTL_H
#	ifndef	_INCL_FCNTL_H
#	include <fcntl.h>
#	define	_INCL_FCNTL_H
#endif
#endif

/*
 * Do not define more than O_RDONLY / O_WRONLY / O_RDWR / O_BINARY
 * The values may differ.
 *
 * O_BINARY is defined here to allow all applications to compile on a non DOS
 * environment without repeating this definition.
 * O_SEARCH, O_DIRECTORY and NOFOLLOW are defined here to allow to compile on
 * older platforms.
 * open(name, O_SEARCH) is like UNOS open(name, "") (open neither for read nor
 * for write). This allows to call fstat() and to get an fd for openat(fd, ...)
 */
#ifndef	O_RDONLY
#	define	O_RDONLY	0
#endif
#ifndef	O_WRONLY
#	define	O_WRONLY	1
#endif
#ifndef	O_RDWR
#	define	O_RDWR		2
#endif
#ifndef	O_BINARY			/* Only present on DOS or similar */
#	define	O_BINARY	0
#endif
#ifndef	O_NDELAY			/* This is undefined on BeOS :-( */
#	define	O_NDELAY	0
#endif
#ifndef	O_EXEC				/* Open for exec only (non-directory) */
#	define	O_EXEC		O_RDONLY
#endif
#ifndef	O_SEARCH			/* Open for search only. */
#	define	O_SEARCH	O_RDONLY
#endif
#ifndef	O_DIRECTORY			/* Fail if not a directory */
#	define	O_DIRECTORY	0
#endif
#ifndef	O_NOFOLLOW			/* Fail if a symlink */
#	define	O_NOFOLLOW	0
#endif

#ifndef	O_ACCMODE
#define	O_ACCMODE		(O_RDONLY|O_WRONLY|O_RDWR|O_EXEC|O_SEARCH)
#endif

#ifdef	O_NOATIME			/* Allow #ifdef O_NOATIME */
#define	_O_NOATIME		O_NOATIME
#else
#define	_O_NOATIME		0
#endif

#ifndef	FD_CLOEXEC
#define	FD_CLOEXEC		1	/* close on exec flag */
#endif

/*
 * The following definitions are used for emulating the *at() functions.
 * We use higher numbers for our definitions, to allow to add emulations
 * for missing functions without causing a clash with system definitions.
 */
#ifndef	HAVE_OPENAT
#ifndef	AT_FDCWD
#define	AT_FDCWD		0xffd19553 /* *at() to working dir */
#endif
#endif
#ifndef	HAVE_FSTATAT
#ifndef	AT_SYMLINK_NOFOLLOW
#define	AT_SYMLINK_NOFOLLOW	0x10000	/* emulate lstat() */
#endif
#endif
#ifndef	HAVE_UNLINKAT
#ifndef	AT_REMOVEDIR
#define	AT_REMOVEDIR		0x20000	/* emulate rmdir() */
#endif
#endif
#ifndef	HAVE_FACCESSAT
#ifndef	AT_EACCESS
#define	AT_EACCESS		0x40000	/* EUID access() */
#endif
#endif
#ifndef	HAVE_LINKAT
#ifndef	AT_SYMLINK_FOLLOW
#define	AT_SYMLINK_FOLLOW	0x80000	/* follow symlinks before link() */
#endif
#endif

#endif	/* _SCHILY_FCNTL_H */
