/* @(#)niwrite.c	1.7 09/06/30 Copyright 1986, 2001-2009 J. Schilling */
/*
 *	Non interruptable write
 *
 *	Copyright (c) 1986, 2001-2009 J. Schilling
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
_niwrite(f, buf, count)
	int	f;
	void	*buf;
	size_t	count;
{
	ssize_t	ret;
	int	oerrno = geterrno();

	if ((ret = (ssize_t)count) < 0) {
		seterrno(EINVAL);
		return ((ssize_t)-1);
	}
	while ((ret = write(f, buf, count)) < 0 && geterrno() == EINTR) {
		/*
		 * Set back old 'errno' so we don't change the errno visible
		 * to the outside if we did not fail.
		 */
		seterrno(oerrno);
	}
	return (ret);
}
