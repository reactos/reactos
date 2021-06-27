/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/ctxswitch.S
 * PURPOSE:         Thread Context Switching
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Gregor Anich (FPU Code)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ks386.inc>

EXTERN @KiSwapContextEntry@8:PROC
EXTERN @KiSwapContextExit@8:PROC
EXTERN @KiRetireDpcList@4:PROC
EXTERN @KiEnterV86Mode@4:PROC
EXTERN @KiExitV86Mode@4:PROC
EXTERN _KeI386FxsrPresent:DWORD

/* FUNCTIONS ****************************************************************/
.code

/*++
 * KiSwapContextInternal
 *
 * \brief
 *     The KiSwapContextInternal routine switches context to another thread.
 *
 * BOOLEAN USERCALL KiSwapContextInternal(ULONG_PTR OldThreadAndApcFlag@<edx>);
 *
 * \param OldThreadAndApcFlag@<edx>
 *     Pointer to the current thread with the lowest bit set to the current IRQL.
 *
 * \returns
 *     APC state.
 *
 * \remarks
 *     Absolutely all registers except ESP can be trampled here for maximum code flexibility.
 *
 *--*/
PUBLIC @KiSwapContextInternal@0
@KiSwapContextInternal@0:
    /* Build switch frame */
    sub esp, 2 * 4
    mov ecx, esp
    jmp @KiSwapContextEntry@8


/**
 * KiSwapContext
 *
 * \brief
 *     The KiSwapContext routine switches context to another thread.
 *
 * BOOLEAN FASTCALL
 * KiSwapContext(KIRQL WaitIrql@<cl>, PKTHREAD CurrentThread@<edx>);
 *
 * \param WaitIrql@<cl>
 *     The IRQL at which the wait happens.
 *
 * \param CurrentThread@<edx>
 *     Pointer to the KTHREAD of the current thread.
 *
 * \returns
 *     The WaitStatus of the Target Thread.
 *
 * \remarks
 *     This is a wrapper around KiSwapContextInternal which will save all the
 *     non-volatile registers so that the Internal function can use all of
 *     them. It will also save the old current thread and set the new one.
 *
 *     The calling thread does not return after KiSwapContextInternal until
 *     another thread switches to IT.
 *
 *--*/
PUBLIC @KiSwapContext@8
@KiSwapContext@8:
    /* Save 4 registers */
    sub esp, 4 * 4

    /* Save all the non-volatile ones */
    mov [esp+12], ebx
    mov [esp+8], esi
    mov [esp+4], edi
    mov [esp+0], ebp

    /* Combine current thread and the wait IRQL in edx */
    or dl, cl

    /* Do the swap with the registers correctly setup */
    call @KiSwapContextInternal@0

    /* Restore the registers */
    mov ebp, [esp+0]
    mov edi, [esp+4]
    mov esi, [esp+8]
    mov ebx, [esp+12]

    /* Clean stack */
    add esp, 4 * 4
    ret


PUBLIC @KiSwitchThreads@8
@KiSwitchThreads@8:
    /* Load the new kernel stack and switch OS to new thread */
    mov esp, edx
#if DBG
    /* Restore the frame pointer early to get sensible backtraces */
    mov ebp, [esp+12]
#endif
    call @KiSwapContextExit@8

    /* Now we're on the new thread. Return to the caller to restore registers */
    add esp, 2 * 4
    ret


PUBLIC @KiRetireDpcListInDpcStack@8
@KiRetireDpcListInDpcStack@8:
    /* Switch stacks and retire DPCs */
    mov eax, esp
    mov esp, edx
    push eax
    call @KiRetireDpcList@4

    /* Return on original stack */
    pop esp
    ret

PUBLIC _Ki386EnableCurrentLargePage@8
_Ki386EnableCurrentLargePage@8:
    /* Save StartAddress in eax */
    mov eax, [esp + 4]

    /* Save new CR3 value in ecx */
    mov ecx, [esp + 8]

    /* Save flags value */
    pushfd

    /* Disable interrupts */
    cli

    /* Compute linear address */
    sub eax, offset _Ki386EnableCurrentLargePage@8
    add eax, offset _Ki386LargePageIdentityLabel

    /* Save old CR3 in edx and replace with a new one */
    mov edx, cr3
    mov cr3, ecx

    /* Jump to the next instruction but in linear mapping */
    jmp eax

_Ki386LargePageIdentityLabel:
    /* Disable paging */
    mov eax, cr0
    and eax, NOT CR0_PG
    mov cr0, eax

    /* Jump to the next instruction to clear the prefetch queue */
    jmp $+2

    /* Enable Page Size Extension in CR4 */
    mov ecx, cr4
    or ecx, CR4_PSE
    mov cr4, ecx

    /* Done, now re-enable paging */
    or eax, CR0_PG
    mov cr0, eax

    /* Jump to virtual address */
    mov eax, offset VirtualSpace
    jmp eax

VirtualSpace:
    /* Restore CR3 contents */
    mov cr3, edx

    /* Restore flags */
    popfd

    ret 8

/* FIXFIX: Move to C code ****/
PUBLIC _Ki386SetupAndExitToV86Mode@4
_Ki386SetupAndExitToV86Mode@4:

    /* Enter V8086 mode */
    pushad
    sub esp, (12 + KTRAP_FRAME_LENGTH + NPX_FRAME_LENGTH + 16)
    mov ecx, esp
    call @KiEnterV86Mode@4
    jmp $


PUBLIC @Ki386BiosCallReturnAddress@4
@Ki386BiosCallReturnAddress@4:

    /* Exit V8086 mode */
    call @KiExitV86Mode@4
    mov esp, eax
    add esp, (12 + KTRAP_FRAME_LENGTH + NPX_FRAME_LENGTH + 16)
    popad
    ret 4

PUBLIC _FrRestore
PUBLIC @Ke386LoadFpuState@4
@Ke386LoadFpuState@4:

    /* Check if we have FXSR and choose which operand to use */
    test byte ptr [_KeI386FxsrPresent], 1
    jz _FrRestore

    /* Restore all the FPU, MMX, XMM and MXCSR registers */
    fxrstor [ecx]
    ret

    /*
     * Just restore the basic FPU registers.
     * This may raise an exception depending
     * on the status word, which KiNpxHandler will
     * need to check for and handle during delayed load
     * to avoid raising an unhandled exception
     * and crashing the system.
     */
_FrRestore:
    frstor [ecx]
    ret

END
