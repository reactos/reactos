/* @(#)fileopen.c	1.12 10/08/21 Copyright 1986, 1995-2010 J. Schilling */
/*
 *	Copyright (c) 1986, 1995-2010 J. Schilling
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

EXPORT FILE *
fileopen(name, mode)
	const char	*name;
	const char	*mode;
{
	int	ret;
	int	omode = 0;
	int	flag = 0;

	if (!_cvmod(mode, &omode, &flag))
		return ((FILE *)NULL);

	if ((ret = _openfd(name, omode)) < 0)
		return ((FILE *)NULL);

	return (_fcons((FILE *)0, ret, flag | FI_CLOSE));
}
