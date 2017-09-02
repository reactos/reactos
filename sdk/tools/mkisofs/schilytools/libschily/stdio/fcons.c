/* @(#)fcons.c	2.20 10/11/06 Copyright 1986, 1995-2010 J. Schilling */
/*
 *	Copyright (c) 1986, 1995-2010  J. Schilling
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

#include "schilyio.h"

/*
 * Note that because of a definition in schilyio.h we are using
 * fseeko()/ftello() instead of fseek()/ftell() if available.
 */

LOCAL	char	*fmtab[] = {
			"",	/* 0	FI_NONE				*/
			"r",	/* 1	FI_READ				*/
			"w",	/* 2	FI_WRITE		**1)	*/
			"r+",	/* 3	FI_READ  | FI_WRITE		*/
			"b",	/* 4	FI_NONE  | FI_BINARY		*/
			"rb",	/* 5	FI_READ  | FI_BINARY		*/
			"wb",	/* 6	FI_WRITE | FI_BINARY	**1)	*/
			"r+b",	/* 7	FI_READ  | FI_WRITE | FI_BINARY	*/

/* + FI_APPEND	*/	"",	/* 0	FI_NONE				*/
/* ...		*/	"r",	/* 1	FI_READ				*/
			"a",	/* 2	FI_WRITE		**1)	*/
			"a+",	/* 3	FI_READ  | FI_WRITE		*/
			"b",	/* 4	FI_NONE  | FI_BINARY		*/
			"rb",	/* 5	FI_READ  | FI_BINARY		*/
			"ab",	/* 6	FI_WRITE | FI_BINARY	**1)	*/
			"a+b",	/* 7	FI_READ  | FI_WRITE | FI_BINARY	*/
		};
/*
 * NOTES:
 *	1)	fdopen() guarantees not to create/trunc files in this case
 *
 *	"w"	will create/trunc files with fopen()
 *	"a"	will create files with fopen()
 */


EXPORT FILE *
_fcons(fd, f, flag)
	register FILE	*fd;
		int	f;
		int	flag;
{
	int	my_gflag = _io_glflag;

	if (fd == (FILE *)NULL)
		fd = fdopen(f,
			fmtab[flag&(FI_READ|FI_WRITE|FI_BINARY | FI_APPEND)]);

	if (fd != (FILE *)NULL) {
		if (flag & FI_APPEND) {
			(void) fseek(fd, (off_t)0, SEEK_END);
		}
		if (flag & FI_UNBUF) {
			setbuf(fd, NULL);
			my_gflag |= _JS_IOUNBUF;
		}
		set_my_flag(fd, my_gflag); /* must clear it if fd is reused */
		return (fd);
	}
	if (flag & FI_CLOSE)
		close(f);

	return ((FILE *)NULL);
}
