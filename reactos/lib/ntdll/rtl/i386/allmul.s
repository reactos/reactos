/* $Id: allmul.s,v 1.1 2003/06/11 15:37:15 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Math support for IA-32
 * FILE:              lib/ntdll/rtl/i386/allmul.s
 * PROGRAMER:         Eric Kohl (ekohl@rz-online.de)
 */

/*
 * long long
 * __allmul(long long Multiplier, long long Multiplicand);
 *
 * Parameters:
 *   [ESP+04h] - long long Multiplier
 *   [ESP+0Ch] - long long Multiplicand
 * Registers:
 *   Unknown
 * Returns:
 *   EDX:EAX - long long product (Multiplier*Multiplicand)
 * Notes:
 *   Routine removes the arguments from the stack.
 */
.globl __allmul
__allmul:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$12, %esp
	movl	16(%ebp), %ebx
	movl	8(%ebp), %eax
	mull	%ebx
	movl	20(%ebp), %ecx
	movl	%eax, -24(%ebp)
	movl	8(%ebp), %eax
	movl	%edx, %esi
	imull	%ecx, %eax
	addl	%eax, %esi
	movl	12(%ebp), %eax
	imull	%eax, %ebx
	leal	(%ebx,%esi), %eax
	movl	%eax, -20(%ebp)
	movl	-24(%ebp), %eax
	movl	-20(%ebp), %edx
	addl	$12, %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret		$0x10

/* EOF */
