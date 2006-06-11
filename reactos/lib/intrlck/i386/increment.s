/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/intrlck/i386/increment.s
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

.globl _InterlockedIncrement@4

_InterlockedIncrement@4:
	movl	$1,%ebx
	lock
	xaddl	%eax,%ebx
	incl	%eax
	leave
	ret	$4
