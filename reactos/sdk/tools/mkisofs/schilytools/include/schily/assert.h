/* @(#)assert.h	1.1 09/08/07 Copyright 2009 J. Schilling */
/*
 *	Abstraction code for assert.h
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

#ifndef	_SCHILY_ASSERT_H
#define	_SCHILY_ASSERT_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_ASSERT_H
#ifndef	_INCL_ASSERT_H
#define	_INCL_ASSERT_H
#include <assert.h>
#endif
#else	/* !HAVE_ASSERT_H */

#undef	assert

#ifdef	NDEBUG
#define	assert(ignore)	((void) 0)
#else
#if	defined(__STDC__)
#define	assert(exp) (void)((exp) || (__assert(#exp, __FILE__, __LINE__), 0))
#else
#define	assert(exp) (void)((exp) || (__assert("exp", __FILE__, __LINE__), 0))
#endif
#endif

#endif	/* !HAVE_ASSERT_H */

#endif	/* _SCHILY_ASSERT_H */
