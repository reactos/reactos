/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           Debug Routines
 * FILE:              lib/rtl/i386/debug.S
 * PROGRAMER:         Alex Ionescu (alex@relsoft.net)
 */

.intel_syntax noprefix

/* GLOBALS ****************************************************************/

.globl _DbgBreakPoint
.globl _DbgBreakPointWithStatus
.globl _DbgUserBreakPoint
.globl _DebugService
.globl _DebugService2
.globl _DbgBreakPointNoBugCheck
.globl _RtlpBreakWithStatusInstruction

/* FUNCTIONS ***************************************************************/

.func DbgBreakPointNoBugCheck
_DbgBreakPointNoBugCheck:
    int 3
    ret
.endfunc

.func DbgBreakPoint
_DbgBreakPoint:
_DbgUserBreakPoint:
    int 3
    ret
.endfunc

.func DbgBreakPointWithStatus
_DbgBreakPointWithStatus:
    mov eax, ecx

_RtlpBreakWithStatusInstruction:
    int 3
    ret
.endfunc

.func DebugService2
_DebugService2:
    /* Call the interrupt */
//    mov eax, [rbp+8]
//    int 0x2D
    int 3

.endfunc

.func DebugService
_DebugService:

    /* Save the registers */
    push rcx
    push rbx
    push rdi
    push rdi
    push rbx

    /* Call the Interrupt */
    // FIXME: setup registers
    int 0x2D
    int 3

    /* Restore registers */
    pop rbx
    pop rdi
    pop rdi
    pop rbx

    /* Return */
    ret 20
.endfunc
