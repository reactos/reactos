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

    /* Setup the new stack */
    mov ax, HEX(18)
    mov ss, ax
    mov rsp, rcx
    // Note: 8 bytes extra to compensate for the missing return address on
    // the stack. On function entry the stack is unaligned by 8!!
    sub rsp, HEX(308) // FIXME
    .ENDPROLOG

    jmp KiSystemStartupBootStack

.ENDP

END

