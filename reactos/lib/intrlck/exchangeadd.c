/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/intrlck/exchangeadd.c
 * PURPOSE:     Inter lock exchange adds
 * PROGRAMMERS: Copyright 1995 Martin von Loewis
 *              Copyright 1997 Onno Hovers
 */

#include <windows.h>

/************************************************************************
 *           InterlockedExchangeAdd
 *
 * Atomically adds Increment to Addend and returns the previous value of
 * Addend
 *
 * RETURNS
 *	Prior value of value pointed to by Addend
 */

LONG NTAPI
InterlockedExchangeAdd(
	PLONG Addend,
	LONG Increment)
{
	LONG ret;
	LONG newval;

	do
	{
		ret = *(volatile LONG *)Addend;
		newval = ret + Increment;
	} while (InterlockedCompareExchange(Addend, ret, newval) != ret);

	return ret;
}
