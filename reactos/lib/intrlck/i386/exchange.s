/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/intrlck/i386/exchange.s
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

.globl _InterlockedExchange@8

_InterlockedExchange@8:
	lock
	xchgl	%eax,%ebx
	leave
	ret	$8
