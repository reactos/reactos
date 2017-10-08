/* @(#)stat.h	1.20 15/08/23 Copyright 1998-2015 J. Schilling */
/*
 *	Definitions for stat() file mode
 *
 *	Copyright (c) 1998-2015 J. Schilling
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

#ifndef	_SCHILY_STAT_H
#define	_SCHILY_STAT_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>	/* Needed for sys/stat.h		*/
#endif
#ifndef	_INCL_SYS_STAT_H
#include <sys/stat.h>		/* For S_IS* and S_IF*			*/
#define	_INCL_SYS_STAT_H
#endif

#if	!defined(HAVE_LSTAT) || !defined(S_IFLNK)
#define	lstat	stat
#endif

/*
 * Apollo Domain/OS has a broken sys/stat.h that defines
 * S_IFIFO == S_IFSOCK and creates trouble if the constants
 * are used as case labels.
 */
#if S_IFIFO == S_IFSOCK
#	undef	S_IFSOCK
#endif

#ifdef	STAT_MACROS_BROKEN
#undef	S_ISFIFO			/* Named pipe		*/
#undef	S_ISCHR				/* Character special	*/
#undef	S_ISMPC				/* UNUSED multiplexed c	*/
#undef	S_ISDIR				/* Directory		*/
#undef	S_ISNAM				/* Named file (XENIX)	*/
#undef	S_ISBLK				/* Block special	*/
#undef	S_ISMPB				/* UNUSED multiplexed b	*/
#undef	S_ISREG				/* Regular file		*/
#undef	S_ISCTG				/* Contiguous file	*/
#undef	S_ISLNK				/* Symbolic link	*/
#undef	S_ISSHAD			/* Solaris shadow inode	*/
#undef	S_ISSOCK			/* UNIX domain socket	*/
#undef	S_ISDOOR			/* Solaris DOOR		*/
#undef	S_ISPORT			/* Solaris event port	*/
#undef	S_ISWHT				/* BSD whiteout		*/
#undef	S_ISEVC				/* UNOS eventcount	*/
#endif

