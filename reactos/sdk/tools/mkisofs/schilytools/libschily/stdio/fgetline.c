/* @(#)fgetline.c	1.13 14/03/27 Copyright 1986, 1996-2014 J. Schilling */
/*
 *	Copyright (c) 1986, 1996-2014 J. Schilling
 *
 *	This is an interface that exists in the public since 1982.
 *	The POSIX.1-2008 standard did ignore POSIX rules not to
 *	redefine existing public interfaces and redefined the interfaces
 *	forcing us to add a js_*() prefix to the original functions.
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

#define	fgetline	__no__fgetline__
#define	getline		__no__getline__

#include "schilyio.h"

#ifndef	NO_GETLINE_COMPAT	/* Define to disable backward compatibility */
#undef	fgetline
#undef	getline
#ifdef	HAVE_PRAGMA_WEAK
#pragma	weak fgetline =	js_fgetline
#pragma	weak getline =	js_getline
#else

EXPORT	int	fgetline	__PR((FILE *, char *, int));
EXPORT 	int	getline		__PR((char *, int));

EXPORT int
fgetline(f, buf, len)
	FILE	*f;
	char	*buf;
	int	len;
{
	return (js_fgetline(f, buf, len));
}

EXPORT int
getline(buf, len)
	char	*buf;
	int	len;
{
	return (js_fgetline(stdin, buf, len));
}
#endif
#endif

/*
 * XXX should we check if HAVE_USG_STDIO is defined and
 * XXX use something line memccpy to speed things up ???
 * XXX On Solaris 64 bits, we may use #define FAST_GETC_PUTC
 * XXX and getc_unlocked()
 */
#if !defined(getc) && defined(USE_FGETS_FOR_FGETLINE)
#include <schily/string.h>

EXPORT int
js_fgetline(f, buf, len)
	register	FILE	*f;
			char	*buf;
	register	int	len;
{
	char	*bp = fgets(buf, len, f);

	if (bp) {
		len = strlen(bp);

		if (len > 0) {
			if (bp[len-1] == '\n')
				bp[--len] = '\0';
		}
		return (len);
	}
	buf[0] = '\0';
	return (-1);
}

#else
EXPORT int
js_fgetline(f, buf, len)
	register	FILE	*f;
			char	*buf;
	register	int	len;
{
	register int	c	= '\0';
	register char	*bp	= buf;
	register int	nl	= '\n';

	down2(f, _IOREAD, _IORW);

	for (;;) {
		if ((c = getc(f)) < 0)
			break;
		if (c == nl)
			break;
		if (--len > 0) {
			*bp++ = (char)c;
		} else {
#ifdef	__never__
			/*
			 * Read up to end of line
			 */
			while ((c = getc(f)) >= 0 && c != nl)
				/* LINTED */
				;
#endif
			break;
		}
	}
	*bp = '\0';
	/*
	 * If buffer is empty and we hit EOF, return EOF
	 */
	if (c < 0 && bp == buf)
		return (c);

	return (bp - buf);
}
#endif

EXPORT int
js_getline(buf, len)
	char	*buf;
	int	len;
{
	return (js_fgetline(stdin, buf, len));
}
