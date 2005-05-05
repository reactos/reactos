/* $Id: alldiv.s 12852 2005-01-06 13:58:04Z mf $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Math support for IA-32
 * FILE:              ntoskrnl/rtl/i386/alldiv.s
 * PROGRAMER:         Eric Kohl (ekohl@rz-online.de)
 */

/*
 * long long
 * __alldiv(long long Dividend, long long Divisor);
 *
 * Parameters:
 *   [ESP+04h] - long long Dividend
 *   [ESP+0Ch] - long long Divisor
 * Registers:
 *   Unknown
 * Returns:
 *   EDX:EAX - long long quotient (Dividend/Divisor)
 * Notes:
 *   Routine removes the arguments from the stack.
 */
.globl __alldiv
__alldiv:
	call	___divdi3
	ret		$0x10

/*
__alldiv:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%eax
	pushl	%eax
	movl	20(%ebp), %eax
	pushl	%eax
	movl	16(%ebp), %eax
	pushl	%eax
	movl	12(%ebp), %eax
	pushl	%eax
	movl	8(%ebp), %eax
	pushl	%eax
	call	___divdi3
	addl	$16, %esp
	movl	%ebp, %esp
	popl	%ebp
	ret
*/

/* EOF */
