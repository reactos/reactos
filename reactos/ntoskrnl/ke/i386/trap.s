/* $Id$
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/trap.s
 * PURPOSE:         Exception handlers
 * PROGRAMMER:      David Welch <welch@cwcom.net>
 */

/* INCLUDES ******************************************************************/

#include <roscfg.h>
#include <ndk/asm.h>
#include <ndk/i386/segment.h>

#define KernelMode 0
#define UserMode 1

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
	popl	%ebx

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
	movl	$_KiTrapHandler, %ebx
	
.global _KiTrapProlog2
_KiTrapProlog2:
	pushl	%edi
	pushl	%fs

	/* Make room for the previous mode and the exception list */
	subl	$8, %esp

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
	leal    0x64(%esp), %eax
	pushl	%eax		/* XXX: TempESP */
	pushl	%ss		/* XXX: TempSS */
	pushl	$0		/* XXX: DebugPointer */
	pushl	$0		/* XXX: DebugArgMark */
	movl    0x60(%esp), %eax
	pushl	%eax		/* XXX: DebugEIP */
	pushl	%ebp		/* XXX: DebugEBP */	
	
	/* Load the segment registers */
	movl	$KERNEL_DS, %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %gs
	
	/* save the trap frame */
	movl	%esp, %ebp		
	
	/* Load the PCR selector into fs */
	movl	$PCR_SELECTOR, %eax
	movl	%eax, %fs

	/* Save the old exception list */
	movl    %fs:KPCR_EXCEPTION_LIST, %eax
	movl	%eax, KTRAP_FRAME_EXCEPTION_LIST(%ebp)
	
	/* Get a pointer to the current thread */
	movl    %fs:KPCR_CURRENT_THREAD, %edi

	/* The current thread may be NULL early in the boot process */
	cmpl	$0, %edi
	je	.L4
		
	/* Save the old previous mode */
	movl    $0, %eax
	movb    KTHREAD_PREVIOUS_MODE(%edi), %al
	movl	%eax, KTRAP_FRAME_PREVIOUS_MODE(%ebp)
	
        /* Set the new previous mode based on the saved CS selector */
	movl	 KTRAP_FRAME_CS(%ebp), %eax
	andl     $0x0000FFFF, %eax

	/* Save the old trap frame. */
	movl	KTHREAD_TRAP_FRAME(%edi), %edx
	pushl	%edx
	
	/* Save a pointer to the trap frame in the current KTHREAD */
	movl	%ebp, KTHREAD_TRAP_FRAME(%edi)
.L6:	
	
	/* Call the C exception handler */
	pushl	%esi
	pushl	%ebp
	call	*%ebx
	addl	$8, %esp

	/* Get a pointer to the current thread */
        movl	%fs:KPCR_CURRENT_THREAD, %esi
        
        /* Restore the old trap frame pointer */
	popl	%ebx
	cmpl	$0, %esi
	je	_KiTrapEpilog
	movl	%ebx, KTHREAD_TRAP_FRAME(%esi)

	/* Return to the caller */
	jmp	_KiTrapEpilog

	/* Handle the no-thread case out of line */
.L4:
	movl	$0, %eax	/* previous mode */
	movl	%eax, KTRAP_FRAME_PREVIOUS_MODE(%ebp)
	pushl	%eax		/* old trap frame */
	jmp	.L6	
	
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
	movl	$_KiPageFaultHandler, %ebx
	jmp	_KiTrapProlog2

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
