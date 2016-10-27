/* @(#)zerobytes.c	1.2 11/07/30 Copyright Copyright 1987, 1995-2011 J. Schilling */
/*
 *	fill memory with null bytes
 *
 *	Copyright (c) 1987, 1995-2011 J. Schilling
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
#include <schily/align.h>
#include <schily/types.h>
#include <schily/schily.h>

#define	DO8(a)	a; a; a; a; a; a; a; a;

/*
 * zero(to, cnt)
 */
EXPORT char *
zerobytes(tov, cnt)
	void	*tov;
	ssize_t	cnt;
{
	register char	*to = (char *)tov;
	register ssize_t n;
	register long	lval = 0L;

	/*
	 * If we change cnt to be unsigned, check for == instead of <=
	 */
	if ((n = cnt) <= 0)
		return (to);

	if (n < 8 * sizeof (long)) {	/* Simple may be faster... */
		do {			/* n is always > 0 */
			*to++ = '\0';
		} while (--n > 0);
		return (to);
	}

	/*
	 * Assign byte-wise until properly aligned for a long pointer.
	 */
	while (--n >= 0 && !laligned(to)) {
		*to++ = '\0';
	}
	n++;

	if (n >= (ssize_t)(8 * sizeof (long))) {
		register ssize_t rem = n % (8 * sizeof (long));

		n /= (8 * sizeof (long));
		{
			register long *tol = (long *)to;

			do {
				DO8 (*tol++ = lval);
			} while (--n > 0);

			to = (char *)tol;
		}
		n = rem;

		if (n >= 8) {
			n -= 8;
			do {
				DO8 (*to++ = '\0');
			} while ((n -= 8) >= 0);
			n += 8;
		}
		if (n > 0) do {
			*to++ = '\0';
		} while (--n > 0);
		return (to);
	}
	if (n > 0) do {
		*to++ = '\0';
	} while (--n > 0);
	return (to);
}
