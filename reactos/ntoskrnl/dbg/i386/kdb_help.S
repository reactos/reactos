#include <internal/ke.h>
#include <internal/i386/segment.h>

	.data
_KdbEipTemp:
	.int	0

	.text
.globl _KdbEnter	
_KdbEnter:
	/*
	 * Record when we are inside the debugger.
	 */
	incl	_KdbEntryCount

	/*
	 * Save the callers eip.
	 */
	popl	_KdbEipTemp
		
	/*
	 * Set up a trap frame
	 */
	pushfl			/* Eflags */
	pushl	%cs		/* Cs */
	pushl	_KdbEipTemp	/* Eip */
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
	pushl	$0		/* TempEip */
	pushl	$0		/* TempCs */
	pushl	$0		/* DebugPointer */
	pushl	$0		/* DebugArgMark */
	pushl	_KdbEipTemp	/* DebugEip */
	pushl	%ebp		/* DebugEbp */

	/*
	 * Push a pointer to the trap frame
	 */
	pushl	%esp

	/*
	 * Call KDB
	 */
	call	_KdbInternalEnter

	/*
	 * Pop the argument
	 */
	popl	%eax	

	/*
	 * Ignore unused portions of the trap frame.
	 */
	popl	%eax		/* DebugEbp */
	popl	%eax		/* DebugEip */
	popl	%eax		/* DebugArgMark */
	popl	%eax		/* DebugPointer */
	popl	%eax		/* TempCs */
	popl	%eax		/* TempEip */
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
	addl	$4, %esp	/* PreviousMode */
	addl	$4, %esp	/* ExceptionList */
	popl	%fs		/* Fs */
	popl	%edi		/* Edi */
	popl	%esi		/* Esi */
	popl	%ebx		/* Ebx */
	popl	%ebp		/* Ebp */
	addl	$4, %esp	/* ErrorCode */

	/*
	 * Record when we are in the debugger.
	 */
	decl	_KdbEntryCount

	/*
	 * Return to the caller.
	 */
	iret



	