#ifndef	S_ISFIFO			/* 1 Named pipe		*/
#	ifdef	S_IFIFO
#		define	S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#	else
#		define	S_ISFIFO(m)	(0)
#	endif
#endif
#ifndef	S_ISCHR				/* 2 Character special	*/
#	ifdef	S_IFCHR
#		define	S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#	else
#		define	S_ISCHR(m)	(0)
#	endif
#endif
#ifndef	S_ISMPC				/* 3 UNUSED multiplexed char	*/
#	ifdef	S_IFMPC
#		define	S_ISMPC(m)	(((m) & S_IFMT) == S_IFMPC)
#	else
#		define	S_ISMPC(m)	(0)
#	endif
#endif
#ifndef	S_ISDIR				/* 4 Directory			*/
#	ifdef	S_IFDIR
#		define	S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#	else
#		define	S_ISDIR(m)	(0)
#	endif
#endif
#ifndef	S_ISNAM				/* 5 Named file (XENIX)		*/
#	ifdef	S_IFNAM
#		define	S_ISNAM(m)	(((m) & S_IFMT) == S_IFNAM)
#	else
#		define	S_ISNAM(m)	(0)
#	endif
#endif
#ifndef	S_ISBLK				/* 6 Block special		*/
#	ifdef	S_IFBLK
#		define	S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#	else
#		define	S_ISBLK(m)	(0)
#	endif
#endif
#ifndef	S_ISMPB				/* 7 UNUSED multiplexed block	*/
#	ifdef	S_IFMPB
#		define	S_ISMPB(m)	(((m) & S_IFMT) == S_IFMPB)
#	else
#		define	S_ISMPB(m)	(0)
#	endif
#endif
#ifndef	S_ISREG				/* 8 Regular file		*/
#	ifdef	S_IFREG
#		define	S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#	else
#		define	S_ISREG(m)	(0)
#	endif
#endif
#ifndef	S_ISCTG				/* 9 Contiguous file		*/
#	ifdef	S_IFCTG
#		define	S_ISCTG(m)	(((m) & S_IFMT) == S_IFCTG)
#	else
#		define	S_ISCTG(m)	(0)
#	endif
#endif
#ifndef	S_ISLNK				/* 10 Symbolic link		*/
#	ifdef	S_IFLNK
#		define	S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#	else
#		define	S_ISLNK(m)	(0)
#	endif
#endif
#ifndef	S_ISSHAD			/* 11 Solaris shadow inode	*/
#	ifdef	S_IFSHAD
#		define	S_ISSHAD(m)	(((m) & S_IFMT) == S_IFSHAD)
#	else
#		define	S_ISSHAD(m)	(0)
#	endif
#endif
#ifndef	S_ISSOCK			/* 12 UNIX domain socket	*/
#	ifdef	S_IFSOCK
#		define	S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)
#	else
#		define	S_ISSOCK(m)	(0)
#	endif
#endif
#ifndef	S_ISDOOR			/* 13 Solaris DOOR		*/
#	ifdef	S_IFDOOR
#		define	S_ISDOOR(m)	(((m) & S_IFMT) == S_IFDOOR)
#	else
#		define	S_ISDOOR(m)	(0)
#	endif
#endif
#ifndef	S_ISPORT			/* 14 Solaris event port	*/
#	ifdef	S_IFPORT
#		define	S_ISPORT(m)	(((m) & S_IFMT) == S_IFPORT)
#	else
#		define	S_ISPORT(m)	(0)
#	endif
#endif
#ifndef	S_ISWHT				/* 14 BSD whiteout		*/
#	ifdef	S_IFWHT
#		define	S_ISWHT(m)	(((m) & S_IFMT) == S_ISWHT)
#	else
#		define	S_ISWHT(m)	(0)
#	endif
#endif
#ifndef	S_IFEVC				/* 15 UNOS eventcount		*/
#	ifdef	S_IFEVC
#		define	S_ISEVC(m)	(((m) & S_IFMT) == S_IFEVC)
#	else
#		define	S_ISEVC(m)	(0)
#	endif
#endif

#ifndef	S_TYPEISMQ
/*
 * XXX ??? where is a definition of a message queue ???
 */
#		define	S_TYPEISMQ(_stbuf)	(0)
#endif
#ifndef	S_TYPEISSEM
#	ifdef	S_INSEM
#		define	S_TYPEISSEM(_stbuf)	(S_ISNAM((_stbuf)->st_mode) && \
						(_stbuf)->st_rdev == S_INSEM)
#	else
#		define	S_TYPEISSEM(_stbuf)	(0)
#	endif
#endif
#ifndef	S_TYPEISSHM
#	ifdef	S_INSHD
#		define	S_TYPEISSHM(_stbuf)	(S_ISNAM((_stbuf)->st_mode) && \
						(_stbuf)->st_rdev == S_INSHD)
#	else
#		define	S_TYPEISSHM(_stbuf)	(0)
#	endif
#endif

/*
 * Mode permission bits.
 * UNIX V.7 has only	S_ISUID/S_ISGID/S_ISVTX and S_IREAD/S_IWRITE/S_IEXEC
 *
 * S_ISUID/S_ISGID/S_ISVTX is available on UNIX V.7 and POSIX
 */
#ifndef	S_ISUID			/* Set-user-ID on execution */
#define	S_ISUID	0		/* If it is not defined, it is not supported */
#endif
#ifndef	S_ISGID			/* Set-group-ID on execution */
#define	S_ISGID	0		/* If it is not defined, it is not supported */
#endif
#ifndef	S_ISVTX			/* On directories, restricted deletion flag */
#define	S_ISVTX	0		/* If it is not defined, it is not supported */
#endif

/*
 * S_IREAD/S_IWRITE/S_IEXEC is only available on UNIX V.7 but not on POSIX
 * Emulate these definitions to support compilation of programs like
 * SCCS and the Bourne Shell and to make the other definitions simpler.
 */
