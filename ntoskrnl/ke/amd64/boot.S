/*
 * FILE:            ntoskrnl/ke/amd64/boot.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         FreeLDR Wrapper Bootstrap Code and Bootstrap Trampoline
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ksamd64.inc>

EXTERN KiSystemStartupBootStack:PROC

/* GLOBALS *******************************************************************/


/* FUNCTIONS *****************************************************************/

.code64

/**
 * VOID
 * KiSwitchToBootStack(
 *     IN ULONG_PTR InitialStack<rcx>)
 */
PUBLIC KiSwitchToBootStack
.PROC KiSwitchToBootStack

    /* CRITICAL FIX: In UEFI mode, we're already in long mode with flat segments
     * We don't need to change segments at all - just switch the stack pointer
     * The problem was trying to change SS which causes a fault in UEFI mode */
    
    /* Simply set the new stack pointer */
    mov rsp, rcx
    
    /* Align stack and reserve space */
    and rsp, HEX(FFFFFFFFFFFFFFF0)  // Align to 16 bytes
    sub rsp, HEX(28)                 // Reserve shadow space for Windows x64 ABI
    .ENDPROLOG

    /* Jump to the boot stack initialization routine */
    jmp KiSystemStartupBootStack

.ENDP

END

