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
/* $Id: trap.s,v 1.12 2002/01/27 01:11:23 dwelch Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/trap.s
 * PURPOSE:         Exception handlers
 * PROGRAMMER:      David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/
	
#include <ddk/status.h>
#include <internal/i386/segment.h>
#include <internal/ps.h>
#include <ddk/defines.h>

/* FUNCTIONS *****************************************************************/

/*
 * Epilog for exception handlers
 */
_KiTrapEpilog:
	cmpl	$1, %eax       /* Check for v86 recovery */
	jne     _KiTrapRet
	jmp	_KiV86Complete
_KiTrapRet:		
	/* Get a pointer to the current thread */
        movl	%fs:0x124, %esi
	
        /* Restore the old trap frame pointer */
        movl	0x3c(%esp), %ebx
	movl	%ebx, KTHREAD_TRAP_FRAME(%esi)
	
	/* Skip debug information and unsaved registers */
	addl	$0x30, %esp
	popl	%gs
	popl	%es
	popl	%ds
	popl	%edx
	popl	%ecx
	popl	%eax

	/* Restore the old previous mode */
	popl	%ebx
	movb	%bl, %ss:KTHREAD_PREVIOUS_MODE(%esi)

	/* Restore the old exception handler list */
	popl	%ebx
	movl	%ebx, %fs:KPCR_EXCEPTION_LIST
	
	popl	%fs 
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	addl	$0x4, %esp  /* Ignore error code */
		
	iret

.globl _KiTrapProlog
_KiTrapProlog:	
	pushl	%edi
	pushl	%fs

	/* 
	 * Check that the PCR exists, very early in the boot process it may 
	 * not 
	 */
	cmpl	$0, %ss:_KiPcrInitDone
	je	.L5
	
	/* Load the PCR selector into fs */
	movl	$PCR_SELECTOR, %ebx
	movl	%ebx, %fs

	/* Save the old exception list */
	movl    %fs:KPCR_EXCEPTION_LIST, %ebx
	pushl	%ebx
	
	/* Put the exception handler chain terminator */
	movl    $0xffffffff, %fs:KPCR_EXCEPTION_LIST
	
	/* Get a pointer to the current thread */
	movl    %fs:KPCR_CURRENT_THREAD, %edi

	/* The current thread may be NULL early in the boot process */
	cmpl	$0, %edi
	je	.L4
		
	/* Save the old previous mode */
	movl    $0, %ebx
	movb    %ss:KTHREAD_PREVIOUS_MODE(%edi), %bl
	pushl   %ebx
	
        /* Set the new previous mode based on the saved CS selector */
	movl	 0x24(%esp), %ebx
	cmpl     $KERNEL_CS, %ebx
	jne      .L1
	movb     $KernelMode, %ss:KTHREAD_PREVIOUS_MODE(%edi)
	jmp      .L3
.L1:
	movb     $UserMode, %ss:KTHREAD_PREVIOUS_MODE(%edi)
.L3:
	
	/* Save other registers */	
	pushl	%eax
	pushl	%ecx
	pushl	%edx
	pushl	%ds
	pushl	%es
	pushl	%gs
	pushl	$0     /* DR7 */
	pushl	$0     /* DR6 */
	pushl	$0     /* DR3 */
	pushl	$0     /* DR2 */
	pushl	$0     /* DR1 */
	pushl	$0     /* DR0 */
	pushl	$0     /* XXX: TempESP */
	pushl	$0     /* XXX: TempCS */
	pushl	$0     /* XXX: DebugPointer */
	pushl	$0     /* XXX: DebugArgMark */
	pushl	$0     /* XXX: DebugEIP */
	pushl	$0     /* XXX: DebugEBP */

	/* Load the segment registers */
	movl	$KERNEL_DS, %ebx
	movl	%ebx, %ds
	movl	%ebx, %es
	movl	%ebx, %gs

	/*  Set ES to kernel segment  */
	movw	$KERNEL_DS,%bx
	movw	%bx,%es

	movl	%esp, %ebx

	/* Save a pointer to the trap frame in the current KTHREAD */
	movl  %ebx, %ss:KTHREAD_TRAP_FRAME(%edi)

	/* Call the C exception handler */
	pushl	%esi
	pushl	%ebx
	call	_KiTrapHandler
	addl	$4, %esp
	addl	$4, %esp

	/* Return to the caller */
	jmp	_KiTrapEpilog

	/* Handle the no-pcr case out of line */
.L5:	
	pushl	$0
		
	/* Handle the no-thread case out of line */
.L4:
	pushl	$0	
	jmp	.L3	
				
.globl _KiTrap0
_KiTrap0:
	/* No error code */
	pushl	$0
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$0, %esi
	jmp	_KiTrapProlog
				
.globl _KiTrap1
_KiTrap1:
	/* No error code */
	pushl	$0
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$1, %esi
	jmp	_KiTrapProlog
	
.globl _KiTrap2
_KiTrap2:
	pushl	$0
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$2, %esi
	jmp	_KiTrapProlog

.globl _KiTrap3
_KiTrap3:
	pushl	$0
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$3, %esi
	jmp	_KiTrapProlog

.globl _KiTrap4
_KiTrap4:
        pushl	$0
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$4, %esi
	jmp	_KiTrapProlog

.globl _KiTrap5
_KiTrap5:
	pushl	$0
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$5, %esi
	jmp	_KiTrapProlog

.globl _KiTrap6
_KiTrap6:
	pushl	$0
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$6, %esi
	jmp	_KiTrapProlog

.globl _KiTrap7
_KiTrap7:
        pushl	$0
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$7, %esi
	jmp	_KiTrapProlog

.globl _KiTrap8
_KiTrap8:
	call	_KiDoubleFaultHandler
	iret

.globl _KiTrap9
_KiTrap9:
        pushl	$0
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$9, %esi
	jmp	_KiTrapProlog

.globl _KiTrap10
_KiTrap10:
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$10, %esi
	jmp	_KiTrapProlog

.globl _KiTrap11
_KiTrap11:
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$11, %esi
	jmp	_KiTrapProlog

.globl _KiTrap12
_KiTrap12:
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$12, %esi
	jmp	_KiTrapProlog

.globl _KiTrap13
_KiTrap13:
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$13, %esi
	jmp	_KiTrapProlog

.globl _KiTrap14
_KiTrap14:
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$14, %esi
	jmp	_KiTrapProlog

.globl _KiTrap15
_KiTrap15:
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$15, %esi
	jmp	_KiTrapProlog

.globl _KiTrap16
_KiTrap16:
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$16, %esi
	jmp	_KiTrapProlog
	 
.globl _KiTrapUnknown
_KiTrapUnknown:
        pushl	$0
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$255, %esi
	jmp	_KiTrapProlog


/* EOF */
