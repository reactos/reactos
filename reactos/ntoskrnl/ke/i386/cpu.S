/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ke/i386/cpu.S
 * PURPOSE:         Handles CPU-centric operations: TLB Flushes, Breakpoints, FPU
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
.intel_syntax noprefix

/* GLOBALS ****************************************************************/  


/* FUNCTIONS ****************************************************************/

.global _DbgBreakPointNoBugCheck@0
.func DbgBreakPointNoBugCheck@0
_DbgBreakPointNoBugCheck@0:
    int 3
    ret
.endfunc

.globl _KeFlushCurrentTb@0
.func KeFlushCurrentTb@0
_KeFlushCurrentTb@0:
    /* Check for global page support */
    test byte ptr [_Ke386GlobalPagesEnabled], 0xff
    jz .L1

    /* Modifying the PSE, PGE or PAE Flag in CR4 causes the TLB to be flushed */
    mov eax, cr4
.att_syntax /* Older binutils versions don't support ~ for .intel_syntax */
    and $~CR4_PGE, %eax
.intel_syntax noprefix
    mov cr4, eax
    or eax, CR4_PGE
    mov cr4, eax
    ret

.L1:
    /* the old way ... */
    mov eax, cr3
    mov cr3, eax
    ret
.endfunc
