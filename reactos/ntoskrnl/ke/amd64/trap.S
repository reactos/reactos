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
.ascii "Page fault! Code = 0x%x, RIP = %p, FaultingAddress = %p\n\0"

_MsgGeneralProtFault:
.ascii "General protection fault at %p!\n\0"

_MsgBreakpointTrap:
.ascii "BreakpointTrap at %p\n\0"

_MsgUnexpectedInterrupt:
.ascii "UnexpectedInterrupt\n\0"

_MsgInvalidOpcodeFault:
.ascii "General protection fault at %p!\n\0"

_MsgTrapInfo:
.ascii "Trap: %s at %p\n\0"

.macro TRAPINFO func
#if 0
    jmp 2f
    .equ expr, 12
1:  .asciz "\func"
2:
    sub rsp, 0x20
    lea rcx, _MsgTrapInfo[rip]
    lea rdx, 1b[rip]
    mov r8, [rbp + KTRAP_FRAME_Rip]
    call _FrLdrDbgPrint[rip]
    add rsp, 0x20
#endif
.endm

/* Helper Macros *************************************************************/

#define TRAPFLAG_VOLATILES 0x01
#define TRAPFLAG_NONVOLATILES 0x02
#define TRAPFLAG_XMM 0x04
#define TRAPFLAG_SEGMENTS 0x08
#define TRAPFLAG_DEBUG 0x10

#define TRAPFLAG_SYSTEMSERVICE (TRAPFLAG_VOLATILES|TRAPFLAG_DEBUG)
#define TRAPFLAG_ALL 0xff

/* 
 * Stack Layout:
 * |-------------------|
 * | KTRAP_FRAME       |
 * |-------------------| <- rbp
 * | EXCEPTION_RECORD  |
 * |-------------------|
 * | KEXCEPTION_FRAME  |
 * |-------------------| <- rsp
 *
 */

/*
 * ENTER_TRAP_FRAME - Allocate SIZE_KTRAP_FRAME and save registers to it
 */
.macro ENTER_TRAP_FRAME AllocSize Flags
.set SIZE_INITIAL_FRAME, 7 * 8
//.set SIZE_LOCAL_DATA, SIZE_EXCEPTION_RECORD + 0x28
.set SIZE_TRAP_FRAME_ALLOC, SIZE_KTRAP_FRAME - SIZE_INITIAL_FRAME
.set TRAPFLAGS, \Flags

    /* Save rbp */
    push rbp
    .pushreg rbp

    /* Make room for a KTRAP_FRAME and function parameters */
    sub rsp, SIZE_TRAP_FRAME_ALLOC
    .allocstack SIZE_TRAP_FRAME_ALLOC

    /* Point rbp to the KTRAP_FRAME */
    lea rbp, [rsp]

.if (TRAPFLAGS & TRAPFLAG_NONVOLATILES)
    /* Save non-volatile registers */
    mov [rbp + KTRAP_FRAME_Rbx], rbx
    mov [rbp + KTRAP_FRAME_Rdi], rdi
    mov [rbp + KTRAP_FRAME_Rsi], rsi
.endif

.if (TRAPFLAGS & TRAPFLAG_VOLATILES)
    /* Save volatile registers */
    mov [rbp + KTRAP_FRAME_Rax], rax
    mov [rbp + KTRAP_FRAME_Rcx], rcx
    mov [rbp + KTRAP_FRAME_Rdx], rdx
    mov [rbp + KTRAP_FRAME_R8], r8
    mov [rbp + KTRAP_FRAME_R9], r9
    mov [rbp + KTRAP_FRAME_R10], r10
    mov [rbp + KTRAP_FRAME_R11], r11
.endif

.if (TRAPFLAGS & TRAPFLAG_XMM)
    /* Save xmm registers */
