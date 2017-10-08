/* @(#)geterrno.c	1.14 10/08/23 Copyright 1985, 1995-2010 J. Schilling */
/*
 *	Get error number
 *
 *	Copyright (c) 1985, 1995-2010 J. Schilling
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

#ifndef	_TS_ERRNO
#define	_TS_ERRNO		/* Solaris: get thread safe errno value */
#endif
#ifndef	_LIBC_REENTRANT
#define	_LIBC_REENTRANT		/* Linux: get thread safe errno value */
#endif
#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/schily.h>

#ifdef	geterrno
#undef	geterrno
#endif

EXPORT int
geterrno()

{
	return (errno);
}
