/* $Id: allrem.s,v 1.1 2003/06/11 15:37:15 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Math support for IA-32
 * FILE:              lib/ntdll/rtl/i386/math.s
 * PROGRAMER:         Eric Kohl (ekohl@rz-online.de)
 * NOTES:             This file is shared with ntoskrnl/rtl/i386/math.s.
 *                    Please keep the files synchronized!
 */

/*
 * long long
 * __allrem(long long Dividend, long long Divisor);
 *
 * Parameters:
 *   [ESP+04h] - long long Dividend
 *   [ESP+0Ch] - long long Divisor
 * Registers:
 *   Unknown
 * Returns:
 *   EDX:EAX - long long remainder (Dividend/Divisor)
 * Notes:
 *   Routine removes the arguments from the stack.
 */
.globl __allrem
__allrem:
	call	___moddi3
	ret		$16

/* EOF */
