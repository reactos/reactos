/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           Debug Routines
 * FILE:              lib/rtl/i386/debug.S
 * PROGRAMER:         Alex Ionescu (alex@relsoft.net)
 */

.intel_syntax noprefix

/* GLOBALS ****************************************************************/

.globl _DbgBreakPoint@0
.globl _DbgBreakPointWithStatus@4
.globl _DbgUserBreakPoint@0
.globl _DebugService@20
.globl _DebugService2@12
.globl _DbgBreakPointNoBugCheck@0
.globl _RtlpBreakWithStatusInstruction@0

/* FUNCTIONS ***************************************************************/

.func DbgBreakPointNoBugCheck@0
_DbgBreakPointNoBugCheck@0:
    int 3
    ret
.endfunc

.func DbgBreakPoint@0
_DbgBreakPoint@0:
_DbgUserBreakPoint@0:
    int 3
    ret
.endfunc

.func DbgBreakPointWithStatus@4
_DbgBreakPointWithStatus@4:
    mov eax, [esp+4]

_RtlpBreakWithStatusInstruction@0:
    int 3
    ret 4
.endfunc

.func DebugService2@12
_DebugService2@12:

    /* Setup the stack */
    push ebp
    mov ebp, esp

    /* Call the interrupt */
    mov eax, [ebp+16]
    mov ecx, [ebp+8]
    mov edx, [ebp+12]
    int 0x2D
    int 3

    /* Restore stack */
    pop ebp
    ret 12
.endfunc

.func DebugService@20
_DebugService@20:

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
    int 0x2D
    int 3
 
    /* Restore registers */
    pop edi
    pop ebx
 
    /* Return */
    pop ebp
    ret 20
.endfunc
