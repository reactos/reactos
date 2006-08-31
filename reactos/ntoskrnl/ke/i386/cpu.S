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

/* FUNCTIONS ****************************************************************/

.globl _KeFlushCurrentTb@0
.func KeFlushCurrentTb@0
_KeFlushCurrentTb@0:

    /* Check for global page support */
    test byte ptr [_Ke386GlobalPagesEnabled], 0xff
    jz .L1

    /* Modifying the PSE, PGE or PAE Flag in CR4 causes the TLB to be flushed */
    mov eax, cr4
    and eax, ~CR4_PGE
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


