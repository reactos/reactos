#include <internal/ke.h>
#include <internal/i386/segment.h>

.globl _KdbEnter	
_KdbEnter:
	/*
	 * Set up a stack frame
	 */ 
	pushl	%ebp
	movl	%esp, %ebp

	/*
	 * Save registers
	 */ 
	pushl	%edi
	pushl	%esi
	pushl	%ebx

	/*
	 * Set up a trap frame
	 */
	pushl	$0		/* V86_Gs */
	pushl	$0		/* V86_Fs */
	pushl	$0		/* V86_Ds */
	pushl	$0		/* V86_Es */
	pushl	%ss		/* Ss */
	pushl	%ebp            /* Esp */
	pushfl			/* Eflags */
	pushl	%cs		/* Cs */
	pushl	4(%ebp)		/* Eip */
	pushl	$0		/* ErrorCode */
	pushl	0(%ebp)		/* Ebp */
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
	pushl	$0		/* Dr7 */
	pushl	$0		/* Dr6 */
	pushl	$0		/* Dr3 */
	pushl	$0		/* Dr2 */
	pushl	$0		/* Dr1 */
	pushl	$0		/* Dr0 */
	pushl	$0		/* TempEip */
	pushl	$0		/* TempCs */
	pushl	$0		/* DebugPointer */
	pushl	$0		/* DebugArgMark */
	pushl	$0		/* DebugEip */
	pushl	$0		/* DebugEbp */

	/*
	 * Push a pointer to the trap frame
	 */
	pushl	%esp

	/*
	 * Call KDB
	 */
	call	_KdbInternalEnter

	/*
	 * Pop the argument and destroy the trap frame
	 */
	popl	%eax
	addl	$KTRAP_FRAME_SIZE, %esp

	/*
	 * Restore registers
	 */
	popl	%ebx
	popl	%esi
	popl	%edi

	/*
	 * Return
	 */
	popl	%ebp
	ret



	
