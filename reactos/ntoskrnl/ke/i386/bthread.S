/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: bthread.S,v 1.8 2003/05/28 18:09:10 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/bthread.S
 * PURPOSE:         Trap handlers
 * PROGRAMMER:      David Welch (david.welch@seh.ox.ac.uk)
 */

/* INCLUDES ******************************************************************/
	
#include <ddk/status.h>
#include <internal/i386/segment.h>
#include <internal/ps.h>
#include <ddk/defines.h>

/* Values for contextflags */
#define CONTEXT_i386    0x10000
#ifndef CONTEXT_CONTROL
#define CONTEXT_CONTROL         (CONTEXT_i386 | 1)	
#endif
#ifndef CONTEXT_INTEGER
#define CONTEXT_INTEGER         (CONTEXT_i386 | 2)	
#endif
#ifndef CONTEXT_SEGMENTS
#define CONTEXT_SEGMENTS        (CONTEXT_i386 | 4)	
#endif
#ifndef CONTEXT_FLOATING_POINT
#define CONTEXT_FLOATING_POINT  (CONTEXT_i386 | 8)	
#endif
#ifndef CONTEXT_DEBUG_REGISTERS
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_i386 | 0x10)
#endif
#ifndef CONTEXT_FULL
#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS)
#endif
	
/* FUNCTIONS *****************************************************************/

/*
 *
 */

.globl _PsBeginThreadWithContextInternal
.globl _PsBeginThread

_PsBeginThread:
	/*
	 * This isn't really a function, we are called as the return address
	 * of the context switch function
	 */

	/*
	 * Do the necessary prolog after a context switch
	 */
	call	_PiBeforeBeginThread

	/*
	 * Call the actual start of the thread
	 */
	movl	4(%esp), %ebx                /* Start routine */
	movl	8(%esp), %eax                /* Start context */
	pushl	%eax
	call	*%ebx                        /* Call the start routine */
	addl	$4, %esp

	/*
	 * Terminate the thread
	 */
	pushl	%eax
	call	_PsTerminateSystemThread@4
	addl	$4, %esp

	/*
	 * If that fails then bug check
	 */
	pushl	$0
	call	_KeBugCheck@4
	addl    $4, %esp

	/*
	 * And if that fails then loop
	 */
.1:
	jmp	.1


_PsBeginThreadWithContextInternal:
	/*
	 * This isn't really a function, we are called as the return
	 * address of a context switch
	 */

	/*
	 * Do the necessary prolog before the context switch
	 */
	call	_PiBeforeBeginThread

	/*
	 * Load the context flags.
	 */
	popl	%ebx
	
	/*
	 * Load the debugging registers
	 */
	testl	$CONTEXT_DEBUG_REGISTERS, %ebx
	jz	.L1
	popl	%eax		/* Dr0 */
	movl	%eax, %dr0
	popl	%eax		/* Dr1 */
	movl	%eax, %dr1
	popl	%eax		/* Dr2 */
	movl	%eax, %dr2
	popl	%eax		/* Dr3 */
	movl	%eax, %dr3
	popl	%eax		/* Dr6 */
	movl	%eax, %dr6
	popl	%eax		/* Dr7 */
	movl	%eax, %dr7
	jmp	.L3
.L1:
	addl	$24, %esp
.L3:	
	
	/*
	 * Load the floating point registers
	 */
	movl	(_HardwareMathSupport), %eax
	jz	.L2
	testl	$CONTEXT_FLOATING_POINT, %ebx
	jz	.L2
	frstor	(%esp)
.L2:
	addl	$112, %esp

	/* Load the rest of the thread's user mode context. */
	movl	$0, %eax
	jmp	KeReturnFromSystemCallWithHook

