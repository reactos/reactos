/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           Debug Routines
 * FILE:              lib/rtl/arm/debug_asm.S
 */

/* GLOBALS ********************************************************************/

.globl DbgBreakPoint

/* FUNCTIONS ******************************************************************/

.func DbgBreakPoint
DbgBreakPoint:
    bkpt 3
    bx lr
.endfunc