//    movdqa [rbp + KTRAP_FRAME_Xmm0], xmm0
//    movdqa [rbp + KTRAP_FRAME_Xmm1], xmm1
//    movdqa [rbp + KTRAP_FRAME_Xmm2], xmm2
//    movdqa [rbp + KTRAP_FRAME_Xmm3], xmm3
//    movdqa [rbp + KTRAP_FRAME_Xmm4], xmm4
//    movdqa [rbp + KTRAP_FRAME_Xmm5], xmm5
.endif

.if (TRAPFLAGS & TRAPFLAG_SEGMENTS)
    /* Save segment selectors */
    mov ax, ds
    mov [rbp + KTRAP_FRAME_SegDs], ax
    mov ax, es
    mov [rbp + KTRAP_FRAME_SegEs], ax
    mov ax, fs
    mov [rbp + KTRAP_FRAME_SegFs], ax
    mov ax, gs
    mov [rbp + KTRAP_FRAME_SegGs], ax
.endif

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

.if (TRAPFLAGS & TRAPFLAG_DEBUG)
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
.endif

// KTRAP_FRAME_DebugControl
// KTRAP_FRAME_LastBranchToRip
// KTRAP_FRAME_LastBranchFromRip
// KTRAP_FRAME_LastExceptionToRip
// KTRAP_FRAME_LastExceptionFromRip
// KTRAP_FRAME_TrapFrame

    /* Make sure the direction flag is cleared */
    cld
.endm


/*
 *  LEAVE_TRAP_FRAME - Restore registers and free stack space
 */
.macro LEAVE_TRAP_FRAME

.if (TRAPFLAGS & TRAPFLAG_SEGMENTS)
    /* Restore segment selectors */
    mov ax, [rbp + KTRAP_FRAME_SegDs]
    mov ds, ax
    mov ax, [rbp + KTRAP_FRAME_SegEs]
    mov es, ax
    mov ax, [rbp + KTRAP_FRAME_SegFs]
    mov fs, ax
.endif

    test byte ptr [rbp + KTRAP_FRAME_PreviousMode], 1
    jz 1f
    swapgs
1:

.if (TRAPFLAGS & TRAPFLAG_NONVOLATILES)
    /* Restore non-volatile registers */
    mov rbx, [rbp + KTRAP_FRAME_Rbx]
    mov rdi, [rbp + KTRAP_FRAME_Rdi]
    mov rsi, [rbp + KTRAP_FRAME_Rsi]
.endif

.if (TRAPFLAGS & TRAPFLAG_VOLATILES)
    /* Restore volatile registers */
    mov rax, [rbp + KTRAP_FRAME_Rax]
    mov rcx, [rbp + KTRAP_FRAME_Rcx]
    mov rdx, [rbp + KTRAP_FRAME_Rdx]
    mov r8, [rbp + KTRAP_FRAME_R8]
    mov r9, [rbp + KTRAP_FRAME_R9]
    mov r10, [rbp + KTRAP_FRAME_R10]
    mov r11, [rbp + KTRAP_FRAME_R11]
.endif

.if (TRAPFLAGS & TRAPFLAG_XMM)
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



