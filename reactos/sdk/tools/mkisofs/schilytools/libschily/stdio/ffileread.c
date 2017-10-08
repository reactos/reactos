/* @(#)ffileread.c	1.13 09/06/30 Copyright 1986, 1996-2009 J. Schilling */
/*
 *	Copyright (c) 1986, 1996-2009 J. Schilling
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

EXPORT ssize_t
ffileread(f, buf, len)
	register FILE	*f;
	void	*buf;
	size_t	len;
{
	register int		fd;
	register ssize_t	ret;
		int		oerrno = geterrno();

	down2(f, _IOREAD, _IORW);
	fd = fileno(f);

	while ((ret = read(fd, buf, len)) < 0 && geterrno() == EINTR) {
		/*
		 * Set back old 'errno' so we don't change the errno visible
		 * to the outside if we did not fail.
		 */
		seterrno(oerrno);
	}
	return (ret);
}
