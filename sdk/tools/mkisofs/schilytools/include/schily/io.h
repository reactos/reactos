/* @(#)io.h	1.1 09/07/13 Copyright 2009 J. Schilling */
/*
 *	DOS io.h abstraction
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

#ifndef	_SCHILY_IO_H
#define	_SCHILY_IO_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_FCNTL_H
#include <schily/fcntl.h>
#endif

#ifndef	NEED_O_BINARY
#if	O_BINARY != 0
#define	NEED_O_BINARY
#endif
#endif

#if	defined(HAVE_IO_H) && defined(NEED_O_BINARY)
#ifndef	_INCL_IO_H
#include <io.h>
#define	_INCL_IO_H
#endif
#else	/* ! defined(HAVE_IO_H) && defined(NEED_O_BINARY) */

#define	setmode(f, m)

#endif

#endif	/* _SCHILY_IO_H */
