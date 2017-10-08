/* @(#)ctype.h	1.2 16/06/19 Copyright 2009-2016 J. Schilling */
/*
 *	Ctype abstraction
 *
 *	Copyright (c) 2009-2016 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef	_SCHILY_CTYPE_H
#define	_SCHILY_CTYPE_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_TYPES_H
#include <schily/types.h>
#endif

#ifdef	HAVE_CTYPE_H
#ifndef	_INCL_CTYPE_H
#include <ctype.h>
#define	_INCL_CTYPE_H
#endif
#else

#endif	/* ! HAVE_CTYPE_H */

#endif	/* _SCHILY_CTYPE_H */
