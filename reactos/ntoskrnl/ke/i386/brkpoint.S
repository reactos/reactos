/* $Id: brkpoint.c 15164 2005-05-09 01:38:29Z sedwards $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/brkpoint.c
 * PURPOSE:         Handles breakpoints
 *
 * PROGRAMMERS:     No programmer listed.
 */

.global _DbgBreakPoint@0
_DbgBreakPoint@0:
.global _DbgBreakPointNoBugCheck@0
_DbgBreakPointNoBugCheck@0:
	int $3
	ret

.global _DbgBreakPointWithStatus@4
_DbgBreakPointWithStatus@4:
	mov 0x4(%esp), %eax
	int $3
	ret $4
