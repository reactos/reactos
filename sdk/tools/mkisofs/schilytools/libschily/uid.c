/* @(#)uid.c	1.1 11/07/11 Copyright 2011 J. Schilling */
/*
 *	Dummy functions for uid handling, used e.g. on MINGW
 *
 *	Copyright (c) 2011 J. Schilling
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

#include <schily/types.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/schily.h>

#ifndef	HAVE_GETUID

EXPORT uid_t
getuid()
{
	return (0);
}

#endif

#ifndef	HAVE_GETEUID

EXPORT uid_t
geteuid()
{
	return (0);
}

#endif

#ifndef	HAVE_SETUID

EXPORT int
setuid(uid)
	uid_t	uid;
{
	return (0);
}

#endif

#ifndef	HAVE_SETEUID

EXPORT int
seteuid(uid)
	uid_t	uid;
{
	return (0);
}

#endif
