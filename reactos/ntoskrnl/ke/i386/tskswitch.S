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

#include <roscfg.h>
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
#ifdef KDBG
	jmp	SaveTrapFrameForKDB
SaveTrapFrameForKDB_Return:
#endif
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
	
#ifdef CONFIG_SMP
	/*
	 * Get the pointer to the old thread.
	 */
	movl	12(%ebp), %ebx
	/*
	 * Save FPU state if the thread has used it.
	 */
	movl	$0, %fs:KPCR_NPX_THREAD
	testb	$NPX_STATE_DIRTY, KTHREAD_NPX_STATE(%ebx)
	jz	3f
	movl	KTHREAD_INITIAL_STACK(%ebx), %eax
	cmpl	$0, _FxsrSupport
	je	1f
	fxsave	-SIZEOF_FX_SAVE_AREA(%eax)
	jmp	2f
1:
	fnsave	-SIZEOF_FX_SAVE_AREA(%eax)
2:
	movb	$NPX_STATE_VALID, KTHREAD_NPX_STATE(%ebx)
3:
#endif /* CONFIG_SMP */


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

	/*
	 * Get the pointer to the old thread.
	 */
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
	 * For uni-processor we do this only if NewThread != KPCR->NpxThread
	 */
#ifndef CONFIG_SMP
	cmpl	%ebx, %fs:KPCR_NPX_THREAD
	je	4f
#endif /* !CONFIG_SMP */
	movl	%cr0, %eax
	orl	$X86_CR0_TS, %eax
	movl	%eax, %cr0
4:

	/*
	 * FIXME: Restore debugging state
	 */

	/*
	 * Exit the critical section
	 */
	sti

	call    @KeReleaseDispatcherDatabaseLockFromDpcLevel@0

	cmpl	$0, _PiNrThreadsAwaitingReaping
	je	5f
	call	_PiWakeupReaperThread@0
5:

	/*
	 * Restore the saved register and exit
	 */
	popl	%edi
	popl	%esi
	popl	%ebx

	popl	%ebp
	ret
.endfunc



#ifdef KDBG

SaveTrapFrameForKDB:
	/*
	 * Set up a trap frame.
	 */
				/* Ss - space already reserved by return EIP */
	pushl	%esp		/* Esp */
	pushfl			/* Eflags */
	pushl	%cs		/* Cs */
	pushl	12(%esp)	/* Eip */
	movl	%ss, 16(%esp)	/* Save Ss */
	pushl	$0		/* ErrorCode */
	pushl	%ebp		/* Ebp */
	pushl	%ebx		/* Ebx */
	pushl	%esi		/* Esi */
	pushl	%edi		/* Edi */
	pushl	%fs		/* Fs */
	pushl	$0		/* ExceptionList */
	pushl	$0		/* PreviousMode */
	pushl	%eax		/* Eax */
	pushl	%ecx		/* Ecx */
	pushl	%edx		/* Edx */
	pushl	%ds		/* Ds */
	pushl	%es		/* Es */
	pushl	%gs		/* Gs */
	movl	%dr7, %eax
	pushl	%eax		/* Dr7 */
	/* Clear breakpoint enables in dr7. */
	andl	$~0xffff, %eax
	movl	%eax, %dr7
	movl	%dr6, %eax
	pushl	%eax		/* Dr6 */
	movl	%dr3, %eax
	pushl	%eax		/* Dr3 */
	movl	%dr2, %eax
	pushl	%eax		/* Dr2 */
	movl	%dr1, %eax
	pushl	%eax		/* Dr1 */
	movl	%dr0, %eax
	pushl	%eax		/* Dr0 */
	pushl	$0		/* TempEip */
	pushl	$0		/* TempCs */
	pushl	$0		/* DebugPointer */
	pushl	$0xffffffff	/* DebugArgMark (Exception number) */
	pushl	0x60(%esp)	/* DebugEip */
	pushl	%ebp		/* DebugEbp */

	movl	%esp, %ebp	/* Save pointer to new TrapFrame */

	/* Save the old trapframe and set pointer to the new one */
	movl	0x80(%esp), %ebx	/* Get pointer to OldThread */
	pushl	KTHREAD_TRAP_FRAME(%ebx)
	movl	%ebp, KTHREAD_TRAP_FRAME(%ebx)

	/* Copy the arguments which were passed to Ki386ContextSwitch */
	pushl	0x80(%ebp)		/* OldThread */
	pushl	0x7c(%ebp)		/* NewThread */
	pushl	$RestoreTrapFrameForKDB	/* Return address */

	/* Restore clobbered registers */
	movl	KTRAP_FRAME_EBX(%ebp), %ebx
	movl	KTRAP_FRAME_EBP(%ebp), %ebp

	/* Return */
	jmp	SaveTrapFrameForKDB_Return


RestoreTrapFrameForKDB:
	addl	$8, %esp		/* Remove NewThread and OldThread arguments from the stack */
	movl	0x84(%esp), %ebx	/* Get pointer to OldThread */

	/* Restore the old trapframe */
	popl	KTHREAD_TRAP_FRAME(%ebx)
        
	/*
	 * Pop unused portions of the trap frame:
	 *   DebugEbp
	 *   DebugEip
	 *   DebugArgMark
	 *   DebugPointer
	 *   TempCs
	 *   TempEip
	 *   Dr0-3
	 *   Dr6-7
	 */
	addl	$(12*4), %esp

	/*
	 * Restore registers including any that might have been changed
	 * inside the debugger.
	 */
	popl	%gs		/* Gs */
	popl	%es		/* Es */
	popl	%ds		/* Ds */
	popl	%edx		/* Edx */
	popl	%ecx		/* Ecx */
	popl	%eax		/* Eax */
	addl	$4, %esp	/* PreviousMode */
	addl	$4, %esp	/* ExceptionList */
	popl	%fs		/* Fs */
	popl	%edi		/* Edi */
	popl	%esi		/* Esi */
	popl	%ebx		/* Ebx */

        /* Remove SS:ESP from the stack */
	movl	16(%esp), %ebp
	movl	%ebp, 24(%esp)
	movl	12(%esp), %ebp
	movl	%ebp, 20(%esp)
	movl	8(%esp), %ebp
	movl	%ebp, 16(%esp)

	popl	%ebp		/* Ebp */
	addl	$12, %esp	/* ErrorCode and SS:ESP */

	/*
	 * Return to the caller.
	 */
	iret

#endif /* KDBG */

