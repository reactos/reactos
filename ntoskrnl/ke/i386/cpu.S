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

.globl _KiCoprocessorError@0
.func KiCoprocessorError@0
_KiCoprocessorError@0:

    /* Get the NPX Thread's Initial stack */
    mov eax, [fs:KPCR_NPX_THREAD]
    mov eax, [eax+KTHREAD_INITIAL_STACK]

    /* Make space for the FPU Save area */
    sub eax, SIZEOF_FX_SAVE_AREA

    /* Set the CR0 State */
    mov dword ptr [eax+FN_CR0_NPX_STATE], 8

    /* Update it */
    mov eax, cr0
    or eax, 8
    mov cr0, eax

    /* Return to caller */
    ret
.endfunc
