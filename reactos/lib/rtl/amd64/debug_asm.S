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
    ret
    /* Call the interrupt */
//    mov eax, [rbp+8]
//    int 0x2D
//    int 3

.endfunc

/******************************************************************************
 * NTSTATUS NTAPI DebugService(
 *     IN ULONG Service,    // <rcx> = [rsp + 8]
 *     IN PVOID Buffer,     // <rdx> = [rsp + 16]
 *     IN ULONG Length,     // <r8>  = [rsp + 24]
 *     IN PVOID Argument1,  // <r9>  = [rsp + 32]
 *     IN PVOID Argument2); //         [rsp + 40]
 */
.func DebugService
_DebugService:

    /* Prepare registers for interrupt */
    mov eax, ecx       // Service
    mov rcx, rdx       // Buffer
    mov edx, r8d       // Length
    mov r8, r9         // Argument1
    mov r9, [rsp + 40] // Argument2

    /* Call the Interrupt */
    int 0x2D
    int 3

   /* Return */
    ret
.endfunc
