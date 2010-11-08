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

#include <ndk/asm.h>
.intel_syntax noprefix

#define Ready 1
#define Running 2
#define WrDispatchInt 0x1F

/* FUNCTIONS ****************************************************************/

/*++
 * KiSwapContextInternal 
 *
 *     The KiSwapContextInternal routine switches context to another thread.
 *
 * Params:
 *     ESI - Pointer to the KTHREAD to which the caller wishes to
 *           switch to.
 *     EDI - Pointer to the KTHREAD to which the caller wishes to
 *           switch from.
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     Absolutely all registers except ESP can be trampled here for maximum code flexibility.
 *
 *--*/
.globl @KiSwapContextInternal@0
.func @KiSwapContextInternal@0, @KiSwapContextInternal@0
@KiSwapContextInternal@0:
    /* Set APC Bypass Disable and old thread pointer */
    mov edx, edi
    or dl, cl

    /* Build switch frame */
    sub esp, 2 * 4
    mov ecx, esp
    call @KiSwapContextEntry@8
    mov ecx, 0xB00BFACA
    jmp $
.endfunc

/*++
 * KiSwapContext 
 *
 *     The KiSwapContext routine switches context to another thread.
 *
 * Params:
 *     TargetThread - Pointer to the KTHREAD to which the caller wishes to
 *                    switch to.
 *
 * Returns:
 *     The WaitStatus of the Target Thread.
 *
 * Remarks:
 *     This is a wrapper around KiSwapContextInternal which will save all the
 *     non-volatile registers so that the Internal function can use all of
 *     them. It will also save the old current thread and set the new one.
 *
 *     The calling thread does not return after KiSwapContextInternal until 
 *     another thread switches to IT.
 *
 *--*/
.globl @KiSwapContext@8
.func @KiSwapContext@8, @KiSwapContext@8
@KiSwapContext@8:

    /* Save 4 registers */
    sub esp, 4 * 4

    /* Save all the non-volatile ones */
    mov [esp+12], ebx
    mov [esp+8], esi
    mov [esp+4], edi
    mov [esp+0], ebp

    /* Get the current KPCR */
    mov ebx, fs:[KPCR_SELF]

    /* Get the Current Thread */
    mov edi, ecx

    /* Get the New Thread */
    mov esi, edx

    /* Get the wait IRQL */
    movzx ecx, byte ptr [edi+KTHREAD_WAIT_IRQL]

    /* Do the swap with the registers correctly setup */
    call @KiSwapContextInternal@0

    /* Return the registers */
    mov ebp, [esp+0]
    mov edi, [esp+4]
    mov esi, [esp+8]
    mov ebx, [esp+12]

    /* Clean stack */
    add esp, 4 * 4
    ret
.endfunc

.globl @KiRetireDpcListInDpcStack@8
.func @KiRetireDpcListInDpcStack@8, @KiRetireDpcListInDpcStack@8
@KiRetireDpcListInDpcStack@8:

    /* Switch stacks and retire DPCs */
    mov eax, esp
    mov esp, edx
    push eax
    call @KiRetireDpcList@4

    /* Return on original stack */
    pop esp
    ret
.endfunc

/* FIXFIX: Move to C code ****/
.globl _Ki386SetupAndExitToV86Mode@4
.func Ki386SetupAndExitToV86Mode@4
_Ki386SetupAndExitToV86Mode@4:

    /* Enter V8086 mode */
    pushad
    sub esp, (12 + KTRAP_FRAME_LENGTH + NPX_FRAME_LENGTH)
    mov ecx, esp
    call @KiEnterV86Mode@4
    jmp $
.endfunc

.globl @KiSwitchThreads@8
.func @KiSwitchThreads@8, @KiSwitchThreads@8
@KiSwitchThreads@8:

    /* Load the new kernel stack and switch OS to new thread */
    mov esp, [edx+KTHREAD_KERNEL_STACK]
    mov edx, esp
    call @KiSwapContextExit@8
    
    /* Now we're on the new thread. Return to the caller to restore registers */
    add esp, 2 * 4
    ret
.endfunc

.globl @Ki386BiosCallReturnAddress@4
@Ki386BiosCallReturnAddress@4:

    /* Exit V8086 mode */
    call @KiExitV86Mode@4
    mov esp, eax
    add esp, (12 + KTRAP_FRAME_LENGTH + NPX_FRAME_LENGTH)
    popad
    ret 4

