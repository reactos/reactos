/* $Id: aullrem.s,v 1.1 2003/06/11 12:28:21 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Math support for IA-32
 * FILE:              ntoskrnl/rtl/i386/aullrem.s
 * PROGRAMER:         Eric Kohl (ekohl@rz-online.de)
 */

/*
 * unsigned long long
 * __aullrem(unsigned long long Dividend, unsigned long long Divisor);
 *
 * Parameters:
 *   [ESP+04h] - unsigned long long Dividend
 *   [ESP+0Ch] - unsigned long long Divisor
 * Registers:
 *   Unknown
 * Returns:
 *   EDX:EAX - unsigned long long remainder (Dividend%Divisor)
 * Notes:
 *   Routine removes the arguments from the stack.
 */
.globl __aullrem
__aullrem:
	call	___umoddi3
	ret	$16

/* EOF */
