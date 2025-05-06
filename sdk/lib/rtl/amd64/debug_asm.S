/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           Debug Routines
 * FILE:              lib/rtl/amd64/debug_asm.S
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <asm.inc>

/* FUNCTIONS ***************************************************************/

.code64

PUBLIC DbgUserBreakPoint
DbgUserBreakPoint:
PUBLIC DbgBreakPoint
.PROC DbgBreakPoint
    .endprolog
    int 3
    ret
.ENDP

PUBLIC RtlpBreakWithStatusInstruction
RtlpBreakWithStatusInstruction:
PUBLIC DbgBreakPointWithStatus
.PROC DbgBreakPointWithStatus
    .endprolog
    int 3
    ret
.ENDP

PUBLIC DebugService2
DebugService2:

    /* Pass the service number in eax */
    mov rax, r8
    int HEX(2D)
    int 3
    ret


/******************************************************************************
 * NTSTATUS NTAPI DebugService(
 *     IN ULONG Service,    // <rcx> = [rsp + 8]
 *     IN PVOID Buffer,     // <rdx> = [rsp + 16]
 *     IN ULONG Length,     // <r8>  = [rsp + 24]
 *     IN PVOID Argument1,  // <r9>  = [rsp + 32]
 *     IN PVOID Argument2); //         [rsp + 40]
 */
PUBLIC DebugService
DebugService:

    /* Prepare registers for interrupt */
    mov eax, ecx       // Service
    mov rcx, rdx       // Buffer
    mov edx, r8d       // Length
    mov r8, r9         // Argument1
    mov r9, [rsp + 40] // Argument2

    /* Call the Interrupt */
    int HEX(2D)
    int 3

   /* Return */
    ret

END
