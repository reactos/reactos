/* @(#)mem.c	1.11 15/05/10 Copyright 1998-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)mem.c	1.11 15/05/10 Copyright 1998-2015 J. Schilling";
#endif
/*
 *	Memory handling with error checking
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

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

EXPORT	int	___mexval	__PR((int exval));
EXPORT	void	*___malloc	__PR((size_t size, char *msg));
EXPORT	void	*___realloc	__PR((void *ptr, size_t size, char *msg));
EXPORT	char	*___savestr	__PR((const char *s));

LOCAL	int	mexval;

EXPORT	int
___mexval(exval)
	int	exval;
{
	int	ret = mexval;

	mexval = exval;

	return (ret);
}

EXPORT void *
___malloc(size, msg)
	size_t	size;
	char	*msg;
{
	void	*ret;

	ret = malloc(size);
	if (ret == NULL) {
		int	err = geterrno();

		errmsg(gettext("Cannot allocate memory for %s.\n"), msg);
		if (mexval)
			err = mexval;
		comexit(err);
		/* NOTREACHED */
	}
	return (ret);
}

EXPORT void *
___realloc(ptr, size, msg)
	void	*ptr;
	size_t	size;
	char	*msg;
{
	void	*ret;

	if (ptr == NULL)
		ret = malloc(size);
	else
		ret = realloc(ptr, size);
	if (ret == NULL) {
		int	err = geterrno();

		errmsg(gettext("Cannot realloc memory for %s.\n"), msg);
		if (mexval)
			err = mexval;
		comexit(err);
		/* NOTREACHED */
	}
	return (ret);
}

EXPORT char *
___savestr(s)
	const char	*s;
{
	char	*ret = ___malloc(strlen(s)+1, "saved string");

	strcpy(ret, s);
	return (ret);
}
