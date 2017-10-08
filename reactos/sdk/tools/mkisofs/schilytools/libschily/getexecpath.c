/* @(#)getexecpath.c	1.1 10/11/18 Copyright 2006-2010 J. Schilling */
/*
 *	Copyright (c) 2006.2010 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/types.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/schily.h>

#if (defined(sun) || defined(__sun) || defined(__sun__)) && defined(__SVR4)
#define	PATH_IMPL
#define	METHOD_SYMLINK
#define	SYMLINK_PATH	"/proc/self/path/a.out"	/* Solaris 10 -> ... */
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#define	PATH_IMPL
#define	METHOD_SYMLINK
#define	SYMLINK_PATH	"/proc/curproc/file"	/* /proc may nor be mounted */
#endif

#if defined(__linux__) || defined(__linux)|| defined(linux)
#define	PATH_IMPL
#define	METHOD_SYMLINK
#define	SYMLINK_PATH	"/proc/self/exe"
#endif

#if defined(HAVE_PROC_PIDPATH)			/* Mac OS X */
#ifdef	HAVE_LIBPROC_H
#include <libproc.h>
#endif
#define	PATH_IMPL
#endif

/*
 * TODO: AIX:	/proc/$$/object/a.out	-> plain file, match via st_dev/st_ino
 */


EXPORT char *
getexecpath()
{
#ifdef	PATH_IMPL
#ifdef	METHOD_SYMLINK
	char	buf[1024];
	ssize_t	len;

	len = readlink(SYMLINK_PATH, buf, sizeof (buf)-1);
	if (len == -1)
		return (NULL);
	buf[len] = '\0';
	return (strdup(buf));
#endif
#ifdef	HAVE_PROC_PIDPATH			/* Mac OS X */
	char	buf[1024];
	int	len;

	len = proc_pidpath(getpid(), buf, sizeof (buf));
	if (len == -1)
		return (NULL);
	return (strdup(buf));
#endif
#else
	return (NULL);
#endif
}
