/* $Id: allshl.s,v 1.1 2003/06/11 12:28:21 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Math support for IA-32
 * FILE:              ntoskrnl/rtl/i386/allshl.s
 * PROGRAMER:         Eric Kohl (ekohl@rz-online.de)
 */

/*
 * long long
 * __allshl(long long Value, unsigned char Shift);
 *
 * Parameters:
 *   EDX:EAX - signed long long value to be shifted left
 *   CL      - number of bits to shift by
 * Registers:
 *   Destroys CL
 * Returns:
 *   EDX:EAX - shifted value
 */
.globl __allshl
__allshl:
	shldl	%cl, %eax, %edx
	sall	%cl, %eax
	andl	$32, %ecx
	je		L1
	movl	%eax, %edx
	xorl	%eax, %eax
L1:
	ret

/* EOF */
