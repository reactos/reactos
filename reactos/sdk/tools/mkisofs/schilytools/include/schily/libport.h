/* @(#)libport.h	1.45 17/05/02 Copyright 1995-2017 J. Schilling */
/*
 *	Prototypes for POSIX standard functions that may be missing on the
 *	local platform and thus are implemented inside libschily.
 *
 *	Copyright (c) 1995-2017 J. Schilling
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


#ifndef _SCHILY_LIBPORT_H
#define	_SCHILY_LIBPORT_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_TYPES_H
#include <schily/types.h>
#endif

#ifndef _SCHILY_UNISTD_H
#include <schily/unistd.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#if	defined(_INCL_SYS_TYPES_H) || defined(_INCL_TYPES_H) || defined(size_t)
#	ifndef	FOUND_SIZE_T
#	define	FOUND_SIZE_T
#	endif
#endif
#if	defined(_MSC_VER) && !defined(_SIZE_T_DEFINED)
#	undef	FOUND_SIZE_T
#endif

#ifdef	OPENSERVER
/*
 * Don't use the usleep() from libc on SCO's OPENSERVER.
 * It will kill our processes with SIGALRM.
 */
/*
 * Don't #undef HAVE_USLEEP in this file, SCO has a
 * usleep() prototype in unistd.h
 */
/* #undef	HAVE_USLEEP */
#endif

#ifdef	FOUND_SIZE_T
/*
 * We currently cannot define this here because there IRIX has a definition
 * than violates the standard.
 */
#ifndef	HAVE_SNPRINTF
/*PRINTFLIKE3*/
extern	int	snprintf __PR((char *, size_t, const char *, ...))
					__printflike__(3, 4);
#endif
#endif

#ifdef	EOF		/* stdio.h has been included */
#ifdef	FOUND_SIZE_T
#ifndef	HAVE_GETDELIM
extern	ssize_t	getdelim	__PR((char **, size_t *, int, FILE *));
#endif
#endif
#endif

#ifndef	HAVE_GETHOSTID
extern	long		gethostid	__PR((void));
#endif
#ifndef	HAVE_GETPAGESIZE
extern	int		getpagesize	__PR((void));
#endif
#ifndef	HAVE_USLEEP
extern	int		usleep		__PR((int usec));
#endif

#ifndef	HAVE_STRCASECMP
extern	int		strcasecmp	__PR((const char *, const char *));
#endif
#ifdef	FOUND_SIZE_T
#ifndef	HAVE_STRNCASECMP
extern 	int		strncasecmp	__PR((const char *, const char *,
						size_t));
#endif
#endif

#ifndef	HAVE_STRCAT
extern	char		*strcat		__PR((char *s1, const char *s2));
#endif
#ifndef	HAVE_STRCHR
extern	char		*strchr		__PR((const char *s1, int c));
#endif
#ifndef	HAVE_STRCMP
extern	int		strcmp		__PR((const char *s1, const char *s2));
#endif
#ifndef	HAVE_STRCPY
extern	char		*strcpy		__PR((char *s1, const char *s2));
#endif
#if	!defined(HAVE_STRDUP) || defined(__SVR4)
extern	char		*strdup		__PR((const char *s));
#endif
#ifdef	FOUND_SIZE_T
#ifndef	HAVE_STRNDUP
extern	char		*strndup	__PR((const char *s, size_t len));
#endif
#ifndef	HAVE_STRLEN
extern	size_t		strlen		__PR((const char *s));
#endif
#ifndef	HAVE_STRNLEN
extern	size_t		strnlen		__PR((const char *s, size_t len));
#endif
#ifndef	HAVE_STRLCAT
extern	size_t		strlcat		__PR((char *s1, const char *s2,
							size_t len));
#endif
#ifndef	HAVE_STRLCPY
extern	size_t		strlcpy		__PR((char *s1, const char *s2,
							size_t len));
#endif
#ifndef	HAVE_STRNCAT
extern	char		*strncat	__PR((char *s1, const char *s2,
							size_t len));
#endif
#ifndef	HAVE_STRNCMP
extern	int		strncmp		__PR((const char *s1, const char *s2,
							size_t len));
#endif
#ifndef	HAVE_STRNCPY
extern	char		*strncpy	__PR((char *s1, const char *s2,
							size_t len));
#endif
#endif	/* FOUND_SIZE_T */
#ifndef	HAVE_STRRCHR
extern	char		*strrchr	__PR((const char *s1, int c));
#endif

#ifndef	HAVE_STRSTR
extern	char		*strstr		__PR((const char *s1, const char *s2));
#endif

