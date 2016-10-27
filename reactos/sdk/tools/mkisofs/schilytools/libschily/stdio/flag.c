/* @(#)flag.c	2.14 10/11/06 Copyright 1986-2010 J. Schilling */
/*
 *	Copyright (c) 1986-2010 J. Schilling
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
#include <schily/stdlib.h>

#ifdef	DO_MYFLAG

#define	FL_INIT	10

#if	defined(IS_MACOS_X)
/*
 * The MAC OS X linker does not grok "common" varaibles.
 * Make _io_glflag a "data" variable.
 */
EXPORT	int	_io_glflag = 0;		/* global default flag */
#else
EXPORT	int	_io_glflag;		/* global default flag */
#endif
LOCAL	int	_fl_inc = 10;		/* increment to expand flag struct */
EXPORT	int	_fl_max = FL_INIT;	/* max fd currently in _io_myfl */
LOCAL	_io_fl	_io_smyfl[FL_INIT];	/* initial static space */
EXPORT	_io_fl	*_io_myfl = _io_smyfl;	/* init to static space */

LOCAL int _more_flags	__PR((FILE *));

LOCAL int
_more_flags(fp)
	FILE	*fp;
{
	register int	f = fileno(fp);
	register int	n = _fl_max;
	register _io_fl	*np;

	while (n <= f)
		n += _fl_inc;

	if (_io_myfl == _io_smyfl) {
		np = (_io_fl *) malloc(n * sizeof (*np));
		fillbytes(np, n * sizeof (*np), '\0');
		movebytes(_io_smyfl, np, sizeof (_io_smyfl)/sizeof (*np));
	} else {
		np = (_io_fl *) realloc(_io_myfl, n * sizeof (*np));
		if (np)
			fillbytes(&np[_fl_max], (n-_fl_max)*sizeof (*np), '\0');
	}
	if (np) {
		_io_myfl = np;
		_fl_max = n;
		return (_io_get_my_flag(fp));
	} else {
		return (_JS_IONORAISE);
	}
}

EXPORT int
_io_get_my_flag(fp)
	register FILE	*fp;
{
	register int	f = fileno(fp);
	register _io_fl	*fl;

	if (f >= _fl_max)
		return (_more_flags(fp));

	fl = &_io_myfl[f];

	if (fl->fl_io == 0 || fl->fl_io == fp)
		return (fl->fl_flags);

	while (fl && fl->fl_io != fp)
		fl = fl->fl_next;

	if (fl == 0)
		return (0);

	return (fl->fl_flags);
}

EXPORT void
_io_set_my_flag(fp, flag)
	FILE	*fp;
	int	flag;
{
	register int	f = fileno(fp);
	register _io_fl	*fl;
	register _io_fl	*fl2;

	if (f >= _fl_max)
		(void) _more_flags(fp);

	fl = &_io_myfl[f];

	if (fl->fl_io != (FILE *)0) {
		fl2 = fl;

		while (fl && fl->fl_io != fp)
			fl = fl->fl_next;
		if (fl == 0) {
			if ((fl = (_io_fl *) malloc(sizeof (*fl))) == 0)
				return;
			fl->fl_next = fl2->fl_next;
			fl2->fl_next = fl;
		}
	}
	fl->fl_io = fp;
	fl->fl_flags = flag;
}

EXPORT void
_io_add_my_flag(fp, flag)
	FILE	*fp;
	int	flag;
{
	int	oflag = _io_get_my_flag(fp);

	oflag |= flag;

	_io_set_my_flag(fp, oflag);
}

#endif	/* DO_MYFLAG */
