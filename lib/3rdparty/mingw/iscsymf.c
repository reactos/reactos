/*
 * iscsymf.c
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Oldnames from ANSI header ctype.h
 *
 * Some wrapper functions for those old name functions whose appropriate
 * equivalents are not simply underscore prefixed.
 *
 */

#include <ctype.h>

#ifdef iscsymf
#undef iscsymf
#endif

int
iscsymf (int c)
{
	return __iscsymf(c);
}

