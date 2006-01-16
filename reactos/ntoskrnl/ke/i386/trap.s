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
#include <internal/i386/ke.h>

#define KernelMode 0
#define UserMode 1

/* NOTES:
 * The epilog will be replaced by a call to Ki386EoiHelper when bugs are fixed.
 * The prologue is currently a duplication of the trap enter code in KiDebugService.
 * It will be made a macro and shared later.
 */

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

.intel_syntax noprefix
    /* Load the PCR selector into fs */
    mov edi, KGDT_R0_PCR
    mov fs, di

    /* Push exception list and previous mode (invalid) */
    push fs:[KPCR_EXCEPTION_LIST]
    push -1

    /* Push volatiles and segments */
    push eax
    push ecx
    push edx
    push ds
    push es
    push gs

    /* Set the R3 data segment */
    mov ax, KGDT_R3_DATA + RPL_MASK

    /* Skip debug registers and debug stuff */
    sub esp, 0x30

    /* Load the segment registers */
    mov ds, ax
    mov es, ax

    /* Set up frame */
    mov ebp, esp

    /* Check if this was from V86 Mode */
    test dword ptr [ebp+KTRAP_FRAME_EFLAGS], X86_EFLAGS_VM
    //jnz V86_kids

    /* Get current thread */
    mov ecx, [fs:KPCR_CURRENT_THREAD]
    cld

    /* Flush DR7 */
    and dword ptr [ebp+KTRAP_FRAME_DR7], 0

    /* Check if the thread was being debugged */
    //test byte ptr [ecx+KTHREAD_DEBUG_ACTIVE], 0xFF
    //jnz Dr_kids

    /* Get the Debug Trap Frame EBP/EIP */
    mov ecx, [ebp+KTRAP_FRAME_EBP]
    mov edi, [ebp+KTRAP_FRAME_EIP]

    /* Write the debug data */
    mov [ebp+KTRAP_FRAME_DEBUGPOINTER], edx
    mov dword ptr [ebp+KTRAP_FRAME_DEBUGARGMARK], 0xBADB0D00
    mov [ebp+KTRAP_FRAME_DEBUGEBP], ecx
    mov [ebp+KTRAP_FRAME_DEBUGEIP], edi
.att_syntax

.L6:	
	
	/* Call the C exception handler */
	pushl	%esi
	pushl	%ebp
	call	*%ebx
	addl	$8, %esp

	/* Return to the caller */
	jmp	_KiTrapEpilog

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

.intel_syntax noprefix
.globl _KiCoprocessorError@0
_KiCoprocessorError@0:

    /* Get the NPX Thread's Initial stack */
    mov eax, [fs:KPCR_NPX_THREAD]
    mov eax, [eax+KTHREAD_INITIAL_STACK]

    /* Make space for the FPU Save area */
    sub eax, SIZEOF_FX_SAVE_AREA

    /* Set the CR0 State */
    mov dword ptr [eax+FN_CR0_NPX_STATE], 8

    /* Update it */
    mov eax, cr0
    or eax, 8
    mov cr0, eax

    /* Return to caller */
    ret

.globl _Ki386AdjustEsp0@4
_Ki386AdjustEsp0@4:

    /* Get the current thread */
    mov eax, [fs:KPCR_CURRENT_THREAD]

    /* Get trap frame and stack */
    mov edx, [esp+4]
    mov eax, [eax+KTHREAD_INITIAL_STACK]

    /* Check if V86 */
    test dword ptr [edx+KTRAP_FRAME_EFLAGS], X86_EFLAGS_VM
    jnz NoAdjust

    /* Bias the stack */
    sub eax, KTRAP_FRAME_V86_GS - KTRAP_FRAME_SS

NoAdjust:
    /* Skip FX Save Area */
    sub eax, SIZEOF_FX_SAVE_AREA

    /* Disable interrupts */
    pushf
    cli

    /* Adjust ESP0 */
    mov edx, [fs:KPCR_TSS]
    mov ss:[edx+KTSS_ESP0], eax

    /* Enable interrupts and return */
    popf
    ret 4

/* EOF */
