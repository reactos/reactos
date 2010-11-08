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

/* FUNCTIONS ****************************************************************/

.globl @KiSwapContextInternal@0
.func @KiSwapContextInternal@0, @KiSwapContextInternal@0
@KiSwapContextInternal@0:
    /* Build switch frame */
    sub esp, 2 * 4
    mov ecx, esp
    jmp @KiSwapContextEntry@8
.endfunc

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

    /* Get the wait IRQL */
    or dl, cl

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

.globl @KiSwitchThreads@8
.func @KiSwitchThreads@8, @KiSwitchThreads@8
@KiSwitchThreads@8:
    /* Load the new kernel stack and switch OS to new thread */
    mov esp, edx
    call @KiSwapContextExit@8
    
    /* Now we're on the new thread. Return to the caller to restore registers */
    add esp, 2 * 4
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

.globl @Ki386BiosCallReturnAddress@4
@Ki386BiosCallReturnAddress@4:

    /* Exit V8086 mode */
    call @KiExitV86Mode@4
    mov esp, eax
    add esp, (12 + KTRAP_FRAME_LENGTH + NPX_FRAME_LENGTH)
    popad
    ret 4

