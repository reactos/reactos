/* @(#)streql.c	1.10 09/06/07 Copyright 1985, 1995-2009 J. Schilling */
/*
 *	Check if two strings are equal
 *
 *	Copyright (c) 1985, 1995-2009 J. Schilling
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
#include <schily/schily.h>

EXPORT int
streql(a, b)
	const char	*a;
	const char	*b;
{
	register const char	*s1 = a;
	register const char	*s2 = b;

	if (s1 == NULL || s2 ==  NULL)
		return (FALSE);

	if (s1 == s2)
		return (TRUE);

	while (*s1 == *s2++)
		if (*s1++ == '\0')
			return (TRUE);

	return (FALSE);
}
