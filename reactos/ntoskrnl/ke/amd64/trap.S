/*
 * FILE:            ntoskrnl/ke/amd64/trap.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Traps, Entrypoints and Exitpoints
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 * NOTE:            See asmmacro.S for the shared entry/exit code.
 */

/* INCLUDES ******************************************************************/

//#include <asm.h>
//#include <internal/amd64/asmmacro.S>
.intel_syntax noprefix
.code64

/* GLOBALS *******************************************************************/

.data

_MsgPageFault:
.ascii "Page fault 0x%x at %p!\n\0"

_MsgGeneralProtFault:
.ascii "General protection fault at %p!\n\0"

_MsgBreakpointTrap:
.ascii "BreakpointTrap at %p\n\0"

_MsgUnexpectedInterrupt:
.ascii "UnexpectedInterrupt\n\0"


/* SOFTWARE INTERRUPT SERVICES ***********************************************/
.text
.code64

.global _KiDivideErrorFault
_KiDivideErrorFault:
    /* Push pseudo error code */
    push 0


.global _KiDebugTrapOrFault
_KiDebugTrapOrFault:
    /* Push pseudo error code */
    push 0

.global _KiNmiInterrupt
_KiNmiInterrupt:
    /* Push pseudo error code */
    push 0

jmp $

.global _KiBreakpointTrap
_KiBreakpointTrap:
    /* Push pseudo error code */
    push 0

    push rax
    push rcx
    push rdx

    sub rsp, 0x10
    movabs rcx, offset _MsgBreakpointTrap
    mov rdx, [rsp + 0x10 + 24 + 8]
    movabs rax, offset _FrLdrDbgPrint
    call [rax]
    add rsp, 0x10

    pop rdx
    pop rcx
    pop rax
    add rsp, 8
    iretq

.global _KiOverflowTrap
_KiOverflowTrap:
    /* Push pseudo error code */
    push 0

.global _KiBoundFault
_KiBoundFault:
    /* Push pseudo error code */
    push 0

.global _KiInvalidOpcodeFault
_KiInvalidOpcodeFault:
    /* Push pseudo error code */
    push 0

.global _KiNpxNotAvailableFault
_KiNpxNotAvailableFault:
    /* Push pseudo error code */
    push 0

.global _KiDoubleFaultAbort
_KiDoubleFaultAbort:
    /* Push pseudo error code */
    push 0

.global _KiNpxSegmentOverrunAbort
_KiNpxSegmentOverrunAbort:
    /* Push pseudo error code */
    push 0

.global _KiInvalidTssFault
_KiInvalidTssFault:
    /* We have an error code */


.global _KiSegmentNotPresentFault
_KiSegmentNotPresentFault:
    /* We have an error code */


.global _KiStackFault
_KiStackFault:
    /* We have an error code */

jmp $


.global _KiGeneralProtectionFault
_KiGeneralProtectionFault:
    /* We have an error code */

    mov rdx, 0
    mov dx, ss
    movabs rcx, offset _MsgGeneralProtFault
    movabs rax, offset _FrLdrDbgPrint
    call [rax]
    jmp $


.global _KiPageFault
_KiPageFault:
    /* We have an error code */
    movabs rcx, offset _MsgPageFault
    mov rdx, [rsp]
    mov r8, [rsp+8]
    mov r9, rsp
    movabs rax, offset _FrLdrDbgPrint
    call [rax]
    jmp $


.global _KiFloatingErrorFault
_KiFloatingErrorFault:
    /* Push pseudo error code */
    push 0

.global _KiAlignmentFault
_KiAlignmentFault:
    /* We have an error code */

.global _KiMcheckAbort
_KiMcheckAbort:
    /* Push pseudo error code */
    push 0

.global _KiXmmException
_KiXmmException:
    /* Push pseudo error code */
    push 0

.global _KiApcInterrupt
_KiApcInterrupt:


.global _KiRaiseAssertion
_KiRaiseAssertion:
jmp $

.global _KiDebugServiceTrap
_KiDebugServiceTrap:
    /* Push pseudo error code */
    push 0

    push rax
    push rcx
    push rdx
    push r8
    push r9

    /* Create stack space for parameters */
    sub rsp, 0x18

    /* just forward first 3 parameters */
    call _KdpServiceDispatcher

    /* Skip the int 3, increment return rip */
    inc qword ptr [rsp + 0x18 + 48]

    /* Cleanup */
    add rsp, 0x18

    pop r9
    pop r8
    pop rdx
    pop rcx
    pop rax

    add rsp, 8
    iretq


.global _KiDpcInterrupt
_KiDpcInterrupt:


.global _KiIpiInterrupt
_KiIpiInterrupt:
jmp $


.global _KiUnexpectedInterrupt
_KiUnexpectedInterrupt:
    movabs rcx, offset _MsgUnexpectedInterrupt
    movabs rax, offset _FrLdrDbgPrint
    call [rax]
    jmp $
