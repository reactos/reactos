/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/intrlck/i386/decrement.c
 * PURPOSE:     Inter lock decrements
 * PROGRAMMERS: Copyright 1995 Martin von Loewis
 *              Copyright 1997 Onno Hovers
 */

/************************************************************************
*           InterlockedDecrement                                        *
*                                                                       *
* InterlockedDecrement adds -1 to a long variable and returns           *
* the resulting decremented value.                                      *
*                                                                       *
************************************************************************/

/*
 * LONG NTAPI InterlockedDecrement(LPLONG lpAddend)
 */

#include <windows.h>
LONG
NTAPI
InterlockedDecrement(LPLONG lpAddend)
{
	LONG ret;
	__asm__
	(
		"\tlock\n"  /* for SMP systems */
		"\txaddl %0, (%1)\n"
		"\tdecl %0\n"
		:"=r" (ret)
		:"r" (lpAddend), "0" (-1)
		: "memory"
	);
	return ret;
}
