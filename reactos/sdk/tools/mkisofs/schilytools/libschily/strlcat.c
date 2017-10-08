/* @(#)strlcat.c	1.1 10/04/26 Copyright 2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)strlcat.c	1.1 10/04/26 Copyright 2010 J. Schilling";
#endif
/*
 *	strlcat() to be used if missing in libc
 *
 *	Copyright (c) 2010 J. Schilling
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

#include <schily/standard.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/libport.h>

#ifndef	HAVE_STRLCAT

EXPORT size_t
strlcat(s1, s2, len)
	register char		*s1;
	register const char	*s2;
	register size_t		len;
{
	const char		*os1	= s1;
		size_t		olen	= len;

	if (len > 0) {
		while (--len > 0 && *s1++ != '\0')
			;

		if (len == 0)
			return (olen + strlen(s2));

		s1--;
		len++;
		while (--len > 0 && (*s1++ = *s2++) != '\0')
			;
		if (len == 0) {
			*s1 = '\0';
			return (s1 - os1 + strlen(s2));
		}
		return (--s1 - os1);
	}
	return (strlen(s2));
}
#endif	/* HAVE_STRLCAT */
