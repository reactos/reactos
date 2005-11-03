/*
 *  ReactOS kernel
 *  Copyright (C) 2000  David Welch <welch@cwcom.net>
 *
 *  Converted to MSVC-compatible inline assembler by Mike Nordell, 2003.
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
 * FILE:            MSVC6/ntoskrnl/ke_i386_syscall.c
 *                  based on ntoskrnl/ke/i386/syscall.s
 * PURPOSE:         syscall dispatching and support
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


#define KernelMode  (0)
#define UserMode    (1)


// TMN: Replicated here to reduce mess-time
#ifdef STATUS_INVALID_SYSTEM_SERVICE
#undef STATUS_INVALID_SYSTEM_SERVICE
#endif
#define STATUS_INVALID_SYSTEM_SERVICE               0xc000001c


/*
 *
 */

void KiServiceCheck (ULONG Nr);
ULONG KiAfterSystemCallHook(ULONG NtStatus, PKTRAP_FRAME TrapFrame);
VOID KiSystemCallHook(ULONG Nr, ...);

void KeReturnFromSystemCallWithHook();
void KeReturnFromSystemCall();


__declspec(naked)
void interrupt_handler2e(void)
{
	__asm
	{
		/* Construct a trap frame on the stack */

		/* Error code */
		push	0
		push	ebp
		push	ebx
		push	esi
		push	edi
		push	fs
		/* Load PCR selector into fs */
		mov		ebx, PCR_SELECTOR
		mov		fs, bx

		/* Save the old exception list */
		mov		ebx, fs:KPCR_EXCEPTION_LIST
		push	ebx
		/* Set the exception handler chain terminator */
		mov		dword ptr fs:KPCR_EXCEPTION_LIST, 0xffffffff
		/* Get a pointer to the current thread */
		mov		esi, fs:KPCR_CURRENT_THREAD
		/* Save the old previous mode */
		xor		ebx,ebx
		mov		bl, ss:KTHREAD_PREVIOUS_MODE[esi]
		push	ebx
		/* Set the new previous mode based on the saved CS selector */
		mov		ebx, 0x24[esp]
		and		ebx, 0x0000FFFF
		cmp		ebx, KERNEL_CS
#if 0
		// TODO: Verify implementation change and use this code path
		// to remove two conditional jumps.
		setnz	bl
		mov		ss:KTHREAD_PREVIOUS_MODE[esi], bl
#else
		jne		L1
		mov		ss:KTHREAD_PREVIOUS_MODE[esi], KernelMode
		jmp		L3
L1:
		mov		ss:KTHREAD_PREVIOUS_MODE[esi], UserMode
L3:

#endif
		/* Save other registers */	   
		push	eax
		push	ecx
		push	edx
		push	ds
		push	es
		push	gs
		push	0     /* DR7 */
		push	0     /* DR6 */
		push	0     /* DR3 */
		push	0     /* DR2 */
		push	0     /* DR1 */
		push	0     /* DR0 */
		push	0     /* XXX: TempESP */
		push	0     /* XXX: TempCS */
		push	0     /* XXX: DebugPointer */
		push	0     /* XXX: DebugArgMark */
		mov		ebx, 0x60[esp]
		push	ebx   /* DebugEIP */
		push	ebp   /* DebugEBP */

		/* Load the segment registers */
		mov		bx, KERNEL_DS
		mov		ds, bx
		mov		es, bx
		mov		gs, bx

		/* 
		 * Save the old trap frame pointer over where we would save the EDX
		 * register.
		 */
		mov		ebx, KTHREAD_TRAP_FRAME[esi]
		mov		0x3C[esp], ebx

		/* Save a pointer to the trap frame in the TCB */
		mov		KTHREAD_TRAP_FRAME[esi], esp

		/*  Set ES to kernel segment  */
		mov		bx, KERNEL_DS
		mov		es, bx

		/*  Allocate new Kernel stack frame  */
		mov		ebp, esp

		/*  Users's current stack frame pointer is source  */
		mov		esi, edx

		/*  Determine system service table to use  */
		cmp		eax, 0x0fff
		ja		new_useShadowTable

		/*  Check to see if EAX is valid/inrange  */
		cmp		eax, es:KeServiceDescriptorTable + 8
		jbe		new_serviceInRange
		mov		eax, STATUS_INVALID_SYSTEM_SERVICE
		jmp		KeReturnFromSystemCall

new_serviceInRange:

		/*  Allocate room for argument list from kernel stack  */
		mov		ecx, es:KeServiceDescriptorTable + 12
		mov		ecx, es:[ecx + eax * 4]
		sub		esp, ecx

		/*  Copy the arguments from the user stack to the kernel stack  */
		mov		edi, esp
		cld
		repe	movsb

		/*  DS is now also kernel segment  */
		mov		ds, bx

		/* Call system call hook */
		push	eax
		call	KiSystemCallHook
		pop		eax

		/*  Make the system service call  */
		mov		ecx, es:KeServiceDescriptorTable
		mov		eax, es:[ecx + eax * 4]
		call	eax

#if CHECKED
		/*  Bump Service Counter  */
#endif

		/*  Deallocate the kernel stack frame  */
		mov		esp, ebp

		/* Call the post system call hook and deliver any pending APCs */
		push	ebp
		push	eax
		call	KiAfterSystemCallHook
		add		esp, 8

		jmp  KeReturnFromSystemCall

new_useShadowTable:

		sub		eax, 0x1000

		/*  Check to see if EAX is valid/inrange  */
		cmp		eax, es:KeServiceDescriptorTableShadow + 24
		jbe		new_shadowServiceInRange
		mov		eax, STATUS_INVALID_SYSTEM_SERVICE
		jmp		KeReturnFromSystemCall

new_shadowServiceInRange:

		/*  Allocate room for argument list from kernel stack  */
		mov		ecx, es:KeServiceDescriptorTableShadow + 28
		mov		ecx, es:[ecx + eax * 4]
		sub		esp, ecx

		/*  Copy the arguments from the user stack to the kernel stack  */
		mov		edi, esp
		cld
		repe	movsb

		/*  DS is now also kernel segment  */
		mov		ds, bx

		/* Call system call hook */
//		pushl %eax
//		call _KiSystemCallHook
//		popl %eax

		/* Call service check routine */
		push	eax
		call	KiServiceCheck
		pop		eax
 
		/*  Make the system service call  */
		mov		ecx, es:KeServiceDescriptorTableShadow + 16
		mov		eax, es:[ecx + eax * 4]
		call	eax

#if CHECKED
		/*  Bump Service Counter  */
#endif

		/*  Deallocate the kernel stack frame  */
		mov		esp, ebp

		// TMN: Added, to be able to separate this into different functions
		jmp KeReturnFromSystemCallWithHook
	}
}