// rbp = TrapFrame, ecx = ExceptionCode, edx = NumParams, r9,r10,r11 = params
_InternalDispatchException:

    /* Allocate stack space for EXCEPTION_RECORD and KEXCEPTION_FRAME */
    sub rsp, SIZE_EXCEPTION_RECORD + SIZE_KEXCEPTION_FRAME

    /* Set up EXCEPTION_RECORD */
    mov eax, ecx
    lea rcx, [rsp + SIZE_KEXCEPTION_FRAME]
    mov [rcx + EXCEPTION_RECORD_ExceptionCode], eax
    xor rax, rax
    mov [rcx + EXCEPTION_RECORD_ExceptionFlags], eax
    mov [rcx + EXCEPTION_RECORD_ExceptionRecord], rax
    mov rax, [rbp + KTRAP_FRAME_Rip]
    mov [rcx + EXCEPTION_RECORD_ExceptionAddress], rax
    mov [rcx + EXCEPTION_RECORD_NumberParameters], edx
    mov [rcx + EXCEPTION_RECORD_ExceptionInformation + 0x00], r9
    mov [rcx + EXCEPTION_RECORD_ExceptionInformation + 0x08], r10
    mov [rcx + EXCEPTION_RECORD_ExceptionInformation + 0x10], r11

    /* Set up KEXCEPTION_FRAME */
    mov rax, [rbp + KTRAP_FRAME_Rbp]
    mov [rsp + KEXCEPTION_FRAME_Rbp], rax
    mov [rsp + KEXCEPTION_FRAME_Rbx], rbx
    mov [rsp + KEXCEPTION_FRAME_Rdi], rdi
    mov [rsp + KEXCEPTION_FRAME_Rsi], rsi
    mov [rsp + KEXCEPTION_FRAME_R12], r12
    mov [rsp + KEXCEPTION_FRAME_R13], r13
    mov [rsp + KEXCEPTION_FRAME_R14], r14
    mov [rsp + KEXCEPTION_FRAME_R15], r15
    mov qword ptr [rsp + KEXCEPTION_FRAME_Return], 0

    /* Call KiDispatchException */
    // rcx already points to ExceptionRecord
    mov rdx, rsp                                // ExceptionFrame
    mov r8, rbp                                 // TrapFrame
    mov r9b, [r8 + KTRAP_FRAME_PreviousMode]    // PreviousMode
    mov byte ptr [rsp + KEXCEPTION_FRAME_P5], 1 // FirstChance
    call _KiDispatchException

    add rsp, SIZE_EXCEPTION_RECORD + SIZE_KEXCEPTION_FRAME
    ret

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

    ENTER_TRAP_FRAME (0x28), TRAPFLAG_ALL

    TRAPINFO KiDebugTrapOrFault

    /* Check if the frame was from kernelmode */
    test word ptr [rbp + KTRAP_FRAME_SegCs], 3
    jz KiDebugTrapOrFaultKMode

    /* Enable interrupts for user-mode */
    sti

KiDebugTrapOrFaultKMode:

    /* Dispatch the exception */
    mov ecx, STATUS_SINGLE_STEP
    mov edx, 0
    mov r9, 0
    mov r10, 0
    mov r11, 0
    call _InternalDispatchException

    /* Return */
    LEAVE_TRAP_FRAME
    iretq
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

    ENTER_TRAP_FRAME (0x28), TRAPFLAG_ALL

    TRAPINFO KiBreakpointTrap

//    lea rcx, _MsgBreakpointTrap[rip]
//    mov rdx, rsp
//    call _FrLdrDbgPrint[rip]

    /* Dispatch the exception */
    mov ecx, STATUS_BREAKPOINT
    mov edx, 3
    mov r9, 0
    mov r10, 0
    mov r11, 0
    call _InternalDispatchException

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

    ENTER_TRAP_FRAME (0x28), TRAPFLAG_ALL

    TRAPINFO KiInvalidOpcodeFault

//    DISPATCH_EXCEPTION STATUS_BREAKPOINT, 3, 0, 0, 0

    mov rdx, [rbp + KTRAP_FRAME_Rip]
    lea rcx, _MsgInvalidOpcodeFault[rip]
    call _FrLdrDbgPrint[rip]
    jmp $

    /* Return */
    LEAVE_TRAP_FRAME
    iretq
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

    /* Reserve stack space for parameters */
    sub rsp, 0x28
    .allocstack 0x28

    /* Bugcheck code UNEXPECTED_KERNEL_MODE_TRAP */
    mov rcx, 0x0000007F
    
    /* Set double fault parameters */
    mov rdx, 0x00000008
    mov r8, 0
    mov r9, 0
    mov qword ptr [rsp + 0x20], 0

    call _KeBugCheckEx

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

    ENTER_TRAP_FRAME (0x28), TRAPFLAG_ALL

    TRAPINFO KiGeneralProtectionFault

