/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
 *
 *  Moved to MSVC-compatible inline assembler by Mike Nordell, 2003-12-25
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
/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/bthread.S
 * PURPOSE:         Trap handlers
 * PROGRAMMER:      David Welch (david.welch@seh.ox.ac.uk)
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ddk/status.h>
#include <internal/i386/segment.h>
#include <internal/i386/fpu.h>
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

void KeReturnFromSystemCallWithHook();

VOID PiBeforeBeginThread(CONTEXT c);

/*
 *
 */

__declspec(naked)
VOID PsBeginThread(PKSTART_ROUTINE StartRoutine, PVOID StartContext)
{
	/*
	 * This isn't really a function, we are called as the return address
	 * of the context switch function
	 */

	/*
	 * Do the necessary prolog after a context switch
	 */
	__asm
	{
	call	PiBeforeBeginThread

	/*
	 * Call the actual start of the thread
	 */
	// We must NOT use the arguments by name. VC then uses EBP-relative
	// addressing, and with an EBP of 0 you can imagine what happens.
	mov	ebx, 4[esp]			// StartRoutine
	mov	eax, 8[esp]			// StartContext
	push	eax
	call	ebx                        /* Call the start routine */
	add		esp, 4

	/*
	 * Terminate the thread
	 */
	push	eax
	call	PsTerminateSystemThread
	add		esp, 4

	}

	/* If that fails then bug check */
	KeBugCheck(0);

	/* And if that fails then loop */
	for (;;)
		; // forever
}


__declspec(naked)
VOID PsBeginThreadWithContextInternal(VOID)
{
	/*
	 * This isn't really a function, we are called as the return
	 * address of a context switch
	 */

	/*
	 * Do the necessary prolog before the context switch
	 */
	__asm
	{
	call	PiBeforeBeginThread

	/*
	 * Load the context flags.
	 */
	pop		ebx
	
	/*
	 * Load the debugging registers
	 */
	test	ebx, (CONTEXT_DEBUG_REGISTERS & ~CONTEXT_i386)
	jz	L1
	pop		eax	__asm	mov		dr0, eax
	pop		eax	__asm	mov		dr1, eax
	pop		eax	__asm	mov		dr2, eax
	pop		eax	__asm	mov		dr3, eax
	pop		eax	__asm	mov		dr6, eax
	pop		eax	__asm	mov		dr7, eax
	jmp	L3
L1:
	add		esp, 24
L3:
	
	/*
	 * Load the floating point registers
	 */
	mov		eax, HardwareMathSupport
	test	eax,eax
	jz	L2
	test	ebx, (CONTEXT_FLOATING_POINT & ~CONTEXT_i386)
	jz	L2
	frstor	[esp]
L2:
	add		esp, 112

	/* Load the rest of the thread's user mode context. */
	mov		eax, 0
	jmp		KeReturnFromSystemCallWithHook
	}
}