#ifdef	_SCHILY_WCHAR_H
#ifndef	HAVE_WCSCAT
extern	wchar_t		*wcscat		__PR((wchar_t *s1, const wchar_t *s2));
#endif
#ifndef	HAVE_WCSCHR
extern	wchar_t		*wcschr		__PR((const wchar_t *s1, wchar_t c));
#endif
#ifndef	HAVE_WCSCMP
extern	int		wcscmp		__PR((const wchar_t *s1,
							const wchar_t *s2));
#endif
#ifndef	HAVE_WCSCPY
extern	wchar_t		*wcscpy		__PR((wchar_t *s1, const wchar_t *s2));
#endif
#ifndef	HAVE_WCSDUP
extern	wchar_t		*wcsdup		__PR((const wchar_t *s));
#endif
#ifdef	FOUND_SIZE_T
#ifndef	HAVE_WCSNDUP
extern	wchar_t		*wcsndup	__PR((const wchar_t *s, size_t len));
#endif
#ifndef	HAVE_WCSLEN
extern	size_t		wcslen		__PR((const wchar_t *s));
#endif
#ifndef	HAVE_WCSNLEN
extern	size_t		wcsnlen		__PR((const wchar_t *s, size_t len));
#endif
#ifndef	HAVE_WCSLCAT
extern	size_t		wcslcat		__PR((wchar_t *s1, const wchar_t *s2,
							size_t len));
#endif
#ifndef	HAVE_WCSLCPY
extern	size_t		wcslcpy		__PR((wchar_t *s1, const wchar_t *s2,
							size_t len));
#endif
#ifndef	HAVE_WCSNCAT
extern	wchar_t		*wcsncat	__PR((wchar_t *s1, const wchar_t *s2,
							size_t len));
#endif
#ifndef	HAVE_WCSNCMP
extern	int		wcsncmp		__PR((const wchar_t *s1,
							const wchar_t *s2,
							size_t len));
#endif
#ifndef	HAVE_WCSNCPY
extern	wchar_t		*wcsncpy	__PR((wchar_t *s1, const wchar_t *s2,
							size_t len));
#endif
#endif	/* FOUND_SIZE_T */
#ifndef	HAVE_WCSRCHR
extern	wchar_t		*wcsrchr	__PR((const wchar_t *s1, wchar_t c));
#endif

#ifndef	HAVE_WCSSTR
extern	wchar_t		*wcsstr		__PR((const wchar_t *s1,
							const wchar_t *s2));
#endif
#endif	/* _SCHILY_WCHAR_H */

#ifndef	HAVE_RENAME
extern	int		rename		__PR((const char *__old,
							const char *__new));
#endif

/*
 * XXX Note: libgen.h / -lgen on Solaris contain eaccess()
 */
#ifndef	HAVE_EACCESS
extern	int		eaccess		__PR((const char *name, int mode));
#endif

/*
 * See also libgen.h
 */
#ifndef	HAVE_BASENAME
extern	char		*basename	__PR((char *name));
#endif
#ifndef	HAVE_DIRNAME
extern	char		*dirname	__PR((char *name));
#endif

#ifndef	HAVE_TIMEGM
#if	defined(_SCHILY_TIME_H)
extern	time_t		timegm		__PR((struct tm *));
#endif
#endif

#ifndef	HAVE_GETUID
extern	uid_t	getuid	__PR((void));
#endif
#ifndef	HAVE_GETEUID
extern	uid_t	geteuid	__PR((void));
#endif
#ifndef	HAVE_SETUID
extern	int	setuid	__PR((uid_t uid));
#endif
#ifndef	HAVE_SETEUID
extern	int	seteuid	__PR((uid_t uid));
#endif

#ifndef	HAVE_GETGID
extern	gid_t	getgid	__PR((void));
#endif
#ifndef	HAVE_GETEGID
extern	gid_t	getegid	__PR((void));
#endif
#ifndef	HAVE_SETGID
extern	int	setgid	__PR((gid_t gid));
#endif
#ifndef	HAVE_SETEGID
extern	int	setegid	__PR((gid_t gid));
#endif

#ifndef	HAVE_GETPWNAM
extern	struct passwd *getpwnam __PR((const char *name));
#endif
#ifndef	HAVE_GETPWENT
extern	struct passwd *getpwent __PR((void));
#endif
#ifndef	HAVE_GETPWUID
extern	struct passwd *getpwuid __PR((uid_t uid));
#endif
#ifndef	HAVE_SETPWENT
extern	void		setpwent __PR((void));
#endif
#ifndef	HAVE_ENDPWENT
extern	void		endpwent __PR((void));
#endif


