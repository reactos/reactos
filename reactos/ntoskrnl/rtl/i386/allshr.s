/* $Id: allshr.s,v 1.1 2003/06/11 12:28:21 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Math support for IA-32
 * FILE:              ntoskrnl/rtl/i386/allshr.s
 * PROGRAMER:         Eric Kohl (ekohl@rz-online.de)
 */

/*
 * long long
 * __allshr(long long Value, unsigned char Shift);
 *
 * Parameters:
 *   EDX:EAX - signed long long value to be shifted right
 *   CL      - number of bits to shift by
 * Registers:
 *   Destroys CL
 * Returns:
 *   EDX:EAX - shifted value
 */
.globl __allshr
__allshr:
	shrdl	%cl, %edx, %eax
	sarl	%cl, %edx
	andl	$32, %ecx
	je		L1
	movl	%edx, %eax
	sarl	$31, %edx
L1:
	ret

/* EOF */
