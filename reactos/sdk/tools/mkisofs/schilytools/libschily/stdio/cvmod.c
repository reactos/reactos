/* @(#)cvmod.c	2.9 04/08/08 Copyright 1986, 1995-2003 J. Schilling */
/*
 *	Copyright (c) 1986, 1995-2003 J. Schilling
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

#ifndef	O_BINARY
#define	O_BINARY	0
#endif
#ifndef	O_LARGEFILE
#define	O_LARGEFILE	0
#endif

EXPORT int
_cvmod(mode, omode, flag)
	const char	*mode;
	int		*omode;
	int		*flag;
{
	while (*mode) {
		switch (*mode) {

		case 'r':   *omode |= O_RDONLY;	*flag |= FI_READ;	break;
		case 'w':   *omode |= O_WRONLY;	*flag |= FI_WRITE;	break;
		case 'e':   *omode |= O_EXCL;				break;
		case 'c':   *omode |= O_CREAT;	*flag |= FI_CREATE;	break;
		case 't':   *omode |= O_TRUNC;	*flag |= FI_TRUNC;	break;
		case 'a':   *omode |= O_APPEND;	*flag |= FI_APPEND;	break;
		case 'u':			*flag |= FI_UNBUF;	break;
			/* dummy on UNIX */
		case 'b':   *omode |= O_BINARY; *flag |= FI_BINARY;	break;
			/*
			 * XXX do we need this ?
			 * XXX May this be a problem?
			 */
		case 'l':   *omode |= O_LARGEFILE;			break;
		default:    raisecond(_badmode, 0L);
			    return (0);
		}
		mode++;
	}
	if (*flag & FI_READ && *flag & FI_WRITE) {
		*omode &= ~(O_RDONLY|O_WRONLY);
		*omode |= O_RDWR;
	}
	return (1);
}
