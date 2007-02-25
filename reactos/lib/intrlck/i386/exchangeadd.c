/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/intrlck/i386/exchangeadd.c
 * PURPOSE:     Inter lock exchange adds
 * PROGRAMMERS: Copyright 1995 Martin von Loewis
 *              Copyright 1997 Onno Hovers
 */

/************************************************************************
 *           InterlockedExchangeAdd
 *
 * Atomically adds Increment to Addend and returns the previous value of
 * Addend
 *
 * RETURNS
 *	Prior value of value pointed to by Addend
 */

/*
 * LONG NTAPI InterlockedExchangeAdd(PLONG Addend, LONG Increment)
 */

#include <windows.h>
LONG
NTAPI
InterlockedExchangeAdd(
	IN OUT LONG volatile *Addend,
	LONG Increment)
{
	LONG ret;
	__asm__ (
		/* lock for SMP systems */
		"lock\n\t"
		"xaddl %0,(%1)"
		:"=r" (ret)
		:"r" (Addend), "0" (Increment)
		:"memory" );
	return ret;
}
