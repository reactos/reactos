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
 * FILE:	    ntoskrnl/ke/i386/tskswitch.S
 * PURPOSE:	 Microkernel thread support
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *		  Created 09/10/00
 */

/* INCLUDES ******************************************************************/

#include <internal/i386/segment.h>
#include <internal/i386/ke.h>
#include <internal/i386/fpu.h>
#include <internal/ps.h>
#include <ntos/tss.h>
#include <internal/ntoskrnl.h>

/* FUNCTIONS ****************************************************************/

.globl _Ki386ContextSwitch
.func Ki386ContextSwitch
_Ki386ContextSwitch:
/*
 * FUNCTIONS: Switches to another thread's context
 * ARGUMENTS:
 *	Thread = Thread to switch to
 *	OldThread = Thread to switch from
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
	 * Set the current LDT
	 */
	xorl	%eax, %eax
	movl	KTHREAD_APCSTATE_PROCESS(%ebx), %edi
	testw	$0xFFFF, KPROCESS_LDT_DESCRIPTOR0(%edi)
	jz	0f

	pushl	KPROCESS_LDT_DESCRIPTOR1(%edi)
	pushl	KPROCESS_LDT_DESCRIPTOR0(%edi)
	pushl	$LDT_SELECTOR
	call	_KeSetGdtSelector
	addl	$12, %esp

	movl	$LDT_SELECTOR, %eax

0:
	lldtw	%ax

	movl	12(%ebp), %ebx

	/*
	 * FIXME: Save debugging state.
	 */

	/*
	 * Load up the iomap offset for this thread in
	 * preparation for setting it below.
	 */
	movl	KPROCESS_IOPM_OFFSET(%edi), %eax

	/*
	 * Save the stack pointer in this processors TSS
	 */
	movl	%fs:KPCR_TSS, %esi
	pushl	KTSS_ESP0(%esi)

	/*
	 * Switch stacks
	 */
	movl	%esp, KTHREAD_KERNEL_STACK(%ebx)
	movl	8(%ebp), %ebx
	movl	KTHREAD_KERNEL_STACK(%ebx), %esp
	movl	KTHREAD_STACK_LIMIT(%ebx), %edi

	movl	%fs:KPCR_TSS, %esi

	/*
	 * Set current IOPM offset in the TSS
	 */
	movw	%ax, KTSS_IOMAPBASE(%esi)

	/*
	 * Change the address space
	 */
	movl	KTHREAD_APCSTATE_PROCESS(%ebx), %eax
	movl	KPROCESS_DIRECTORY_TABLE_BASE(%eax), %eax
	movl	%eax, %cr3

	/*
	 * Restore the stack pointer in this processors TSS
	 */
	popl	KTSS_ESP0(%esi)

	/*
	 * Set TS in cr0 to catch FPU code and load the FPU state when needed
	 * We do this only if NewThread != KPCR->NpxThread
	 */
	cmpl	%ebx, %fs:KPCR_NPX_THREAD
	je	1f
	movl	%cr0, %eax
	orl	$X86_CR0_TS, %eax
	movl	%eax, %cr0
1:

	/*
	 * FIXME: Restore debugging state
	 */

	/*
	 * Exit the critical section
	 */
	sti

	push	$_PiThreadLock
	call	_KeReleaseSpinLockFromDpcLevel@4

	cmpl	$0, _PiNrThreadsAwaitingReaping
	je	4f
	call	_PiWakeupReaperThread@0
4:

	/*
	 * Restore the saved register and exit
	 */
	popl	%edi
	popl	%esi
	popl	%ebx

	popl	%ebp
	ret
.endfunc

