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

#include <internal/v86m.h>
#include <ddk/i386/tss.h>
#include <internal/trap.h>
#include <internal/ps.h>
			
.globl _Ki386RetToV86Mode
.globl _KiV86Complete
	
	/*
	 * VOID Ki386RetToV86Mode(KV86M_REGISTERS* InRegs,
	 *	                  KV86M_REGISTERS* OutRegs);
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
	 * Save a pointer to IN_REGS which the v86m exception handler will
	 * use to handle exceptions
	 */
	pushl	%ebx

	/*
	 * Since we are going to fiddle with the stack pointer this must be
	 * a critical section for this processor
	 */
	
	/*
	 * Save the old initial stack
	 */
	movl	%fs:KPCR_CURRENT_THREAD, %esi
	movl	KTHREAD_INITIAL_STACK(%esi), %edi
	pushl	%edi

	/*
	 * We also need to set the stack in the kthread structure
	 */
	movl	%esp, KTHREAD_INITIAL_STACK(%esi)
	
	/*
	 * The stack used for handling exceptions from v86 mode in this thread
	 * will be the current stack adjusted so we don't overwrite the 
	 * existing stack frames
	 */
	movl    %fs:KPCR_TSS, %esi
	movl	%esp, KTSS_ESP0(%esi)
	
	/*
	 * Create the stack frame for an iret to v86 mode
	 */
	pushl	KV86M_REGISTERS_GS(%ebx)
	pushl	KV86M_REGISTERS_FS(%ebx)
	pushl	KV86M_REGISTERS_DS(%ebx)
	pushl	KV86M_REGISTERS_ES(%ebx)
	pushl	KV86M_REGISTERS_SS(%ebx)
	pushl	KV86M_REGISTERS_ESP(%ebx)
	pushl	KV86M_REGISTERS_EFLAGS(%ebx)
	pushl	KV86M_REGISTERS_CS(%ebx)
	pushl	KV86M_REGISTERS_EIP(%ebx)

	/*
	 * Setup the CPU registers
	 */
	movl	KV86M_REGISTERS_EAX(%ebx), %eax
	movl	KV86M_REGISTERS_ECX(%ebx), %ecx
	movl	KV86M_REGISTERS_EDX(%ebx), %edx
	movl	KV86M_REGISTERS_ESI(%ebx), %esi
	movl	KV86M_REGISTERS_EDI(%ebx), %edi
	movl	KV86M_REGISTERS_EBP(%ebx), %ebp
	movl	KV86M_REGISTERS_EBX(%ebx), %ebx

	/*
	 * Go to v86 mode
	 */
	iret
		
	/*
	 * Handle the completion of a vm86 routine. We are called from
	 * an exception handler with the registers at the point of the
	 * exception on the stack.
	 */
_KiV86Complete:
	/* Restore the original ebp */
	movl	TF_ORIG_EBP(%esp), %ebp
	
	/* Get a pointer to the OUT_REGS structure */
	movl	12(%ebp), %ebx

	/* Skip debug information and unsaved registers */
	addl	$0x30, %esp	

	/* Ignore 32-bit segment registers */
	addl	$12, %esp

	/* Save the vm86 registers into the OUT_REGS structure */
	popl	KV86M_REGISTERS_EDX(%ebx)
	popl	KV86M_REGISTERS_ECX(%ebx)
	popl	KV86M_REGISTERS_EAX(%ebx)

	/* Ignore the previous mode */
	addl	$4, %esp

	/* Ignore old exception handler list */
	addl	$4, %esp
	
	/* Ignore the 32-bit fs register */
	addl	$4, %esp

	popl	KV86M_REGISTERS_EDI(%ebx)
	popl	KV86M_REGISTERS_ESI(%ebx)
	popl	KV86M_REGISTERS_EBX(%ebx)
	popl	KV86M_REGISTERS_EBP(%ebx)

	/* Ignore error code */
	addl	$4, %esp

	popl	KV86M_REGISTERS_EIP(%ebx)
	popl	KV86M_REGISTERS_CS(%ebx)
	popl	KV86M_REGISTERS_EFLAGS(%ebx)
	popl	KV86M_REGISTERS_ESP(%ebx)
	popl	KV86M_REGISTERS_SS(%ebx)
	popl	KV86M_REGISTERS_ES(%ebx)
	popl	KV86M_REGISTERS_DS(%ebx)
	popl	KV86M_REGISTERS_FS(%ebx)
	popl	KV86M_REGISTERS_GS(%ebx)

	/*
	 * We are going to fiddle with the stack so this must be a critical
	 * section for this process
	 */
	cli

	/*
	 * Restore the initial stack
	 */
	popl	%eax
	movl    %fs:KPCR_TSS, %esi
	movl	%eax, KTSS_ESP0(%esi)

	/*
	 * We also need to set the stack in the kthread structure
	 */
	movl	%fs:KPCR_CURRENT_THREAD, %esi
	movl	KTHREAD_INITIAL_STACK(%esi), %edi
	movl	%eax, KTHREAD_INITIAL_STACK(%esi)	

	/* Exit the critical section */
	sti
	
	/* Ignore IN_REGS pointer */
	addl	$4, %esp
	
	/* Ignore ebp restored above */
	addl	$4, %esp
	
	/* Return to caller */
	popa
	movl	%ebp, %esp	
	popl	%ebp
	ret
	
		




