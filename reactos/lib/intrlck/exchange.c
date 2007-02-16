/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/intrlck/exchange.c
 * PURPOSE:     Inter lock exchanges
 * PROGRAMMERS: Copyright 1995 Martin von Loewis
 *              Copyright 1997 Onno Hovers
 */

#include <windows.h>

/************************************************************************
 *           InterlockedExchange
 *
 * Atomically exchanges a pair of values.
 *
 * RETURNS
 *	Prior value of value pointed to by Target
 */

LONG NTAPI
InterlockedExchange(
	LPLONG target,
	LONG value)
{
	LONG ret;

	do
	{
		ret = *(volatile LONG *)target;
	} while( InterlockedCompareExchange( target, value, ret ) != ret );

	return ret;
}
