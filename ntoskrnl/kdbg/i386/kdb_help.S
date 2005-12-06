#include <ndk/asm.h>

.text

.globl _KdbEnter
_KdbEnter:
	/*
	 * Set up a trap frame
	 */
	pushfl			/* Eflags */
	pushl	%cs		/* Cs */
	pushl	$0		/* ErrorCode */
	pushl	%ebp		/* Ebp */
	pushl	%ebx		/* Ebx */
	movl	20(%esp), %ebp	/* Eip */
	movl	16(%esp), %ebx	/* Eflags */
	movl	%ebx, 20(%esp)
	movl	12(%esp), %ebx	/* Cs */
	movl	%ebx, 16(%esp)
	movl	%ebp, 12(%esp)
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
	leal	0x58(%esp), %eax
	pushl	%eax		/* TempEsp */
	pushl	%ss		/* TempSegSs */
	pushl	$0		/* DebugPointer */
	pushl	$3       	/* DebugArgMark (Exception number) */
	pushl	0x60(%esp)	/* DebugEip */
	pushl	%ebp		/* DebugEbp */

	/*
	 * Call KDB
	 */
	movl	%esp, %eax
	pushl	$1		/* FirstChance */
	pushl	%eax		/* Push a pointer to the trap frame */
	pushl	$0		/* Context */
	pushl	$0		/* PreviousMode (KernelMode) */
	pushl	$0		/* ExceptionRecord */
	call	_KdbEnterDebuggerException

	/*
	 * Pop the arguments and unused portions of the trap frame:
	 *   DebugEbp
	 *   DebugEip
	 *   DebugArgMark
	 *   DebugPointer
	 *   TempSegSs
	 *   TempEsp
	 */
	addl	$(11*4), %esp

	/*
	 * Restore/update debugging registers.
	 */
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
	addl	$8, %esp	/* PreviousMode, ExceptionList */
	popl	%fs		/* Fs */
	popl	%edi		/* Edi */
	popl	%esi		/* Esi */
	popl	%ebx		/* Ebx */
	popl	%ebp		/* Ebp */
	addl	$4, %esp	/* ErrorCode */

	/*
	 * Return to the caller.
	 */
	iret


.globl _KdbpStackSwitchAndCall@8
_KdbpStackSwitchAndCall@8:
        pushl   %ebp
        movl    %esp, %ebp
        
        movl    0x8(%esp), %eax         /* New stack */
        movl    0xC(%esp), %ecx         /* Function to call */
        movl    %esp, %edx              /* Old stack */

        /* Switch stack */
        movl    %eax, %esp
        pushl   %edx

        /* Call function */
        call    *%ecx

        /* Switch back to old stack */
        popl    %esp

        /* Return */
        popl    %ebp
        ret     $8

