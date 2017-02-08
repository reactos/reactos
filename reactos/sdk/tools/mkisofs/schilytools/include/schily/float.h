/* @(#)float.h	1.1 09/08/08 Copyright 2009 J. Schilling */
/*
 *	Abstraction code for float.h
 *
 *	Copyright (c) 2009 J. Schilling
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

#ifndef	_SCHILY_FLOAT_H
#define	_SCHILY_FLOAT_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_FLOAT_H
#ifndef	_INCL_FLOAT_H
#define	_INCL_FLOAT_H
#include <float.h>
#endif
#endif

#ifdef	HAVE_VALUES_H
#ifndef	_INCL_VALUES_H
#define	_INCL_VALUES_H
#include <values.h>
#endif

#ifndef	FLT_MAX
#define	FLT_MAX	MAXFLOAT
#endif
#ifndef	FLT_MIN
#define	FLT_MIN	MINFLOAT
#endif
#ifndef	DBL_MAX
#define	DBL_MAX	MAXDOUBLE
#endif
#ifndef	DBL_MIN
#define	DBL_MIN	MINDOUBLE
#endif

#endif



#endif	/* _SCHILY_FLOAT_H */
