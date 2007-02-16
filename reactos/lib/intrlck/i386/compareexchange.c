/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/intrlck/i386/compareexchange.c
 * PURPOSE:     Inter lock compare exchanges
 * PROGRAMMERS: Copyright 1995 Martin von Loewis
 *              Copyright 1997 Onno Hovers
 */

/************************************************************************
 *           InterlockedCompareExchange
 *
 * Atomically compares Destination and Comperand, and if found equal exchanges
 * the value of Destination with Exchange
 *
 * RETURNS
 *	Prior value of value pointed to by Destination
 */

/*
 * LONG NTAPI InterlockedCompareExchange(LPLONG Destination, LONG Exchange, LONG Comperand)
 */

#include <windows.h>
LONG
NTAPI
InterlockedCompareExchange(
	IN OUT LONG volatile *Destination,
	LONG Exchange,
	LONG Comperand)
{
	LONG ret;
	__asm__ __volatile__(
		"lock; cmpxchgl %2,(%1)"
		: "=a" (ret) : "r" (Destination), "r" (Exchange), "0" (Comperand) : "memory" );
	return ret;
}
