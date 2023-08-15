/* @(#)jssnprintf.c	1.14 17/08/03 Copyright 1985, 1995-2017 J. Schilling */
/*
 *	Copyright (c) 1985, 1995-2017 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/unistd.h>	/* include <sys/types.h> try to get size_t */
#include <schily/stdio.h>	/* Try again for size_t	*/
#include <schily/stdlib.h>	/* Try again for size_t	*/
#include <schily/varargs.h>
#include <schily/standard.h>
#include <schily/schily.h>

EXPORT	int js_snprintf __PR((char *, size_t maxcnt, const char *, ...));

typedef struct {
	char	*ptr;
	int	count;
} *BUF, _BUF;

static void
_cput(char c, void *l)
{
	register BUF	bp = (BUF)l;

	if (--bp->count > 0) {
		*bp->ptr++ = c;
	} else {
		/*
		 * Make sure that there will never be a negative overflow.
		 */
		bp->count++;
	}
}

/* VARARGS2 */
EXPORT int
js_snprintf(char *buf, size_t maxcnt, const char *form, ...)
{
	va_list	args;
	int	cnt;
	_BUF	bb;

	bb.ptr = buf;
	bb.count = maxcnt;

	va_start(args, form);
	cnt = format(_cput, &bb, form,  args);
	va_end(args);
	if (maxcnt > 0)
		*(bb.ptr) = '\0';
	if (bb.count < 0)
		return (-1);

	return (cnt);
}
