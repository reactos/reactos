/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/intrlck/i386/increment.c
 * PURPOSE:     Inter lock increments
 * PROGRAMMERS: Copyright 1995 Martin von Loewis
 *              Copyright 1997 Onno Hovers
 */

/************************************************************************
*           InterlockedIncrement                                        *
*                                                                       *
* InterlockedIncrement adds 1 to a long variable and returns            *
* the resulting incremented value.                                      *
*                                                                       *
************************************************************************/

/*
 * LONG NTAPI InterlockedIncrement(PLONG Addend)
 */

#include <windows.h>
LONG
NTAPI
InterlockedIncrement(PLONG lpAddend)
{
	LONG ret;
	__asm__
	(
		"\tlock\n"  /* for SMP systems */
		"\txaddl %0, (%1)\n"
		"\tincl %0\n"
		:"=r" (ret)
		:"r" (lpAddend), "0" (1)
		: "memory"
	);
	return ret;
}