__declspec(naked)
void KeReturnFromSystemCallWithHook()
{
	__asm
	{
		/* Call the post system call hook and deliver any pending APCs */
		push	esp
		push	eax
		call	KiAfterSystemCallHook
		add		esp, 8

		// TMN: Added, to be able to separate this into different functions
		jmp KeReturnFromSystemCall
	}
}

__declspec(naked)
void KeReturnFromSystemCall()
{
	__asm
	{
		/* Restore the user context */
		/* Get a pointer to the current thread */
		mov		esi, fs:0x124

		/* Restore the old trap frame pointer */
		mov		ebx, 0x3c[esp]
		mov		KTHREAD_TRAP_FRAME[esi], ebx

		/* Skip debug information and unsaved registers */
		add		esp, 0x30
		pop		gs
		pop		es
		pop		ds
		pop		edx
		pop		ecx
		add		esp, 4	/* Don't restore eax */

		/* Restore the old previous mode */
		pop		ebx
		mov		ss:KTHREAD_PREVIOUS_MODE[esi], bl

		/* Restore the old exception handler list */
		pop		ebx
		mov		fs:KPCR_EXCEPTION_LIST, ebx

		pop		fs 
		pop		edi
		pop		esi
		pop		ebx
		pop		ebp
		add		esp, 4	/* Ignore error code */
		
		iretd
	}
}