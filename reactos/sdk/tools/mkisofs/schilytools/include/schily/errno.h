/* @(#)errno.h	1.7 10/08/24 Copyright 2006-2010 J. Schilling */
/*
 *	Error number related definitions
 *
 *	Copyright (c) 2006-2010 J. Schilling
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

#ifndef _SCHILY_ERRNO_H
#define	_SCHILY_ERRNO_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	JOS
#include <error.h>

#define	ENOEXEC		EBADHEADER
#define	EACCES		EACCESS
#define	ENOENT		ENOFILE
#define	EEXIST		EEXISTS
#else
#include <errno.h>

#define	EMISSDIR	ENOENT
#define	ENDOFFILE	EFBIG
#endif

#ifndef	HAVE_ERRNO_DEF
extern	int	errno;
#endif

#ifndef	HAVE_STRERROR
extern	char	*sys_errlist[];
extern	int	sys_nerr;
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef	seterrno
extern	int	seterrno __PR((int));
#endif
extern	int	geterrno __PR((void));

#ifdef	__cplusplus
}
#endif

#endif /* _SCHILY_ERRNO_H */
