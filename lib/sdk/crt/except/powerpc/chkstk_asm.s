/* $Id: chkstk_asm.s 26099 2007-03-14 20:30:32Z ion $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Stack checker
 * FILE:              lib/ntdll/rtl/i386/chkstk.s
 * PROGRAMER:         arty
 */

.globl _chkstk
.globl _alloca_probe

/*
 _chkstk() is called by all stack allocations of more than 4 KB. It grows the
 stack in areas of 4 KB each, trying to access each area. This ensures that the
 guard page for the stack is hit, and the stack growing triggered
 */
_chkstk:
_alloca_probe:
        /* return */
        blr

/* EOF */
