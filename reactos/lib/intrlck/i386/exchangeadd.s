/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/intrlck/i386/exchangeadd.s
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

.globl _InterlockedExchangeAdd@8

_InterlockedExchangeAdd@8:
	lock
	xaddl	%eax,%ebx
	leave
	ret	$4
