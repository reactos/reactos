/*
 * ntdef.h
 *
 * This file is part of the ReactOS PSDK package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _NTDEF_
#define _NTDEF_
#pragma once

/* Dependencies */
#include <ctype.h>
//#include <winapifamily.h>
#include <basetsd.h>
#include <guiddef.h>
#include <excpt.h>
#include <sdkddkver.h>
#include <specstrings.h>
#include <kernelspecs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Default to strict */
#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif

/* Pseudo Modifiers for Input Parameters */

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#ifndef NOTHING
#define NOTHING
#endif

#ifndef CRITICAL
#define CRITICAL
#endif

/* Constant modifier */
#ifndef CONST
#define CONST const
#endif

/* TRUE/FALSE */
#define FALSE   0
#define TRUE    1

/* NULL/NULL64 */
#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#define NULL64  0
#else
#define NULL    ((void *)0)
#define NULL64  ((void * POINTER_64)0)
#endif
#endif /* NULL */

$define(_NTDEF_)
$define(ULONG=ULONG)
$define(USHORT=USHORT)
$define(UCHAR=UCHAR)
$include(ntbasedef.h)

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _NTDEF_ */