#ifndef	S_IREAD
#ifdef	S_IRUSR
#define	S_IREAD		S_IRUSR	/* Use POSIX name */
#else
#define	S_IREAD		0400	/* Very old UNIX, use own definition */
#endif
#endif
#ifndef	S_IWRITE
#ifdef	S_IWUSR
#define	S_IWRITE	S_IWUSR	/* Use POSIX name */
#else
#define	S_IWRITE	0200	/* Very old UNIX, use own definition */
#endif
#endif
#ifndef	S_IEXEC
#ifdef	S_IXUSR
#define	S_IEXEC		S_IXUSR	/* Use POSIX name */
#else
#define	S_IEXEC		0100	/* Very old UNIX, use own definition */
#endif
#endif

#ifndef	S_IRUSR			/* Read permission, owner */
#define	S_IRUSR	S_IREAD		/* Needed on old UNIX systems */
#endif
#ifndef	S_IWUSR			/* Write permission, owner */
#define	S_IWUSR	S_IWRITE	/* Needed on old UNIX systems */
#endif
#ifndef	S_IXUSR			/* Execute/search permission, owner */
#define	S_IXUSR	S_IEXEC		/* Needed on old UNIX systems */
#endif
#ifndef	S_IRGRP			/* Read permission, group */
#define	S_IRGRP	(S_IREAD >> 3)	/* Needed on old UNIX systems */
#endif
#ifndef	S_IWGRP			/* Write permission, group */
#define	S_IWGRP	(S_IWRITE >> 3)	/* Needed on old UNIX systems */
#endif
#ifndef	S_IXGRP			/* Execute/search permission, group */
#define	S_IXGRP	(S_IEXEC >> 3)	/* Needed on old UNIX systems */
#endif
#ifndef	S_IROTH			/* Read permission, others */
#define	S_IROTH	(S_IREAD >> 6)	/* Needed on old UNIX systems */
#endif
#ifndef	S_IWOTH			/* Write permission, others */
#define	S_IWOTH	(S_IWRITE >> 6)	/* Needed on old UNIX systems */
#endif
#ifndef	S_IXOTH			/* Execute/search permission, others */
#define	S_IXOTH	(S_IEXEC >> 6)	/* Needed on old UNIX systems */
#endif

#ifndef	S_IRWXU			/* Read, write, execute/search by owner */
#define	S_IRWXU (S_IRUSR|S_IWUSR|S_IXUSR)
#endif
#ifndef	S_IRWXG			/* Read, write, execute/search by group */
#define	S_IRWXG (S_IRGRP|S_IWGRP|S_IXGRP)
#endif
#ifndef	S_IRWXO			/* Read, write, execute/search by others */
#define	S_IRWXO (S_IROTH|S_IWOTH|S_IXOTH)
#endif

/*
 * SCO UnixWare has st_atim.st__tim.tv_nsec but the st_atim.tv_nsec tests also
 * succeeds. If we use st_atim.tv_nsec on UnixWare, we get a warning about
 * illegal structure usage. For this reason, our code needs to have
 * #ifdef HAVE_ST__TIM before #ifdef HAVE_ST_NSEC.
 */
#if	defined(HAVE_ST_ATIMENSEC)

/*
 * Found e.g. on NetBSD and OpenBSD.
 */
#define	stat_ansecs(s)		((s)->st_atimensec)
#define	stat_mnsecs(s)		((s)->st_mtimensec)
#define	stat_cnsecs(s)		((s)->st_ctimensec)

#define	stat_set_ansecs(s, n)	((s)->st_atimensec = n)
#define	stat_set_mnsecs(s, n)	((s)->st_mtimensec = n)
#define	stat_set_cnsecs(s, n)	((s)->st_ctimensec = n)

#define	_FOUND_STAT_NSECS_
#endif

#if	defined(HAVE_ST_ATIME_N)

/*
 * Found e.g. on AIX.
 */
#define	stat_ansecs(s)		((s)->st_atime_n)
#define	stat_mnsecs(s)		((s)->st_mtime_n)
#define	stat_cnsecs(s)		((s)->st_ctime_n)

