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
/* $Id$
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/trap.s
 * PURPOSE:         Exception handlers
 * PROGRAMMER:      David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/
	
#include <roscfg.h>
#include <ddk/status.h>
#include <internal/i386/ke.h>
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
	/* Skip debug information and unsaved registers */
	addl	$0x18, %esp
	popl	%eax		/* Dr0 */
	movl	%eax, %dr0
	popl	%eax		/* Dr1 */
	movl	%eax, %dr1
	popl	%eax		/* Dr2 */
	movl	%eax, %dr2
	popl	%eax		/* Dr3 */
	movl	%eax, %dr3
	popl	%eax		/* Dr6 */
	movl	%eax, %dr6
	popl	%eax		/* Dr7 */
	movl	%eax, %dr7
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

#ifdef DBG
        /*
         * Cleanup the stack which was used to setup a trapframe with SS:ESP when called
         * from kmode.
         */
        movw    0xC(%esp), %bp             /* Get CS from trapframe */
        cmpw    $KERNEL_CS, %bp
        jne     0f

        /* Copy EBP, CS:EIP and EFLAGS from the trapframe back onto the top of our stack. */
        movl    0x00(%esp), %ebp            /* EBP */
        movl    %ebp, 0x24(%esp)
        movl    0x08(%esp), %ebp            /* EIP */
        movl    %ebp, 0x2C(%esp)
        movl    0x0C(%esp), %ebp            /* CS */
        movl    %ebp, 0x30(%esp)
        movl    0x10(%esp), %ebp            /* EFLAGS */
        movl    %ebp, 0x34(%esp)

        addl    $0x24, %esp
0:
#endif /* DBG */
	popl	%ebp
	addl	$0x4, %esp  /* Ignore error code */
		
	iret

.globl _KiTrapProlog
_KiTrapProlog:	
#ifdef DBG
        /*
         * If we were called from kmode we start setting up a new trapframe (with SS:ESP at the end)
         */
        movw    0x14(%esp), %bx             /* Get old CS */
        cmpw    $KERNEL_CS, %bx
        
        jne     0f

        leal    0x1C(%esp), %ebp
        pushl   %ss                          /* Old SS */
        pushl   %ebp                         /* Old ESP */
        pushl   0x20(%esp)                   /* Old EFLAGS */
        pushl   0x20(%esp)                   /* Old CS */
        pushl   0x20(%esp)                   /* Old EIP */
        pushl   0x20(%esp)                   /* ErrorCode */
        pushl   0x20(%esp)                   /* Ebp */
        pushl   0x20(%esp)                   /* Ebx */
        pushl   0x20(%esp)                   /* Esi */
0:
#endif /* DBG */

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
	andl     $0x0000FFFF, %ebx
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
	movl	%dr7, %eax
	pushl	%eax		/* Dr7 */
	/* Clear all breakpoint enables in dr7. */
	andl	$0xFFFF0000, %eax
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
	pushl	$0     /* XXX: TempESP */
	pushl	$0     /* XXX: TempCS */
	pushl	$0     /* XXX: DebugPointer */
	pushl	$0     /* XXX: DebugArgMark */
	movl    0x60(%esp), %ebx
	pushl	%ebx   /* XXX: DebugEIP */
	pushl	%ebp   /* XXX: DebugEBP */	
		
	/* Load the segment registers */
	movl	$KERNEL_DS, %ebx
	movl	%ebx, %ds
	movl	%ebx, %es
	movl	%ebx, %gs
	
	/*  Set ES to kernel segment  */
	movw	$KERNEL_DS,%bx
	movw	%bx,%es

	movl	%esp, %ebx
	movl	%esp, %ebp		

	/* Save the old trap frame. */
	cmpl	$0, %edi
	je	.L7
	movl	%ss:KTHREAD_TRAP_FRAME(%edi), %edx
	pushl	%edx
	jmp	.L8
.L7:
	pushl	$0
.L8:	

	/* Save a pointer to the trap frame in the current KTHREAD */
	cmpl	$0, %edi
	je	.L6
	movl	%ebx, %ss:KTHREAD_TRAP_FRAME(%edi)
.L6:	
	
	/* Call the C exception handler */
	pushl	%esi
	pushl	%ebx
	call	_KiTrapHandler
	addl	$4, %esp
	addl	$4, %esp

	/* Get a pointer to the current thread */
        movl	%fs:KPCR_CURRENT_THREAD, %esi
	
        /* Restore the old trap frame pointer */
	popl	%ebx
	movl	%ebx, KTHREAD_TRAP_FRAME(%esi)
	
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
	pushl	$0
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$15, %esi
	jmp	_KiTrapProlog

.globl _KiTrap16
_KiTrap16:
	pushl	$0
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$16, %esi
	jmp	_KiTrapProlog
	 
.globl _KiTrap17
_KiTrap17:
	pushl	$0
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$17, %esi
	jmp	_KiTrapProlog

.globl _KiTrap18
_KiTrap18:
	pushl	$0
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$18, %esi
	jmp	_KiTrapProlog

.globl _KiTrap19
_KiTrap19:
	pushl	$0
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	movl	$19, %esi
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
