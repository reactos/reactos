/* $Id: aulldiv.s,v 1.1 2003/06/11 15:37:15 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Math support for IA-32
 * FILE:              lib/ntdll/rtl/i386/aulldiv.s
 * PROGRAMER:         Eric Kohl (ekohl@rz-online.de)
 */

/*
 * unsigned long long
 * __aulldiv(unsigned long long Dividend, unsigned long long Divisor);
 *
 * Parameters:
 *   [ESP+04h] - unsigned long long Dividend
 *   [ESP+0Ch] - unsigned long long Divisor
 * Registers:
 *   Unknown
 * Returns:
 *   EDX:EAX - unsigned long long quotient (Dividend/Divisor)
 * Notes:
 *   Routine removes the arguments from the stack.
 */
.globl __aulldiv
__aulldiv:
	call	___udivdi3
	ret	$16

/* EOF */
