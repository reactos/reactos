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
/*
 * FILE:            ntoskrnl/ke/i386/vm86_sup.S
 * PURPOSE:         V86 mode support
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 09/10/00
 */

#include <internal/vm86.h>

.globl _Ki386RetToV86Mode

	/*
	 * NTSTATUS Ki386RetToV86Mode(KV86M_REGISTERS* in_regs,
	 *                            KV86M_REGISTERS* out_regs);
	 *
	 * Starts in v86 mode with the registers set to the
	 * specified values.
	 */		
_Ki386RetToV86Mode:
        /*
	 * Setup a stack frame
	 */
	pushl	%ebp
	movl	%esp, %ebp
	
	/*
	 * Save registers
	 */
	pusha
	
	/*
	 * Get a pointer to IN_REGS
	 */
	movl	8(%ebp), %ebx

	/*
	 * Save ebp
	 */
	pushl   %ebp
	
	/*
	 * The stack used for handling exceptions from v86 mode in this thread
	 * will be the current stack adjusted so we don't overwrite the 
	 * existing stack frames
	 */
	movl	%esp, TSS_ESP0(%esi)

	/*
	 * Create the stack frame for an iret to v86 mode
	 */
	pushl	GS(%ebx)
	pushl	FS(%ebx)
	pushl	DS(%ebx)
	pushl	ES(%ebx)
	pushl	SS(%ebx)
	pushl	ESP(%ebx)
	pushl	EFLAGS(%ebx)
	pushl	CS(%ebx)
	pushl	EIP(%ebx)

	/*
	 * Setup the CPU registers
	 */
	movl	EAX(%ebx), %eax
	movl	ECX(%ebx), %ecx
	movl	EDX(%ebx), %edx
	movl	ESI(%ebx), %esi
	movl	EDI(%ebx), %edi
	movl	EBP(%ebx), %ebp
	movl	EBX(%ebx), %ebx

	/*
	 * Go to v86 mode
	 */
	iret
		
	/*
	 * Handle the completion of a vm86 routine. We are called from
	 * an exception handler with the registers at the point of the
	 * exception on the stack.
	 */
_vm86_complete:
        addl    $4, %esp        /* Ignore pointer to registers */    
	addl	$4, %esp	/* Ignore exception number */
	
	/* Get a pointer to the vm86_registers structure */
	movl    _vm86_old_ebp, %ebp
	movl    8(%ebp), %ebx
	
	/* Save the vm86 registers into the vm86_registers structure */
	popl    EBP(%ebx)
	popl    EDI(%ebx)
	popl	ESI(%ebx)
	popl	EDX(%ebx)
	popl	ECX(%ebx)
	popl	EBX(%ebx)
	popl	EAX(%ebx)
	addl	$16, %esp	/* Ignore 32-bit segment registers */
	addl	$4, %esp	/* Ignore error code */

	popl	EIP(%ebx)
	popl	CS(%ebx)
	popl	EFLAGS(%ebx)
	popl	ESP(%ebx)
	popl	SS(%ebx)
	popl	ES(%ebx)
	popl	DS(%ebx)
	popl	FS(%ebx)
	popl	GS(%ebx)

	/* Swap back to the stack */
	cli
	movl	_vm86_old_esp, %esp
	sti
	movl	_vm86_old_ebp, %ebp

	/* Return to caller */
	popa
	movl	%ebp, %esp	
	popl	%ebp
	ret
	
		
