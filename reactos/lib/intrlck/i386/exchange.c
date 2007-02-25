/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/intrlck/i386/exchange.c
 * PURPOSE:     Inter lock exchanges
 * PROGRAMMERS: Copyright 1995 Martin von Loewis
 *              Copyright 1997 Onno Hovers
 */

/************************************************************************
 *           InterlockedExchange
 *
 * Atomically exchanges a pair of values.
 *
 * RETURNS
 *	Prior value of value pointed to by Target
 */

/*
 * LONG NTAPI InterlockedExchange(LPLONG target, LONG value)
 */

#include <windows.h>
LONG
NTAPI
InterlockedExchange(IN OUT LONG volatile *target, LONG value)
{
	LONG ret;
	__asm__ (
		/* lock for SMP systems */
		"lock\n\txchgl %0,(%1)"
		:"=r" (ret):"r" (target), "0" (value):"memory" );
	return ret;
}
