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
 * FILE:            ntoskrnl/ke/i386/tskswitch.S
 * PURPOSE:         Microkernel thread support
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 09/10/00
 */

/* INCLUDES ******************************************************************/

#include <internal/i386/segment.h>
#include <internal/ps.h>
#include <ddk/i386/tss.h>
			
/* FUNCTIONS ****************************************************************/

.globl _Ki386ContextSwitch
_Ki386ContextSwitch:
/* 
 * FUNCTIONS: Switches to another thread's context
 * ARGUMENTS:
 *        Thread = Thread to switch to
 *        OldThread = Thread to switch from
 */
	pushl	%ebp
	movl	%esp, %ebp

	/*
	 * Save callee save registers.
	 */
	pushl	%ebx
	pushl	%esi
	pushl	%edi

	/*
	 * This is a critical section for this processor.
	 */
	cli

	/*
	 * Get the pointer to the new thread.
	 */
	movl	8(%ebp), %ebx
		
	/*
	 * Set the base of the TEB selector to the base of the TEB for
	 * this thread.
	 */
	pushl	%ebx
	pushl	KTHREAD_TEB(%ebx)
	pushl	$TEB_SELECTOR
	call	_KeSetBaseGdtSelector
	addl	$8, %esp
	popl	%ebx

	/*
	 * Load the PCR selector.
	 */
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs

	/*
	 * Set the current thread information in the PCR.
	 */
	 movl	%ebx, %fs:KPCR_CURRENT_THREAD

	/*
	 * FIXME: Save debugging state.
	 */

	/*
	 * FIXME: Save floating point state.
	 */

	/*
	 * Switch stacks
	 */
	movl	12(%ebp), %ebx
	movl	%esp, KTHREAD_KERNEL_STACK(%ebx)	
	movl	8(%ebp), %ebx
	movl	KTHREAD_KERNEL_STACK(%ebx), %esp

	/*
	 * Set the stack pointer in this processors TSS
	 */
	movl	KTHREAD_INITIAL_STACK(%ebx), %eax
	movl	%fs:KPCR_TSS, %esi
	movl	%eax, KTSS_ESP0(%esi)

	/*
	 * Change the address space
	 */
	movl	ETHREAD_THREADS_PROCESS(%ebx), %ebx
	movl	KPROCESS_DIRECTORY_TABLE_BASE(%ebx), %eax
	movl	%eax, %cr3 

	/*
	 * FIXME: Restore floating point state
	 */

	/*
	 * FIXME: Restore debugging state
	 */

	/*
	 * Exit the critical section
	 */
	sti
	
	/*
	 * Restore the saved register and exit
	 */
	popl	%edi
	popl	%esi
	popl	%ebx
	
	popl    %ebp
	ret 
