/* @(#)limits.h	1.7 11/08/13 Copyright 2011 J. Schilling */
/*
 *	Copyright (c) 2011 J. Schilling
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

#ifndef	_SCHILY_LIMITS_H
#define	_SCHILY_LIMITS_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_LIMITS_H
#ifndef	_INCL_LIMITS_H
#include <limits.h>
#define	_INCL_LIMITS_H
#endif
#endif

#ifndef	COLL_WEIGHTS_MAX
#define	COLL_WEIGHTS_MAX	2
#endif

#ifndef	_POSIX2_LINE_MAX
#define	_POSIX2_LINE_MAX	2048
#endif

/*
 * Include sys/param.h for PIPE_BUF
 */
#ifndef	_SCHILY_PARAM_H
#include <schily/param.h>
#endif

#ifndef	PIPE_BUF
#if	defined(__MINGW32__) || defined(_MSC_VER)
#define	PIPE_BUF		5120
#else
#define	PIPE_BUF		512
#endif
#endif	/* PIPE_BUF */

#endif	/* _SCHILY_LIMITS_H */
