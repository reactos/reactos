/*
 * FILE:            ntoskrnl/ke/amd64/trap.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Traps, Entrypoints and Exitpoints
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ndk/amd64/asm.h>
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

/* Helper Macros *************************************************************/

#define TRAPFLAG_SYSTEMSERVICE 1

.macro ENTER_TRAP_FRAME AllocSize Flags
.set SIZE_INITIAL_FRAME, 7 * 8
//.set SIZE_LOCAL_DATA, SIZE_EXCEPTION_RECORD + 0x28
.set SIZE_TRAP_FRAME_ALLOC, SIZE_KTRAP_FRAME - SIZE_INITIAL_FRAME + \AllocSize
.set TRAPFLAGS, \Flags

    /* Save rbp */
    push rbp
    .pushreg rbp

    /* Make room for a KTRAP_FRAME and function parameters */
    sub rsp, SIZE_TRAP_FRAME_ALLOC
    .allocstack SIZE_TRAP_FRAME_ALLOC

    /* Point rbp to the KTRAP_FRAME */
    lea rbp, [rsp + \AllocSize]

// KTRAP_FRAME_P1Home
// KTRAP_FRAME_P2Home
// KTRAP_FRAME_P3Home
// KTRAP_FRAME_P4Home
// KTRAP_FRAME_P5

.if (TRAPFLAGS & TRAPFLAG_SYSTEMSERVICE)
    /* Save non-volatile registers */
    mov [rbp + KTRAP_FRAME_Rbx], rbx
    mov [rbp + KTRAP_FRAME_Rdi], rdi
    mov [rbp + KTRAP_FRAME_Rsi], rsi
.else
    /* Save volatile registers */
    mov [rbp + KTRAP_FRAME_Rax], rax
    mov [rbp + KTRAP_FRAME_Rcx], rcx
    mov [rbp + KTRAP_FRAME_Rdx], rdx
    mov [rbp + KTRAP_FRAME_R8], r8
    mov [rbp + KTRAP_FRAME_R9], r9
    mov [rbp + KTRAP_FRAME_R10], r10
    mov [rbp + KTRAP_FRAME_R11], r11

    /* Save xmm registers */
//    movdqa [rbp + KTRAP_FRAME_Xmm0], xmm0
//    movdqa [rbp + KTRAP_FRAME_Xmm1], xmm1
//    movdqa [rbp + KTRAP_FRAME_Xmm2], xmm2
//    movdqa [rbp + KTRAP_FRAME_Xmm3], xmm3
//    movdqa [rbp + KTRAP_FRAME_Xmm4], xmm4
//    movdqa [rbp + KTRAP_FRAME_Xmm5], xmm5
.endif

    /* Save segment selectors */
    mov ax, ds
    mov [rbp + KTRAP_FRAME_SegDs], ax
    mov ax, es
    mov [rbp + KTRAP_FRAME_SegEs], ax
    mov ax, fs
    mov [rbp + KTRAP_FRAME_SegFs], ax
    mov ax, gs
    mov [rbp + KTRAP_FRAME_SegGs], ax

    /* Save previous mode and swap gs when it was UserMode */
    mov ax, [rbp + KTRAP_FRAME_SegCs]
    and ax, 1
    mov [rbp + KTRAP_FRAME_PreviousMode], al
    jz 1f
    swapgs
1:

    /* Save previous irql */
    mov rax, cr8
    mov [rbp + KTRAP_FRAME_PreviousIrql], al

// KTRAP_FRAME_FaultIndicator
// KTRAP_FRAME_ExceptionActive
// KTRAP_FRAME_MxCsr


    /* Save debug registers */
    mov rax, dr0
    mov [rbp + KTRAP_FRAME_Dr0], rax
    mov rax, dr1
    mov [rbp + KTRAP_FRAME_Dr1], rax
    mov rax, dr2
    mov [rbp + KTRAP_FRAME_Dr2], rax
    mov rax, dr3
    mov [rbp + KTRAP_FRAME_Dr3], rax
    mov rax, dr6
    mov [rbp + KTRAP_FRAME_Dr6], rax
    mov rax, dr7
    mov [rbp + KTRAP_FRAME_Dr7], rax

// KTRAP_FRAME_DebugControl
// KTRAP_FRAME_LastBranchToRip
// KTRAP_FRAME_LastBranchFromRip
// KTRAP_FRAME_LastExceptionToRip
// KTRAP_FRAME_LastExceptionFromRip
// KTRAP_FRAME_TrapFrame

    /* Make sure the direction flag is cleared */
    cld
.endm

.macro LEAVE_TRAP_FRAME

    /* Restore segment selectors */
    mov ax, [rbp + KTRAP_FRAME_SegDs]
    mov ds, ax
    mov ax, [rbp + KTRAP_FRAME_SegEs]
    mov es, ax
    mov ax, [rbp + KTRAP_FRAME_SegFs]
    mov fs, ax
    test byte ptr [rbp + KTRAP_FRAME_PreviousMode], 1
    jz 1f
    swapgs
