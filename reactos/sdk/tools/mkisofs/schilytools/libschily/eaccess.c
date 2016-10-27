/* @(#)eaccess.c	1.6 14/05/15 Copyright 2004-2014 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)eaccess.c	1.6 14/05/15 Copyright 2004-2014 J. Schilling";
#endif
/*
 * Implement the best possible emulation for eaccess()
 *
 * Copyright 2004-2014 J. Schilling
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

#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/errno.h>
#include <schily/schily.h>

#ifndef	HAVE_EACCESS
EXPORT	int	eaccess		__PR((const char *name, int mode));

EXPORT int
eaccess(name, mode)
	const	char	*name;
		int	mode;
{
#ifdef	HAVE_EUIDACCESS
	return (euidaccess(name, mode));
#else
#ifdef	HAVE_ACCESS_E_OK
	return (access(name, E_OK|mode));
#else
	if (getuid() == geteuid() && getgid() == getegid())
		return (access(name, mode));
#ifdef	EOPNOTSUPP
	seterrno(EOPNOTSUPP);
#else
	seterrno(EINVAL);
#endif
	return (-1);
#endif
#endif
}
#endif