#define	stat_set_ansecs(s, n)	((s)->st_atime_n = n)
#define	stat_set_mnsecs(s, n)	((s)->st_mtime_n = n)
#define	stat_set_cnsecs(s, n)	((s)->st_ctime_n = n)

#define	_FOUND_STAT_NSECS_
#endif

#if	defined(HAVE_ST__TIM) && !defined(_FOUND_STAT_NSECS_)

/*
 * Found e.g. on UnixWare.
 */
#define	stat_ansecs(s)		((s)->st_atim.st__tim.tv_nsec)
#define	stat_mnsecs(s)		((s)->st_mtim.st__tim.tv_nsec)
#define	stat_cnsecs(s)		((s)->st_ctim.st__tim.tv_nsec)

#define	stat_set_ansecs(s, n)	((s)->st_atim.st__tim.tv_nsec = n)
#define	stat_set_mnsecs(s, n)	((s)->st_mtim.st__tim.tv_nsec = n)
#define	stat_set_cnsecs(s, n)	((s)->st_ctim.st__tim.tv_nsec = n)

#define	_FOUND_STAT_NSECS_
#endif

#if	defined(HAVE_ST_NSEC) && !defined(_FOUND_STAT_NSECS_)

/*
 * Found e.g. on SunOS-5.x and IRIX.
 */
#define	stat_ansecs(s)		((s)->st_atim.tv_nsec)
#define	stat_mnsecs(s)		((s)->st_mtim.tv_nsec)
#define	stat_cnsecs(s)		((s)->st_ctim.tv_nsec)

#define	stat_set_ansecs(s, n)	((s)->st_atim.tv_nsec = n)
#define	stat_set_mnsecs(s, n)	((s)->st_mtim.tv_nsec = n)
#define	stat_set_cnsecs(s, n)	((s)->st_ctim.tv_nsec = n)

#define	_FOUND_STAT_NSECS_
#endif

#if	defined(HAVE_ST_ATIMESPEC) && !defined(_FOUND_STAT_NSECS_)

/*
 * Found e.g. on FreeBSD and Mac OS X.
 */
#define	stat_ansecs(s)		((s)->st_atimespec.tv_nsec)
#define	stat_mnsecs(s)		((s)->st_mtimespec.tv_nsec)
#define	stat_cnsecs(s)		((s)->st_ctimespec.tv_nsec)

#define	stat_set_ansecs(s, n)	((s)->st_atimespec.tv_nsec = n)
#define	stat_set_mnsecs(s, n)	((s)->st_mtimespec.tv_nsec = n)
#define	stat_set_cnsecs(s, n)	((s)->st_ctimespec.tv_nsec = n)

#define	_FOUND_STAT_NSECS_
#endif

/*
 * st_spare1 is last as some implementations keep backwards
 * compatibility although there is one of the nsec implementations.
 */
#if	defined(HAVE_ST_SPARE1) && !defined(_FOUND_STAT_NSECS_)

/*
 * Found e.g. on SunOS-4.x and HP-UX.
 */
#define	stat_ansecs(s)		((s)->st_spare1 * 1000)
#define	stat_mnsecs(s)		((s)->st_spare2 * 1000)
#define	stat_cnsecs(s)		((s)->st_spare3 * 1000)

#define	stat_set_ansecs(s, n)	((s)->st_spare1 = n / 1000)
#define	stat_set_mnsecs(s, n)	((s)->st_spare2 = n / 1000)
#define	stat_set_cnsecs(s, n)	((s)->st_spare3 = n / 1000)

#define	_FOUND_STAT_USECS_
#define	_FOUND_STAT_NSECS_
#endif

#if	!defined(_FOUND_STAT_NSECS_)
#define	stat_ansecs(s)		(0)
#define	stat_mnsecs(s)		(0)
#define	stat_cnsecs(s)		(0)

#define	stat_set_ansecs(s, n)	(0)
#define	stat_set_mnsecs(s, n)	(0)
#define	stat_set_cnsecs(s, n)	(0)
#endif

#endif	/* _SCHILY_STAT_H */