1:

.if (TRAPFLAGS & TRAPFLAG_SYSTEMSERVICE)
    /* Restore non-volatile registers */
    mov rbx, [rbp + KTRAP_FRAME_Rbx]
    mov rdi, [rbp + KTRAP_FRAME_Rdi]
    mov rsi, [rbp + KTRAP_FRAME_Rsi]
.else
    /* Restore volatile registers */
    mov rax, [rbp + KTRAP_FRAME_Rax]
    mov rcx, [rbp + KTRAP_FRAME_Rcx]
    mov rdx, [rbp + KTRAP_FRAME_Rdx]
    mov r8, [rbp + KTRAP_FRAME_R8]
    mov r9, [rbp + KTRAP_FRAME_R9]
    mov r10, [rbp + KTRAP_FRAME_R10]
    mov r11, [rbp + KTRAP_FRAME_R11]

    /* Restore xmm registers */
//    movdqa xmm0, [rbp + KTRAP_FRAME_Xmm0]
//    movdqa xmm1, [rbp + KTRAP_FRAME_Xmm1]
//    movdqa xmm2, [rbp + KTRAP_FRAME_Xmm2]
//    movdqa xmm3, [rbp + KTRAP_FRAME_Xmm3]
//    movdqa xmm4, [rbp + KTRAP_FRAME_Xmm4]
//    movdqa xmm5, [rbp + KTRAP_FRAME_Xmm5]
.endif

    /* Restore rbp */
    mov rbp, [rbp + KTRAP_FRAME_Rbp]

    /* Adjust stack pointer (plus one qword for rbp, one for error code) */
    add rsp, SIZE_TRAP_FRAME_ALLOC + 0x10
.endm

.macro DISPATCH_EXCEPTION Code, NumParams, p1, p2, p3

    /* rsp+0x28 points to EXCEPTION_RECORD, set it up */
    mov dword ptr [rsp + 0x28 + EXCEPTION_RECORD_ExceptionCode], \Code
    mov dword ptr [rsp + 0x28 + EXCEPTION_RECORD_ExceptionFlags], 0
    mov qword ptr [rsp + 0x28 + EXCEPTION_RECORD_ExceptionRecord], 0
    mov rax, [rbp + KTRAP_FRAME_Rip]
    mov [rsp + 0x28 + EXCEPTION_RECORD_ExceptionAddress], rax
    mov dword ptr [rsp + 0x28 + EXCEPTION_RECORD_NumberParameters], \NumParams

.if \NumParams >= 1
    mov qword ptr [rsp + 0x28 + EXCEPTION_RECORD_ExceptionInformation + 0x00], \p1
.endif
.if \NumParams >= 2
    mov qword ptr [rsp + 0x28 + EXCEPTION_RECORD_ExceptionInformation + 0x08], \p2
.endif
.if \NumParams >= 3
    mov qword ptr [rsp + 0x28 + EXCEPTION_RECORD_ExceptionInformation + 0x10], \p3
.endif

    mov byte ptr [rsp + 0x20], 1  // FirstChance
    mov r9b, [rbp + KTRAP_FRAME_PreviousMode] // PreviousMode
    mov r8, rbp                   // TrapFrame
    xor rdx, rdx                  // ExceptionFrame
    lea rcx, [rsp + 0x28]         // ExceptionRecord
    call _KiDispatchException
.endm


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

    ENTER_TRAP_FRAME (SIZE_EXCEPTION_RECORD + 0x28), 0

    lea rcx, _MsgBreakpointTrap[rip]
    mov rdx, [rbp + KTRAP_FRAME_Rip]
    lea rax, _FrLdrDbgPrint[rip]
    call [rax]

    DISPATCH_EXCEPTION STATUS_BREAKPOINT, 3, 0, 0, 0

    /* Return */
    LEAVE_TRAP_FRAME
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

    ENTER_TRAP_FRAME (SIZE_EXCEPTION_RECORD + 0x28), 0

    /* Save page fault address */
    mov rax, cr2
    mov [rbp  + KTRAP_FRAME_FaultAddress], rax

    DISPATCH_EXCEPTION STATUS_BREAKPOINT, 0, 0, 0, 0

    LEAVE_TRAP_FRAME;
    iretq
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

    ENTER_TRAP_FRAME (SIZE_EXCEPTION_RECORD + 0x28), 0

    /* Increase Rip to skip the int3 */
    inc qword ptr [rbp + KTRAP_FRAME_Rip]

    /* Call KiDispatchException */
    mov r8, [ebp+KTRAP_FRAME_Rax] // Service
    mov rcx, [ebp+KTRAP_FRAME_Rcx] // Buffer
    mov rdx, [ebp+KTRAP_FRAME_Rdx] // Length
    DISPATCH_EXCEPTION STATUS_BREAKPOINT, 3, r8, rcx, rdx

    LEAVE_TRAP_FRAME;
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