//    DISPATCH_EXCEPTION STATUS_BREAKPOINT, 3, 0, 0, 0

    mov rdx, [rbp + KTRAP_FRAME_Rip]
    lea rcx, _MsgGeneralProtFault[rip]
    call _FrLdrDbgPrint[rip]
    jmp $

    /* Return */
    LEAVE_TRAP_FRAME
    iretq
.endproc


.proc KiPageFault
    .pushframe 1
    /* We have an error code */

    ENTER_TRAP_FRAME (0x28), TRAPFLAG_ALL

    TRAPINFO KiPageFault

#if 0
    lea rcx, _MsgPageFault[rip]
    mov rdx, [rbp + KTRAP_FRAME_ErrorCode]
    mov r8, [rbp + KTRAP_FRAME_Rip]
    mov r9, [rbp + KTRAP_FRAME_FaultAddress]
    call _FrLdrDbgPrint[rip]
#endif

    /* Save page fault address */
    mov rdx, cr2
    mov [rbp  + KTRAP_FRAME_FaultAddress], rdx

    /* Call page fault handler */
    mov ecx, [rbp + KTRAP_FRAME_ErrorCode] // StoreInstruction
    and ecx, 1
    // rdx == Address
    mov r8b, [rbp + KTRAP_FRAME_SegCs] // Mode
    and r8b, 1
    mov r9, rbp // TrapInformation
    call _MmAccessFault

    /* Check for success */
    test eax, eax
    jge PageFaultReturn

    /* Set parameter 1 to error code */
    mov r9d, [rbp + KTRAP_FRAME_ErrorCode]

    /* Set parameter2 to faulting address */
    mov r10, cr2  // Param2 = faulting address

    cmp eax, STATUS_ACCESS_VIOLATION
    je AccessViolation
    cmp eax, STATUS_GUARD_PAGE_VIOLATION
    je SpecialCode
    cmp eax, STATUS_STACK_OVERFLOW
    je SpecialCode

InPageException:
    /* Dispatch in-page exception */
    mov ecx, STATUS_IN_PAGE_ERROR // ExceptionCode
    mov r11d, eax // Param3 = Status
    mov edx, 3    // ParamCount
    call _InternalDispatchException
    jmp PageFaultReturn

AccessViolation:
    /* Use more proper status code */
    mov eax, KI_EXCEPTION_ACCESS_VIOLATION

SpecialCode:
    /* Setup a normal page fault exception */
    mov ecx, eax // ExceptionCode
    mov edx, 2   // ParamCount
    call _InternalDispatchException

PageFaultReturn:
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

    ENTER_TRAP_FRAME (0x28), TRAPFLAG_ALL

    TRAPINFO KiDebugServiceTrap

    /* Increase Rip to skip the int3 */
    inc qword ptr [rbp + KTRAP_FRAME_Rip]

    /* Dispatch the exception */
    mov ecx, STATUS_BREAKPOINT
    mov edx, 3
    mov r9, [rbp+KTRAP_FRAME_Rax] // Service
    mov r10, [rbp+KTRAP_FRAME_Rcx] // Buffer
    mov r11, [rbp+KTRAP_FRAME_Rdx] // Length
    call _InternalDispatchException

    LEAVE_TRAP_FRAME;
    iretq
.endproc


.proc KiDpcInterrupt
    .pushframe 1

    UNIMPLEMENTED KiDpcInterrupt

    jmp $
.endproc


.proc KiIpiInterrupt
    .pushframe 1

    UNIMPLEMENTED KiIpiInterrupt

    jmp $
.endproc


.proc KiUnexpectedInterrupt
    .pushframe 0
    push 0
    .allocstack 0x8

    lea rcx, _MsgUnexpectedInterrupt[rip]
    call _FrLdrDbgPrint[rip]

    jmp $
.endproc



