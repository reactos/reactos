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
#include <internal/trap.h>
#include <internal/ps.h>

#include <roscfg.h>
#include <internal/ntoskrnl.h>
#include <internal/i386/segment.h>


void KiV86Complete(void);
void KiTrapHandler(void);
void KiDoubleFaultHandler(void);

extern int KiPcrInitDone;

/*
 * Epilog for exception handlers
 */
__declspec(naked)
void KiTrapEpilog()
{
	__asm
	{
	cmp		eax, 1	/* Check for v86 recovery */
	jne		_KiTrapRet
	jmp		KiV86Complete
_KiTrapRet:				
	/* Skip debug information and unsaved registers */
	add		esp, 0x30
	pop		gs
	pop		es
	pop		ds
	pop		edx
	pop		ecx
	pop		eax

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

__declspec(naked)
void KiTrapProlog()
{
	__asm
	{
		push	edi
		push	fs

		/* 
		 * Check that the PCR exists, very early in the boot process it may 
		 * not 
		 */
		cmp		ss:KiPcrInitDone, 0
		je		L5_

		/* Load the PCR selector into fs */
		mov		ebx, PCR_SELECTOR
		mov		fs, bx

		/* Save the old exception list */
		mov		ebx, fs:KPCR_EXCEPTION_LIST
		push	ebx

		/* Put the exception handler chain terminator */
		mov		dword ptr fs:KPCR_EXCEPTION_LIST, 0xffffffff

		/* Get a pointer to the current thread */
		mov		edi, fs:KPCR_CURRENT_THREAD

		/* The current thread may be NULL early in the boot process */
		cmp		edi, 0
		je		L4_
		
		/* Save the old previous mode */
		xor		ebx, ebx
		mov		bl, ss:KTHREAD_PREVIOUS_MODE[edi]
		push	ebx
	
		/* Set the new previous mode based on the saved CS selector */
		mov		ebx, 0x24[esp]
		and		ebx, 0x0000FFFF
		cmp		ebx, KERNEL_CS
		jne		L1_
		mov		ss:KTHREAD_PREVIOUS_MODE[edi], KernelMode
		jmp		L3_
L1_:
		mov		ss:KTHREAD_PREVIOUS_MODE[edi], UserMode
L3_:
	
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
		push	ebx   /* XXX: DebugEIP */
		push	ebp   /* XXX: DebugEBP */	
		
		/* Load the segment registers */
		mov		ebx, KERNEL_DS
		mov		ds, bx
		mov		es, bx
		mov		gs, bx
	
		/*  Set ES to kernel segment  */
		mov		bx, KERNEL_DS
		mov		es, bx

		mov		ebx, esp
		mov		ebp, esp

	/* Save the old trap frame. */
		cmp		edi, 0
		je		L7_
		mov		edx, ss:KTHREAD_TRAP_FRAME[edi]
		push	edx
		jmp		L8_
L7_:
		push	0
L8_:

		/* Save a pointer to the trap frame in the current KTHREAD */
		cmp		edi, 0
		je		L6_
		mov		ss:KTHREAD_TRAP_FRAME[edi], ebx
L6_:
	
		/* Call the C exception handler */
		push	esi
		push	ebx
		call	KiTrapHandler
		add		esp, 8

		/* Get a pointer to the current thread */
		mov		esi, fs:KPCR_CURRENT_THREAD
	
		/* Restore the old trap frame pointer */
		pop		ebx
		mov		KTHREAD_TRAP_FRAME[esi], ebx
	
		/* Return to the caller */
		jmp		KiTrapEpilog

		/* Handle the no-pcr case out of line */
L5_:
		push	0
		
		/* Handle the no-thread case out of line */
L4_:
		push	0
		jmp		L3_

	}	// end of __asm block
}


__declspec(naked)
void KiTrap0()
{
	__asm
	{
		/* No error code */
		push	0
		push	ebp
		push	ebx
		push	esi
		mov		esi, 0
		jmp		KiTrapProlog
	}
}

__declspec(naked)
void KiTrap1()
{
	__asm
	{
		/* No error code */
		push	0
		push	ebp
		push	ebx
		push	esi
		mov	esi, 1
		jmp		KiTrapProlog
	}
}

__declspec(naked)
void KiTrap2()
{
	__asm
	{
		push	0
		push	ebp
		push	ebx
		push	esi
		mov		esi, 2
		jmp		KiTrapProlog
	}
}

__declspec(naked)
void KiTrap3()
{
	__asm
	{
		push	0
		push	ebp
		push	ebx
		push	esi
		mov		esi, 3
		jmp		KiTrapProlog
	}
}

__declspec(naked)
void KiTrap4()
{
	__asm
	{
		push	0
		push	ebp
		push	ebx
		push	esi
		mov		esi, 4
		jmp		KiTrapProlog
	}
}

__declspec(naked)
void KiTrap5()
{
	__asm
	{
		push	0
		push	ebp
		push	ebx
		push	esi
		mov		esi, 5
		jmp		KiTrapProlog
	}
}

__declspec(naked)
void KiTrap6()
{
	__asm
	{
		push	0
		push	ebp
		push	ebx
		push	esi
		mov		esi, 6
		jmp		KiTrapProlog
	}
}

__declspec(naked)
void KiTrap7()
{
	__asm
	{
		push	0
		push	ebp
		push	ebx
		push	esi
		mov		esi, 7
		jmp		KiTrapProlog
	}
}

__declspec(naked)
void KiTrap8()
{
	__asm
	{
		call	KiDoubleFaultHandler
		iretd
	}
}

__declspec(naked)
void KiTrap9()
{
	__asm
	{
		push	0
		push	ebp
		push	ebx
		push	esi
		mov		esi, 9
		jmp		KiTrapProlog
	}
}

__declspec(naked)
void KiTrap10()
{
	__asm
	{
		push	ebp
		push	ebx
		push	esi
		mov		esi, 10
		jmp		KiTrapProlog
	}
}

__declspec(naked)
void KiTrap11()
{
	__asm
	{
		push	ebp
		push	ebx
		push	esi
		mov		esi, 11
		jmp		KiTrapProlog
	}
}

__declspec(naked)
void KiTrap12()
{
	__asm
	{
		push	ebp
		push	ebx
		push	esi
		mov		esi, 12
		jmp		KiTrapProlog
	}
}

__declspec(naked)
void KiTrap13()
{
	__asm
	{
		push	ebp
		push	ebx
		push	esi
		mov		esi, 13
		jmp		KiTrapProlog
	}
}

__declspec(naked)
void KiTrap14()
{
	__asm
	{
		push	ebp
		push	ebx
		push	esi
		mov		esi, 14
		jmp		KiTrapProlog
	}
}

__declspec(naked)
void KiTrap15()
{
	__asm
	{
		push	0
		push	ebp
		push	ebx
		push	esi
		mov		esi, 15
		jmp		KiTrapProlog
	}
}

__declspec(naked)
void KiTrap16()
{
	__asm
	{
		push	0
		push	ebp
		push	ebx
		push	esi
		mov		esi, 16
		jmp		KiTrapProlog
	}
}

__declspec(naked)
void KiTrapUnknown()
{
	__asm
	{
		push	0
		push	ebp
		push	ebx
		push	esi
		mov		esi, 255
		jmp		KiTrapProlog
	}
}

/* EOF */
