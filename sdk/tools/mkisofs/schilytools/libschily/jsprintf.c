/* @(#)jsprintf.c	1.20 17/08/03 Copyright 1985, 1995-2017 J. Schilling */
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
#include <schily/stdio.h>
#include <schily/types.h>
#include <schily/varargs.h>
#include <schily/standard.h>
#include <schily/schily.h>

#ifdef	NO_FPRFORMAT
#undef	USE_FPRFORMAT
#else
#define	USE_FPRFORMAT
#endif

#ifdef	USE_FPRFORMAT
/*
 * This is the speed-optimized version that currently only has been tested
 * on Solaris.
 * It is based on fprformat() instead of format() and is faster than the
 * the format() based standard implementation, in case that putc() or
 * putc_unlocked() is a macro.
 */

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT int
js_printf(const char *form, ...)
#else
EXPORT int
js_printf(form, va_alist)
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	ret = fprformat(stdout, form, args);
	va_end(args);
	return (ret);
}

/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT int
js_fprintf(FILE *file, const char *form, ...)
#else
EXPORT int
js_fprintf(file, form, va_alist)
	FILE	*file;
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	ret = fprformat(file, form, args);
	va_end(args);
	return (ret);
}

#else	/* !USE_FPRFORMAT */
/*
 * This is the portable standard implementation that works anywhere.
 */

#define	BFSIZ	256

typedef struct {
	short	cnt;
	char	*ptr;
	char	buf[BFSIZ];
	int	count;
	FILE	*f;
} *BUF, _BUF;

LOCAL	void	_bflush		__PR((BUF));
LOCAL	void	_bput		__PR((char, void *));
EXPORT	int	js_fprintf	__PR((FILE *, const char *, ...));
EXPORT	int	js_printf	__PR((const char *, ...));

LOCAL void
_bflush(bp)
	register BUF	bp;
{
	bp->count += bp->ptr - bp->buf;
	if (filewrite(bp->f, bp->buf, bp->ptr - bp->buf) < 0)
		bp->count = EOF;
	bp->ptr = bp->buf;
	bp->cnt = BFSIZ;
}

#ifdef	PROTOTYPES
LOCAL void
_bput(char c, void *l)
#else
LOCAL void
_bput(c, l)
		char	c;
		void	*l;
#endif
{
	register BUF	bp = (BUF)l;

	*bp->ptr++ = c;
	if (--bp->cnt <= 0)
		_bflush(bp);
}

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT int
js_printf(const char *form, ...)
#else
EXPORT int
js_printf(form, va_alist)
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	_BUF	bb;

	bb.ptr = bb.buf;
	bb.cnt = BFSIZ;
	bb.count = 0;
	bb.f = stdout;
#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	format(_bput, &bb, form, args);
	va_end(args);
	if (bb.cnt < BFSIZ)
		_bflush(&bb);
	return (bb.count);
}

/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT int
js_fprintf(FILE *file, const char *form, ...)
#else
EXPORT int
js_fprintf(file, form, va_alist)
	FILE	*file;
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	_BUF	bb;

	bb.ptr = bb.buf;
	bb.cnt = BFSIZ;
	bb.count = 0;
	bb.f = file;
#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	format(_bput, &bb, form, args);
	va_end(args);
	if (bb.cnt < BFSIZ)
		_bflush(&bb);
	return (bb.count);
}
#endif
