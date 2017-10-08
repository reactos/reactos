/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           Debug Routines
 * FILE:              lib/rtl/i386/debug_asm.S
 * PROGRAMER:         Alex Ionescu (alex@relsoft.net)
 */

#include <asm.inc>

/* FUNCTIONS ***************************************************************/

.code

PUBLIC _DbgBreakPointNoBugCheck@0
FUNC _DbgBreakPointNoBugCheck@0
    FPO 0, 0, 0, 0, 0, FRAME_FPO

    /* Do breakpoint */
    int 3
    ret

ENDFUNC


PUBLIC _DbgUserBreakPoint@0
_DbgUserBreakPoint@0:
PUBLIC _DbgBreakPoint@0
FUNC _DbgBreakPoint@0
    FPO 0, 0, 0, 0, 0, FRAME_FPO

    /* Do breakpoint */
    int 3
    ret

ENDFUNC


PUBLIC _DbgBreakPointWithStatus@4
FUNC _DbgBreakPointWithStatus@4
    FPO 0, 1, 0, 0, 0, FRAME_FPO

    /* Put Status in EAX */
    mov eax, [esp+4]

PUBLIC _RtlpBreakWithStatusInstruction@0
GLOBAL_LABEL _RtlpBreakWithStatusInstruction@0

    /*
     * Do a "labeled" breakpoint -- the KD data block has a "BreakpointWithStatus" field
     * pointing to this label, letting a debugger easily check that a breakpoint has occured here
     * and thereby know that there is a Status for it to retrieve from EAX
     *
     * In other words, Status is passed as an argument directly to the debugger
     */
    int 3
    ret 4

ENDFUNC


PUBLIC _DebugService2@12
FUNC _DebugService2@12
    FPO 0, 3, 3, 0, 1, FRAME_NONFPO

    /* Set up the stack */
    push ebp
    CFI_ADJUST_CFA_OFFSET 4
    CFI_REL_OFFSET ebp, 0
    mov ebp, esp
    CFI_DEF_CFA_REGISTER ebp

    /* Call the interrupt */
    mov eax, [ebp+16]
    mov ecx, [ebp+8]
    mov edx, [ebp+12]
    int HEX(2D)
    int 3

    /* Return */
    pop ebp
    CFI_SAME_VALUE ebp
    CFI_DEF_CFA esp, 12
    ret 12

ENDFUNC


PUBLIC _DebugService@20
FUNC _DebugService@20
    FPO 0, 5, 3, 0, 1, FRAME_NONFPO

    /* Set up the stack */
    push ebp
    CFI_ADJUST_CFA_OFFSET 4
    CFI_REL_OFFSET ebp, 0
    mov ebp, esp
    CFI_DEF_CFA_REGISTER ebp

    /* Save non-volatiles */
    push ebx
    CFI_REL_OFFSET ebx, -4
    push edi
    CFI_REL_OFFSET edi, -8

    /* Call the Interrupt */
    mov eax, [ebp+8]
    mov ecx, [ebp+12]
    mov edx, [ebp+16]
    mov ebx, [ebp+20]
    mov edi, [ebp+24]
    int HEX(2D)
    int 3

    /* Restore non-volatiles */
    pop edi
    CFI_SAME_VALUE edi
    pop ebx
    CFI_SAME_VALUE ebx

    /* Return */
    pop ebp
    CFI_SAME_VALUE ebp
    CFI_DEF_CFA esp, 20
    ret 20

ENDFUNC

END
