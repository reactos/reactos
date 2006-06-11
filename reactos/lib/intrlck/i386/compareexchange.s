/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/intrlck/i386/compareexchange.s
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

.globl _InterlockedCompareExchange@12

_InterlockedCompareExchange@12:
	movl	12(%esp),%eax
	movl	8(%esp),%ecx
	movl	4(%esp),%edx
	lock
	cmpxchgl	%ecx,(%edx)
	leave
	ret	$12
