/*
 * FILE:            ntoskrnl/ke/amd64/trap.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Traps, Entrypoints and Exitpoints
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 * NOTE:            See asmmacro.S for the shared entry/exit code.
 */

/* INCLUDES ******************************************************************/

//#include <asm.h>
#include <ndk/amd64/asmmacro.S>

/* GLOBALS *******************************************************************/

.data

.global _MsgUnimplemented
_MsgUnimplemented:
.asciz "WARNING:  %s at %s:%d is UNIMPLEMENTED!\n"

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

.proc KiDivideErrorFault
    .pushframe 0
    /* Push pseudo error code */
    push 0
    .allocstack 0x8

    UNIMPLEMENTED KiDivideErrorFault

    jmp $
.endproc

.proc KiDebugTrapOrFault
    .pushframe 0
    /* Push pseudo error code */
    push 0
    .allocstack 0x8

    UNIMPLEMENTED KiDebugTrapOrFault

    jmp $
.endproc

.proc KiNmiInterrupt
    .pushframe 0
    /* Push pseudo error code */
    push 0
    .allocstack 0x8

    UNIMPLEMENTED KiNmiInterrupt

    jmp $
.endproc

.proc KiBreakpointTrap
    .pushframe 0
    /* Push pseudo error code */
    push 0
    .allocstack 0x8

    push rax
    .pushreg rax
    push rcx
    .pushreg rcx
    push rdx
    sub rsp, 0x10
    .allocstack 0x10
    .endprolog

    lea rcx, _MsgBreakpointTrap[rip]
    mov rdx, [rsp + 0x10 + 24 + 8]
    lea rax, _FrLdrDbgPrint[rip]
    call [rax]

    /* Return */
    add rsp, 0x10
    pop rdx
    pop rcx
    pop rax
    add rsp, 8
    iretq
.endproc

.proc KiOverflowTrap
    .pushframe 0
    /* Push pseudo error code */
    push 0
    .allocstack 0x8

    UNIMPLEMENTED KiOverflowTrap
    jmp $
.endproc

.proc KiBoundFault
    .pushframe 0
    /* Push pseudo error code */
    push 0
    .allocstack 8

    sub rsp, 0x20
    .allocstack 0x20

    mov [rsp + 8], rbx
    .savereg rbx, 8

    UNIMPLEMENTED KiBoundFault

    jmp $
.endproc

.proc KiInvalidOpcodeFault
    .pushframe 0
    /* Push pseudo error code */
    push 0
    .allocstack 0x8

    UNIMPLEMENTED KiInvalidOpcodeFault

    jmp $
.endproc

.proc KiNpxNotAvailableFault
    .pushframe 0
    /* Push pseudo error code */
    push 0
    .allocstack 0x8

    UNIMPLEMENTED KiNpxNotAvailableFault

    jmp $
.endproc

.proc KiDoubleFaultAbort
    .pushframe 0
    /* Push pseudo error code */
    push 0
    .allocstack 0x8

    UNIMPLEMENTED KiDoubleFaultAbort

    jmp $
.endproc

.proc KiNpxSegmentOverrunAbort
    .pushframe 0
    /* Push pseudo error code */
    push 0
    .allocstack 0x8

    UNIMPLEMENTED KiNpxSegmentOverrunAbort

    jmp $
.endproc

.proc KiInvalidTssFault
    .pushframe 1
    /* We have an error code */

    UNIMPLEMENTED KiInvalidTssFault

    jmp $
.endproc


.proc KiSegmentNotPresentFault
    .pushframe 1
    /* We have an error code */

    UNIMPLEMENTED KiSegmentNotPresentFault

    jmp $
.endproc

.proc KiStackFault
    .pushframe 1
    /* We have an error code */

    UNIMPLEMENTED KiStackFault

    jmp $
.endproc


.proc KiGeneralProtectionFault
    .pushframe 1
    /* We have an error code */

    mov rdx, 0
    mov dx, ss
    lea rcx, _MsgGeneralProtFault[rip]
    lea rax, _FrLdrDbgPrint[rip]
    call [rax]

    jmp $
.endproc


.proc KiPageFault
    .pushframe 1
    /* We have an error code */

    lea rcx, _MsgPageFault[rip]
    mov rdx, [rsp]
    mov r8, [rsp+8]
    mov r9, rsp
    lea rax, _FrLdrDbgPrint[rip]
    call [rax]

    jmp $
.endproc


.proc KiFloatingErrorFault
    .pushframe 0
    /* Push pseudo error code */
    push 0
    .allocstack 0x8

    UNIMPLEMENTED KiFloatingErrorFault

    jmp $
.endproc

.proc KiAlignmentFault
    .pushframe 1
    /* We have an error code */

    UNIMPLEMENTED KiAlignmentFault

    jmp $
.endproc

.proc KiMcheckAbort
    .pushframe 0
    /* Push pseudo error code */
    push 0
    .allocstack 0x08

    UNIMPLEMENTED KiMcheckAbort

    jmp $
.endproc

.proc KiXmmException
    .pushframe 0
    /* Push pseudo error code */
    push 0
    .allocstack 0x08

    UNIMPLEMENTED KiXmmException

    jmp $
.endproc

.proc KiApcInterrupt
    .pushframe 1

    UNIMPLEMENTED KiApcInterrupt

    jmp $
.endproc

.proc KiRaiseAssertion
    .pushframe 1

    UNIMPLEMENTED KiRaiseAssertion
 
    jmp $
.endproc

.proc KiDebugServiceTrap
    .pushframe 0
    /* Push pseudo error code */
    push 0
    .allocstack 0x08

    push rax
    .pushreg rax
    push rcx
    .pushreg rcx
    push rdx
    .pushreg rdx
    push r8
    .pushreg r8
    push r9
    .pushreg r9

    /* Create stack space for parameters */
    sub rsp, 0x18
    .allocstack 0x18

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
.endproc


.proc KiDpcInterrupt
    .pushframe 1
    jmp $
.endproc


.proc KiIpiInterrupt
    .pushframe 1
    jmp $
.endproc


.proc KiUnexpectedInterrupt
    .pushframe 0
    push 0
    .allocstack 0x8

    lea rcx, _MsgUnexpectedInterrupt[rip]
    lea rax, _FrLdrDbgPrint[rip]
    call [rax]

    jmp $
.endproc



