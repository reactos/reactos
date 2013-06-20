/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           Debug Routines
 * FILE:              lib/rtl/i386/debug.S
 * PROGRAMER:         Alex Ionescu (alex@relsoft.net)
 */

#include <asm.inc>

/* GLOBALS ****************************************************************/

PUBLIC _DbgBreakPoint@0
PUBLIC _DbgBreakPointWithStatus@4
PUBLIC _DbgUserBreakPoint@0
PUBLIC _DebugService@20
PUBLIC _DebugService2@12
PUBLIC _DbgBreakPointNoBugCheck@0
PUBLIC _RtlpBreakWithStatusInstruction@0

/* FUNCTIONS ***************************************************************/

.code

FUNC _DbgBreakPointNoBugCheck@0
    FPO 0, 0, 0, 0, 0, FRAME_FPO
    int 3
    ret
ENDFUNC

_DbgUserBreakPoint@0:
FUNC _DbgBreakPoint@0
    FPO 0, 0, 0, 0, 0, FRAME_FPO
    int 3
    ret
ENDFUNC

FUNC _DbgBreakPointWithStatus@4
    FPO 0, 1, 0, 0, 0, FRAME_FPO
    mov eax, [esp+4]
ENDFUNC

FUNC _RtlpBreakWithStatusInstruction@0
    FPO 0, 0, 0, 0, 0, FRAME_FPO
    int 3
    ret 4
ENDFUNC

FUNC _DebugService2@12
    FPO 0, 3, 3, 0, 1, FRAME_NONFPO

    /* Setup the stack */
    push ebp
    mov ebp, esp

    /* Call the interrupt */
    mov eax, [ebp+16]
    mov ecx, [ebp+8]
    mov edx, [ebp+12]
    int HEX(2D)
    int 3

    /* Restore stack */
    pop ebp
    ret 12
ENDFUNC

FUNC _DebugService@20
    FPO 0, 5, 3, 0, 1, FRAME_NONFPO

    /* Setup the stack */
    push ebp
    mov ebp, esp

    /* Save the registers */
    push ebx
    push edi

    /* Call the Interrupt */
    mov eax, [ebp+8]
    mov ecx, [ebp+12]
    mov edx, [ebp+16]
    mov ebx, [ebp+20]
    mov edi, [ebp+24]
    int HEX(2D)
    int 3

    /* Restore registers */
    pop edi
    pop ebx

    /* Return */
    pop ebp
    ret 20
ENDFUNC

END
