/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
 *
 *  Moved to MSVC-compatible inline assembler by Mike Nordell, 2003-12-26
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
//#include <ntos/service.h>
#include <internal/trap.h>
#include <internal/ps.h>

#include <roscfg.h>
#include <internal/ntoskrnl.h>
#include <internal/i386/segment.h>


extern KSPIN_LOCK PiThreadListLock;
extern ULONG PiNrThreadsAwaitingReaping;
extern ULONG MmGlobalKernelPageDirectory[1024];

VOID STDCALL PiWakeupReaperThread(VOID);
VOID         KeSetBaseGdtSelector(ULONG Entry, PVOID Base);


/* 
 * FUNCTIONS: Switches to another thread's context
 * ARGUMENTS:
 *        Thread = Thread to switch to
 *        OldThread = Thread to switch from
 */
__declspec(naked)
VOID
Ki386ContextSwitch(struct _KTHREAD* NewThread,  struct _KTHREAD* OldThread)
{
	__asm
	{
		push	ebp
		mov		ebp, esp

		/* Save callee save registers. */
		push	ebx
		push	esi
		push	edi

		cli	/* This is a critical section for this processor. */

	/* Get the pointer to the new thread. */
		mov		ebx, NewThread
		
		/*
		 * Set the base of the TEB selector to the base of the TEB for
		 * this thread.
		 */
		push	ebx
		push	KTHREAD_TEB[ebx]
		push	TEB_SELECTOR
		call	KeSetBaseGdtSelector
		add		esp, 8
		pop		ebx

		/*
		 * Load the PCR selector.
		 */
		mov		eax, PCR_SELECTOR
		mov		fs, ax

		/*
		 * Set the current thread information in the PCR.
		 */
		mov		fs:KPCR_CURRENT_THREAD, ebx

		/*
		 * Set the current LDT
		 */
		xor		eax, eax
		mov		edi, ETHREAD_THREADS_PROCESS[ebx]
		test	word ptr KPROCESS_LDT_DESCRIPTOR0[edi], 0xFFFF
		jz	L4

		push	KPROCESS_LDT_DESCRIPTOR1[edi]
		push	KPROCESS_LDT_DESCRIPTOR0[edi]
		push	LDT_SELECTOR
		call	KeSetGdtSelector
		add		esp, 12

		mov		eax, LDT_SELECTOR

L4:
		lldt		ax

		/*
		 * Load up the iomap offset for this thread in
		 * preparation for setting it below.
		 */
		mov		eax, KPROCESS_IOPM_OFFSET[edi]

		/*
		 * FIXME: Save debugging state.
		 */

		/*
		 * FIXME: Save floating point state.
		 */

		/*
		 * Switch stacks
		 */
		mov		ebx, 12[ebp]
		mov		KTHREAD_KERNEL_STACK[ebx], esp
		mov		ebx, 8[ebp]
		mov		esp, KTHREAD_KERNEL_STACK[ebx]
		mov		edi, KTHREAD_STACK_LIMIT[ebx]

		/*
		 * Set the stack pointer in this processors TSS
		 */
		mov		esi, fs:KPCR_TSS

		/*
		 * Set current IOPM offset in the TSS
		 */
		mov		KTSS_IOMAPBASE[esi], ax

		mov		eax, KTHREAD_INITIAL_STACK[ebx]
		mov		KTSS_ESP0[esi], eax

		/*
		 * Change the address space
		 */
		mov		ebx, ETHREAD_THREADS_PROCESS[ebx]
		mov		eax, KPROCESS_DIRECTORY_TABLE_BASE[ebx]
		mov		cr3, eax

		/*
		 * Set up the PDE for the top of the new stack.
		 */
		mov		ebx, 0
L2:
		mov		esi, edi
		shr		esi, 22
		mov		eax, 0xF03C0000[esi*4]
		cmp		eax, 0
		jne		L1
		mov		eax, MmGlobalKernelPageDirectory[esi*4]
		mov		0xF03C0000[esi*4], eax
L1:
		add		edi, 4096
		inc		ebx
		cmp		ebx, (MM_STACK_SIZE / 4096)
		jl		L2
		
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
	}

	KeReleaseSpinLockFromDpcLevel(&PiThreadListLock);

	if (PiNrThreadsAwaitingReaping) {
		PiWakeupReaperThread();
	}

	__asm
	{
		/*
		 * Restore the saved register and exit
		 */
		pop		edi
		pop		esi
		pop		ebx

		pop		ebp
		ret
	}
}