#ifndef	HAVE_GETGRNAM
extern	struct group	*getgrnam __PR((const char *name));
#endif
#ifndef	HAVE_GETGRENT
extern	struct group	*getgrent __PR((void));
#endif
#ifndef	HAVE_GETGRGID
extern	struct group	*getgrgid __PR((gid_t gid));
#endif
#ifndef	HAVE_SETGRENT
extern	void		setgrent __PR((void));
#endif
#ifndef	HAVE_ENDGRENT
extern	void		endgrent __PR((void));
#endif

#ifndef	HAVE_FCHDIR
extern	int		fchdir __PR((int fd));
#endif
#ifndef	HAVE_OPENAT
extern	int		openat __PR((int fd, const char *name, int oflag, ...));
#endif


#ifndef	HAVE_GETTIMEOFDAY
#ifdef	_SCHILY_TIME_H
extern	int		gettimeofday __PR((struct timeval *__tp, void *__tzp));
#endif
#endif

#ifndef	HAVE_FACCESSAT
extern	int		faccessat __PR((int fd, const char *name,
					int amode, int flag));
#endif
#ifndef	HAVE_FCHMODAT
extern	int		fchmodat __PR((int fd, const char *name,
					mode_t mode, int flag));
#endif
#ifndef	HAVE_LCHMOD
extern	int		lchmod __PR((const char *name, mode_t mode));
#endif

#ifndef	HAVE_FCHOWNAT
extern	int		fchownat __PR((int fd, const char *name,
					uid_t owner, gid_t group, int flag));
#endif

#ifndef	HAVE_FDOPENDIR
#ifdef _SCHILY_DIRENT_H
extern	DIR		*fdopendir __PR((int fd));
#endif
#endif

#ifdef	_SCHILY_STAT_H
#ifndef	HAVE_FSTATAT
extern	int		fstatat __PR((int fd, const char *name,
					struct stat *sbuf, int flag));
#endif
#endif	/* _SCHILY_STAT_H */
#ifdef	_SCHILY_TIME_H
#ifndef	HAVE_FUTIMENS
extern	int		futimens __PR((int fd,
					const struct timespec __times[2]));
#endif
#ifndef	HAVE_FUTIMESAT
extern	int		futimesat __PR((int fd, const char *name,
					const struct timeval __times[2]));
#endif
#ifndef	HAVE_LUTIMENS
extern	int		lutimens __PR((const char *name,
					const struct timespec __times[2]));
#endif
#endif	/* _SCHILY_TIME_H */
#ifndef	HAVE_LINKAT
extern	int		linkat __PR((int fd1, const char *name1,
					int fd2, const char *name2, int flag));
#endif
#ifndef	HAVE_MKDIRAT
extern	int		mkdirat __PR((int fd, const char *name, mode_t mode));
#endif
#ifndef	HAVE_MKFIFO
extern	int		mkfifo __PR((const char *name, mode_t mode));
#endif
#ifndef	HAVE_MKFIFOAT
extern	int		mkfifoat __PR((int fd, const char *name, mode_t mode));
#endif
#ifndef	HAVE_MKNODAT
extern	int		mknodat __PR((int fd, const char *name,
					mode_t mode, dev_t dev));
#endif
#ifndef	HAVE_READLINKAT
extern	ssize_t		readlinkat __PR((int fd, const char *name,
					char *lbuf, size_t lbufsize));
#endif
#ifndef	HAVE_RENAMEAT
extern	int		renameat __PR((int oldfd, const char *__old,
					int newfd, const char *__new));
#endif
#ifndef	HAVE_SYMLINKAT
extern	int		symlinkat __PR((const char *content,
					int fd, const char *name));
#endif
#ifndef	HAVE_UNLINKAT
extern	int		unlinkat __PR((int fd, const char *name, int flag));
#endif
#ifdef	_SCHILY_TIME_H
#ifndef	HAVE_UTIMENS
extern	int		utimens __PR((const char *name,
					const struct timespec __times[2]));
#endif
#ifndef	HAVE_UTIMENSAT
extern	int		utimensat __PR((int fd, const char *name,
					const struct timespec __times[2],
					int flag));
#endif
#endif	/* _SCHILY_TIME_H */

#ifdef	__SUNOS4
/*
 * Define prototypes for POSIX standard functions that are missing on SunOS-4.x
 * to make compilation smooth.
 */
#include <schily/sunos4_proto.h>

#endif	/* __SUNOS4 */

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_LIBPORT_H */
