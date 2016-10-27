/* @(#)inttypes.h	1.35 12/01/22 Copyright 1997-2012 J. Schilling */
/*
 *	Abstraction from inttypes.h
 *
 *	Copyright (c) 1997-2012 J. Schilling
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

#ifndef	_SCHILY_INTTYPES_H
#define	_SCHILY_INTTYPES_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

/*
 * inttypes.h is based on stdint.h
 */
#ifndef	_SCHILY_STDINT_H
#include <schily/stdint.h>
#endif

/*
 * inttypes.h inaddition to stdint.h defines printf() format strings.
 * As we have a portable printf() in libschily, we do not need these #defines.
 */

#endif	/* _SCHILY_INTTYPES_H */
