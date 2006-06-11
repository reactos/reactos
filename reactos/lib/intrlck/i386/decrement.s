/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/intrlck/i386/decrement.s
 * PURPOSE:     Inter lock decrements
 * PROGRAMMERS: Copyright 1995 Martin von Loewis
 *              Copyright 1997 Onno Hovers
 */

/************************************************************************
*           InterlockedDecrement                                        *
*                                                                       *
* InterlockedDecrement adds -1 to a long variable and returns           *
* the resulting decremented value.                                      *
*                                                                       *
************************************************************************/

/*
 * LONG NTAPI InterlockedDecrement(LPLONG lpAddend)
 */

.globl _InterlockedDecrement@4

_InterlockedDecrement@4:
	movl	$-1,%ebx
	lock
	xaddl	%eax,%ebx
	decl	%eax
	leave
	ret	$4

