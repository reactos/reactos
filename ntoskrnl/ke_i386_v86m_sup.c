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
/*
 * FILE:            ntoskrnl/ke/i386/vm86_sup.S
 * PURPOSE:         V86 mode support
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 09/10/00
 */

/* INCLUDES ******************************************************************/

#pragma hdrstop

#include <ddk/ntddk.h>
#include <ddk/status.h>
#include <internal/i386/segment.h>
#include <internal/i386/fpu.h>
#include <internal/ps.h>
#include <ddk/defines.h>
#include <internal/v86m.h>
#include <ntos/tss.h>
#include <internal/trap.h>
#include <internal/ps.h>

#include <roscfg.h>
#include <internal/ntoskrnl.h>
#include <internal/i386/segment.h>
#include <internal/ps.h>


// Taken from ntoskrnl/include/internal/v86m.h, since that one must be fixed
// a bit before it could be used from here.
#define	KV86M_REGISTERS_EBP	(0x0)
#define	KV86M_REGISTERS_EDI	(0x4)
#define	KV86M_REGISTERS_ESI	(0x8)
#define KV86M_REGISTERS_EDX	(0xC)
#define	KV86M_REGISTERS_ECX	(0x10)
#define KV86M_REGISTERS_EBX	(0x14)
#define KV86M_REGISTERS_EAX	(0x18)
#define	KV86M_REGISTERS_DS	(0x1C)
#define KV86M_REGISTERS_ES	(0x20)
#define KV86M_REGISTERS_FS	(0x24)
#define KV86M_REGISTERS_GS	(0x28)
#define KV86M_REGISTERS_EIP     (0x2C)
#define KV86M_REGISTERS_CS      (0x30)
#define KV86M_REGISTERS_EFLAGS  (0x34)
#define	KV86M_REGISTERS_ESP     (0x38)
#define KV86M_REGISTERS_SS	(0x3C)


void KiV86Complete();

	
	/*
	 * Starts in v86 mode with the registers set to the
	 * specified values.
	 */

__declspec(naked)
VOID Ki386RetToV86Mode(KV86M_REGISTERS* InRegs,
                       KV86M_REGISTERS* OutRegs)
{
	__asm
	{
    /*
	 * Setup a stack frame
	 */
	push	ebp
	mov		ebp, esp

	pushad	/* Save registers */

	mov		ebx, InRegs

	/*
	 * Save ebp
	 */
	push	ebp
	
	/*
	 * Save a pointer to IN_REGS which the v86m exception handler will
	 * use to handle exceptions
	 */
	push	ebx

	/*
	 * Since we are going to fiddle with the stack pointer this must be
	 * a critical section for this processor
	 */
	
	/*
	 * Save the old initial stack
	 */
	mov		esi, fs:KPCR_CURRENT_THREAD
	mov		edi, KTHREAD_INITIAL_STACK[esi]
	push	edi

	/*
	 * We also need to set the stack in the kthread structure
	 */
	mov		KTHREAD_INITIAL_STACK[esi], esp
	
	/*
	 * The stack used for handling exceptions from v86 mode in this thread
	 * will be the current stack adjusted so we don't overwrite the 
	 * existing stack frames
	 */
	mov		esi, fs:KPCR_TSS
	mov		KTSS_ESP0[esi], esp
	
	/*
	 * Create the stack frame for an iret to v86 mode
	 */
	push	KV86M_REGISTERS_GS[ebx]
	push	KV86M_REGISTERS_FS[ebx]
	push	KV86M_REGISTERS_DS[ebx]
	push	KV86M_REGISTERS_ES[ebx]
	push	KV86M_REGISTERS_SS[ebx]
	push	KV86M_REGISTERS_ESP[ebx]
	push	KV86M_REGISTERS_EFLAGS[ebx]
	push	KV86M_REGISTERS_CS[ebx]
	push	KV86M_REGISTERS_EIP[ebx]

	/*
	 * Setup the CPU registers
	 */
	mov		eax, KV86M_REGISTERS_EAX[ebx]
	mov		ecx, KV86M_REGISTERS_ECX[ebx]
	mov		edx, KV86M_REGISTERS_EDX[ebx]
	mov		esi, KV86M_REGISTERS_ESI[ebx]
	mov		edi, KV86M_REGISTERS_EDI[ebx]
	mov		ebp, KV86M_REGISTERS_EBP[ebx]
	mov		ebx, KV86M_REGISTERS_EBX[ebx]

	/*
	 * Go to v86 mode
	 */
	iretd

	/*
	 * Handle the completion of a vm86 routine. We are called from
	 * an exception handler with the registers at the point of the
	 * exception on the stack.
	 */

		jmp KiV86Complete	// TMN: Function-splitting
	}
}

__declspec(naked)
void KiV86Complete()
{
	__asm
	{
	/* Restore the original ebp */
	mov		ebp, TF_ORIG_EBP[esp]
	
	/* Get a pointer to the OUT_REGS structure */
	mov		ebx, 12[ebp]	// OutRegs

	/* Skip debug information and unsaved registers */
	add		esp, 0x30

	/* Ignore 32-bit segment registers */
	add		esp, 12

	/* Save the vm86 registers into the OUT_REGS structure */
	pop		dword ptr KV86M_REGISTERS_EDX[ebx]
	pop		dword ptr KV86M_REGISTERS_ECX[ebx]
	pop		dword ptr KV86M_REGISTERS_EAX[ebx]

	/* Restore the old previous mode */
	pop		eax
	mov		ss:KTHREAD_PREVIOUS_MODE[esi], al

	/* Restore the old exception handler list */
	pop		eax
	mov		fs:KPCR_EXCEPTION_LIST, eax
	
	/* Ignore the 32-bit fs register */
	add		esp, 4

	pop		dword ptr KV86M_REGISTERS_EDI[ebx]
	pop		dword ptr KV86M_REGISTERS_ESI[ebx]
	pop		dword ptr KV86M_REGISTERS_EBX[ebx]
	pop		dword ptr KV86M_REGISTERS_EBP[ebx]

	/* Ignore error code */
	add		esp, 4

	pop		dword ptr KV86M_REGISTERS_EIP[ebx]
	pop		dword ptr KV86M_REGISTERS_CS[ebx]
	pop		dword ptr KV86M_REGISTERS_EFLAGS[ebx]
	pop		dword ptr KV86M_REGISTERS_ESP[ebx]
	pop		dword ptr KV86M_REGISTERS_SS[ebx]
	pop		dword ptr KV86M_REGISTERS_ES[ebx]
	pop		dword ptr KV86M_REGISTERS_DS[ebx]
	pop		dword ptr KV86M_REGISTERS_FS[ebx]
	pop		dword ptr KV86M_REGISTERS_GS[ebx]

	/*
	 * We are going to fiddle with the stack so this must be a critical
	 * section for this process
	 */
	cli

	/*
	 * Restore the initial stack
	 */
	pop		eax
	mov		esi, fs:KPCR_TSS
	mov		KTSS_ESP0[esi], eax

	/*
	 * We also need to set the stack in the kthread structure
	 */
	mov		esi, fs:KPCR_CURRENT_THREAD
	mov		edi, KTHREAD_INITIAL_STACK[esi]
	mov		KTHREAD_INITIAL_STACK[esi], eax

	/* Exit the critical section */
	sti
	
	/* Ignore IN_REGS pointer */
	add		esp, 4
	
	/* Ignore ebp restored above */
	add		esp, 4
	
	/* Return to caller */
	popad
	mov		esp, ebp
	pop		ebp
	ret

	}	// end of __asm block
}