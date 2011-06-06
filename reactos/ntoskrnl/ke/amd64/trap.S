/*
 * FILE:            ntoskrnl/ke/amd64/trap.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Traps, Entrypoints and Exitpoints
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

#include <ksamd64.inc>

EXTERN KiDispatchException:PROC
EXTERN FrLdrDbgPrint:DWORD
EXTERN KeBugCheckWithTf:PROC
EXTERN MmAccessFault:PROC
EXTERN KiSystemFatalException:PROC
EXTERN KiNpxNotAvailableFaultHandler:PROC
EXTERN KiGeneralProtectionFaultHandler:PROC
EXTERN KiXmmExceptionHandler:PROC

/* GLOBALS *******************************************************************/

.data

PUBLIC MsgUnimplemented
MsgUnimplemented:
.asciz "WARNING:  %s at %s:%d is UNIMPLEMENTED!\n"

MsgPageFault:
.asciz "Page fault! Code = 0x%x, RIP = %p, FaultingAddress = %p\n"

MsgGeneralProtFault:
.asciz "General protection fault at %p!\n"

MsgBreakpointTrap:
.asciz "BreakpointTrap at %p\n"

MsgUnexpectedInterrupt:
.asciz "UnexpectedInterrupt Vector=0x%02lx\n"

MsgInvalidOpcodeFault:
.asciz "Invalid opcode fault at %p!\n"

MsgDoubleFault:
.asciz "Double fault at %p, rbp=%p!\n"

MsgTrapInfo:
.asciz "Trap: %s at %p\n"

MACRO(TRAPINFO, func)
LOCAL label1, label2
#if 0
    jmp label2
label1:  .asciz "\func"
label2:
    sub rsp, 32
    lea rcx, MsgTrapInfo[rip]
    lea rdx, 1b[rip]
    mov r8, [rbp + KTRAP_FRAME_Rip]
    call qword ptr FrLdrDbgPrint[rip]
    add rsp, 32
#endif
ENDM

/* Helper Macros *************************************************************/

#define TRAPFLAG_VOLATILES HEX(01)
#define TRAPFLAG_NONVOLATILES HEX(02)
#define TRAPFLAG_XMM HEX(04)
#define TRAPFLAG_SEGMENTS HEX(08)
#define TRAPFLAG_DEBUG HEX(10)
#define TRAPFLAG_HAS_ERRORCODE HEX(100)

#define TRAPFLAG_SYSTEMSERVICE (TRAPFLAG_VOLATILES|TRAPFLAG_DEBUG)
#define TRAPFLAG_ALL HEX(ff)

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
 * ENTER_TRAP_FRAME - Allocate KTRAP_FRAME_LENGTH and save registers to it
 */
MACRO(ENTER_TRAP_FRAME, Flags)
LOCAL dont_swap

    /* Save the trap flags for this trap */
    TRAPFLAGS = VAL(Flags)

    /* Size of hardware trap frame */
if (TRAPFLAGS AND TRAPFLAG_HAS_ERRORCODE)
    .pushframe code
    SIZE_INITIAL_FRAME = 6 * 8
else
    .pushframe
    SIZE_INITIAL_FRAME = 5 * 8
endif

    /* Make room for a KTRAP_FRAME */
    sub rsp, (KTRAP_FRAME_LENGTH - SIZE_INITIAL_FRAME)
    .allocstack (KTRAP_FRAME_LENGTH - SIZE_INITIAL_FRAME)
    .endprolog

    /* Save rbp */
    mov [rsp + KTRAP_FRAME_Rbp], rbp

    /* Point rbp to the KTRAP_FRAME */
    lea rbp, [rsp]

if (TRAPFLAGS AND TRAPFLAG_NONVOLATILES)
    /* Save non-volatile registers */
    mov [rbp + KTRAP_FRAME_Rbx], rbx
    mov [rbp + KTRAP_FRAME_Rdi], rdi
    mov [rbp + KTRAP_FRAME_Rsi], rsi
endif

if (TRAPFLAGS AND TRAPFLAG_VOLATILES)
    /* Save volatile registers */
    mov [rbp + KTRAP_FRAME_Rax], rax
    mov [rbp + KTRAP_FRAME_Rcx], rcx
    mov [rbp + KTRAP_FRAME_Rdx], rdx
    mov [rbp + KTRAP_FRAME_R8], r8
    mov [rbp + KTRAP_FRAME_R9], r9
    mov [rbp + KTRAP_FRAME_R10], r10
    mov [rbp + KTRAP_FRAME_R11], r11
endif

if (TRAPFLAGS AND TRAPFLAG_XMM)
    /* Save xmm registers */
    movdqa [rbp + KTRAP_FRAME_Xmm0], xmm0
    movdqa [rbp + KTRAP_FRAME_Xmm1], xmm1
    movdqa [rbp + KTRAP_FRAME_Xmm2], xmm2
    movdqa [rbp + KTRAP_FRAME_Xmm3], xmm3
    movdqa [rbp + KTRAP_FRAME_Xmm4], xmm4
    movdqa [rbp + KTRAP_FRAME_Xmm5], xmm5
endif

if (TRAPFLAGS AND TRAPFLAG_SEGMENTS)
    /* Save segment selectors */
    mov ax, ds
    mov [rbp + KTRAP_FRAME_SegDs], ax
    mov ax, es
    mov [rbp + KTRAP_FRAME_SegEs], ax
    mov ax, fs
    mov [rbp + KTRAP_FRAME_SegFs], ax
    mov ax, gs
    mov [rbp + KTRAP_FRAME_SegGs], ax
endif

    /* Save previous mode and swap gs when it was UserMode */
    mov ax, [rbp + KTRAP_FRAME_SegCs]
    and al, 1
    mov [rbp + KTRAP_FRAME_PreviousMode], al
    jz dont_swap
    swapgs
dont_swap:

    /* Save previous irql */
    mov rax, cr8
    mov [rbp + KTRAP_FRAME_PreviousIrql], al

// KTRAP_FRAME_FaultIndicator
// KTRAP_FRAME_ExceptionActive
// KTRAP_FRAME_MxCsr

if (TRAPFLAGS AND TRAPFLAG_DEBUG)
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
endif

// KTRAP_FRAME_DebugControl
// KTRAP_FRAME_LastBranchToRip
// KTRAP_FRAME_LastBranchFromRip
// KTRAP_FRAME_LastExceptionToRip
// KTRAP_FRAME_LastExceptionFromRip
// KTRAP_FRAME_TrapFrame

    /* Make sure the direction flag is cleared */
    cld
ENDM


/*
 *  LEAVE_TRAP_FRAME - Restore registers and free stack space
 */
MACRO(LEAVE_TRAP_FRAME)
LOCAL dont_swap_back
if (TRAPFLAGS AND TRAPFLAG_SEGMENTS)
    /* Restore segment selectors */
    mov ax, [rbp + KTRAP_FRAME_SegDs]
    mov ds, ax
    mov ax, [rbp + KTRAP_FRAME_SegEs]
    mov es, ax
    mov ax, [rbp + KTRAP_FRAME_SegFs]
    mov fs, ax
endif

    test byte ptr [rbp + KTRAP_FRAME_PreviousMode], 1
    jz dont_swap_back
    swapgs
dont_swap_back:

if (TRAPFLAGS AND TRAPFLAG_NONVOLATILES)
    /* Restore non-volatile registers */
    mov rbx, [rbp + KTRAP_FRAME_Rbx]
    mov rdi, [rbp + KTRAP_FRAME_Rdi]
    mov rsi, [rbp + KTRAP_FRAME_Rsi]
endif

if (TRAPFLAGS AND TRAPFLAG_VOLATILES)
    /* Restore volatile registers */
    mov rax, [rbp + KTRAP_FRAME_Rax]
    mov rcx, [rbp + KTRAP_FRAME_Rcx]
    mov rdx, [rbp + KTRAP_FRAME_Rdx]
    mov r8, [rbp + KTRAP_FRAME_R8]
    mov r9, [rbp + KTRAP_FRAME_R9]
    mov r10, [rbp + KTRAP_FRAME_R10]
    mov r11, [rbp + KTRAP_FRAME_R11]
endif

if (TRAPFLAGS AND TRAPFLAG_XMM)
    /* Restore xmm registers */
    movdqa xmm0, [rbp + KTRAP_FRAME_Xmm0]
    movdqa xmm1, [rbp + KTRAP_FRAME_Xmm1]
    movdqa xmm2, [rbp + KTRAP_FRAME_Xmm2]
    movdqa xmm3, [rbp + KTRAP_FRAME_Xmm3]
    movdqa xmm4, [rbp + KTRAP_FRAME_Xmm4]
    movdqa xmm5, [rbp + KTRAP_FRAME_Xmm5]
endif

    /* Restore rbp */
    mov rbp, [rbp + KTRAP_FRAME_Rbp]

    /* Adjust stack pointer */
    add rsp, KTRAP_FRAME_Rip
ENDM


/* FUNCTIONS *****************************************************************/

.code64

ALIGN 8

PUBLIC InterruptDispatchTable
InterruptDispatchTable:
Vector = 0
REPEAT 256
    push Vector
    jmp KiUnexpectedInterrupt
    ALIGN 8
    Vector = Vector+1
ENDR

// rbp = TrapFrame, eax = ExceptionCode, edx = NumParams, r9,r10,r11 = params
.PROC InternalDispatchException

    /* Allocate stack space for EXCEPTION_RECORD and KEXCEPTION_FRAME */
    sub rsp, EXCEPTION_RECORD_LENGTH + KEXCEPTION_FRAME_LENGTH
    .allocstack (EXCEPTION_RECORD_LENGTH + KEXCEPTION_FRAME_LENGTH)
    .endprolog

    /* Set up EXCEPTION_RECORD */
    lea rcx, [rsp + KEXCEPTION_FRAME_LENGTH]
    mov [rcx + EXCEPTION_RECORD_ExceptionCode], eax
    xor rax, rax
    mov [rcx + EXCEPTION_RECORD_ExceptionFlags], eax
    mov [rcx + EXCEPTION_RECORD_ExceptionRecord], rax
    mov rax, [rbp + KTRAP_FRAME_Rip]
    mov [rcx + EXCEPTION_RECORD_ExceptionAddress], rax
    mov [rcx + EXCEPTION_RECORD_NumberParameters], edx
    mov [rcx + EXCEPTION_RECORD_ExceptionInformation + HEX(00)], r9
    mov [rcx + EXCEPTION_RECORD_ExceptionInformation + HEX(08)], r10
    mov [rcx + EXCEPTION_RECORD_ExceptionInformation + HEX(10)], r11

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
    movdqa [rsp + KEXCEPTION_FRAME_Xmm6], xmm6
    movdqa [rsp + KEXCEPTION_FRAME_Xmm7], xmm7
    movdqa [rsp + KEXCEPTION_FRAME_Xmm8], xmm8
    movdqa [rsp + KEXCEPTION_FRAME_Xmm9], xmm9
    movdqa [rsp + KEXCEPTION_FRAME_Xmm10], xmm10
    movdqa [rsp + KEXCEPTION_FRAME_Xmm11], xmm11
    movdqa [rsp + KEXCEPTION_FRAME_Xmm12], xmm12
    movdqa [rsp + KEXCEPTION_FRAME_Xmm13], xmm13
    movdqa [rsp + KEXCEPTION_FRAME_Xmm14], xmm14
    movdqa [rsp + KEXCEPTION_FRAME_Xmm15], xmm15
    mov qword ptr [rsp + KEXCEPTION_FRAME_Return], 0

    /* Call KiDispatchException */
    // rcx already points to ExceptionRecord
    mov rdx, rsp                                // ExceptionFrame
    mov r8, rbp                                 // TrapFrame
    mov r9b, [r8 + KTRAP_FRAME_PreviousMode]    // PreviousMode
    mov byte ptr [rsp + KEXCEPTION_FRAME_P5], 1 // FirstChance
    call KiDispatchException

    /* Restore registers */
    mov r12, [rsp + KEXCEPTION_FRAME_R12]
    mov r13, [rsp + KEXCEPTION_FRAME_R13]
    mov r14, [rsp + KEXCEPTION_FRAME_R14]
    mov r15, [rsp + KEXCEPTION_FRAME_R15]
    movdqa xmm6, [rsp + KEXCEPTION_FRAME_Xmm6]
    movdqa xmm7, [rsp + KEXCEPTION_FRAME_Xmm7]
    movdqa xmm8, [rsp + KEXCEPTION_FRAME_Xmm8]
    movdqa xmm9, [rsp + KEXCEPTION_FRAME_Xmm9]
    movdqa xmm10, [rsp + KEXCEPTION_FRAME_Xmm10]
    movdqa xmm11, [rsp + KEXCEPTION_FRAME_Xmm11]
    movdqa xmm12, [rsp + KEXCEPTION_FRAME_Xmm12]
    movdqa xmm13, [rsp + KEXCEPTION_FRAME_Xmm13]
    movdqa xmm14, [rsp + KEXCEPTION_FRAME_Xmm14]
    movdqa xmm15, [rsp + KEXCEPTION_FRAME_Xmm15]

    add rsp, EXCEPTION_RECORD_LENGTH + KEXCEPTION_FRAME_LENGTH
    ret
.ENDP InternalDispatchException


/* SOFTWARE INTERRUPT SERVICES ***********************************************/

PUBLIC KiDivideErrorFault
.PROC KiDivideErrorFault
    /* Push pseudo error code */
    ENTER_TRAP_FRAME TRAPFLAG_ALL

    /* Enable interrupts */
    sti

    /* Dispatch the exception */
    mov eax, STATUS_INTEGER_DIVIDE_BY_ZERO
    mov edx, 0
    mov r9, 0
    mov r10, 0
    mov r11, 0
    call InternalDispatchException

    /* Return */
    LEAVE_TRAP_FRAME
    iretq
.ENDP KiDivideErrorFault


PUBLIC KiDebugTrapOrFault
.PROC KiDebugTrapOrFault
    /* Push pseudo error code */
    ENTER_TRAP_FRAME TRAPFLAG_ALL

    TRAPINFO KiDebugTrapOrFault

    /* Check if the frame was from kernelmode */
    test word ptr [rbp + KTRAP_FRAME_SegCs], 3
    jz KiDebugTrapOrFaultKMode

    /* Enable interrupts for user-mode */
    sti

KiDebugTrapOrFaultKMode:

    /* Dispatch the exception */
    mov eax, STATUS_SINGLE_STEP
    mov edx, 0
    mov r9, 0
    mov r10, 0
    mov r11, 0
    call InternalDispatchException

    /* Return */
    LEAVE_TRAP_FRAME
    iretq
.ENDP KiDebugTrapOrFault


PUBLIC KiNmiInterrupt
.PROC KiNmiInterrupt
    /* Push pseudo error code */
    ENTER_TRAP_FRAME TRAPFLAG_ALL

    UNIMPLEMENTED KiNmiInterrupt

    jmp $

    /* Return */
    LEAVE_TRAP_FRAME
    iretq
.ENDP KiNmiInterrupt


PUBLIC KiBreakpointTrap
.PROC KiBreakpointTrap
    /* Push pseudo error code */
    ENTER_TRAP_FRAME TRAPFLAG_ALL

    TRAPINFO KiBreakpointTrap

//    lea rcx, MsgBreakpointTrap[rip]
//    mov rdx, rsp
//    call qword ptr FrLdrDbgPrint[rip]

    /* Dispatch the exception */
    mov eax, STATUS_BREAKPOINT
    mov edx, 3
    mov r9, 0
    mov r10, 0
    mov r11, 0
    call InternalDispatchException

    /* Return */
    LEAVE_TRAP_FRAME
    iretq
.ENDP KiBreakpointTrap


PUBLIC KiOverflowTrap
.PROC KiOverflowTrap
    /* Push pseudo error code */
    ENTER_TRAP_FRAME TRAPFLAG_ALL

    /* Enable interrupts */
    sti

    /* Dispatch the exception */
    mov eax, STATUS_INTEGER_OVERFLOW
    mov edx, 3
    mov r9, 0
    mov r10, 0
    mov r11, 0
    call InternalDispatchException

    /* Return */
    LEAVE_TRAP_FRAME
    iretq
.ENDP KiOverflowTrap


PUBLIC KiBoundFault
.PROC KiBoundFault
    /* Push pseudo error code */
    ENTER_TRAP_FRAME TRAPFLAG_ALL

    /* Check if the frame was from kernelmode */
    test word ptr [rbp + KTRAP_FRAME_SegCs], 3
    jnz KiBoundFaltUserMode

    /* Bugcheck */
    mov ecx, EXCEPTION_BOUND_CHECK
    mov rdx, rbp
    call KiSystemFatalException

KiBoundFaltUserMode:
    /* Enable interrupts for user-mode */
    sti

    /* Dispatch the exception */
    mov eax, STATUS_INTEGER_OVERFLOW
    mov edx, 3
    mov r9, 0
    mov r10, 0
    mov r11, 0
    call InternalDispatchException

    /* Return */
    LEAVE_TRAP_FRAME
    iretq
.ENDP KiBoundFault


PUBLIC KiInvalidOpcodeFault
.PROC KiInvalidOpcodeFault
    /* Push pseudo error code */
    ENTER_TRAP_FRAME TRAPFLAG_ALL

    TRAPINFO KiInvalidOpcodeFault

    mov rdx, [rbp + KTRAP_FRAME_Rip]
    lea rcx, MsgInvalidOpcodeFault[rip]
    call qword ptr FrLdrDbgPrint[rip]

    /* Enable interrupts */
    sti

    /* Check if the frame was from kernelmode */
    test word ptr [rbp + KTRAP_FRAME_SegCs], 3
    jz KiInvalidOpcodeKernel

    // FIXME: handle STATUS_INVALID_LOCK_SEQUENCE

KiInvalidOpcodeKernel:
    /* Kernel mode fault */

    /* Dispatch the exception */
    mov eax, STATUS_ILLEGAL_INSTRUCTION
    mov edx, 3
    mov r9, 0
    mov r10, 0
    mov r11, 0
    call InternalDispatchException

    /* Return */
    LEAVE_TRAP_FRAME
    iretq
.ENDP KiInvalidOpcodeFault


PUBLIC KiNpxNotAvailableFault
.PROC KiNpxNotAvailableFault
    /* Push pseudo error code */
    ENTER_TRAP_FRAME TRAPFLAG_ALL

    /* Call the C handler */
    mov rcx, rbp
    call KiNpxNotAvailableFaultHandler

    /* Check the return status code */
    test eax, eax
    jz KiNpxNotAvailableFaultExit

    /* Dispatch the exception */
    mov edx, 3
    mov r9, 0
    mov r10, 0
    mov r11, 0
    call InternalDispatchException

KiNpxNotAvailableFaultExit:
    /* Return */
    LEAVE_TRAP_FRAME
    iretq
.ENDP KiNpxNotAvailableFault


PUBLIC KiDoubleFaultAbort
.PROC KiDoubleFaultAbort
    /* Push pseudo error code */
    ENTER_TRAP_FRAME TRAPFLAG_ALL

    lea rcx, MsgDoubleFault[rip]
    mov rdx, [rbp + KTRAP_FRAME_FaultAddress]
    mov r8, rbp
    call qword ptr FrLdrDbgPrint[rip]

    /* Bugcheck */
    mov ecx, 8 // EXCEPTION_DOUBLE_FAULT
    mov rdx, rbp
    call KiSystemFatalException

    jmp $
.ENDP KiDoubleFaultAbort


PUBLIC KiNpxSegmentOverrunAbort
.PROC KiNpxSegmentOverrunAbort
    /* Push pseudo error code */
    ENTER_TRAP_FRAME TRAPFLAG_ALL

    /* Bugcheck */
    mov ecx, EXCEPTION_NPX_OVERRUN
    mov rdx, rbp
    call KiSystemFatalException

    jmp $
.ENDP KiNpxSegmentOverrunAbort


PUBLIC KiInvalidTssFault
.PROC KiInvalidTssFault
    /* We have an error code */
    ENTER_TRAP_FRAME (TRAPFLAG_HAS_ERRORCODE OR TRAPFLAG_ALL)

    /* Bugcheck */
    mov ecx, EXCEPTION_INVALID_TSS
    mov rdx, rbp
    call KiSystemFatalException

    jmp $
.ENDP KiInvalidTssFault


PUBLIC KiSegmentNotPresentFault
.PROC KiSegmentNotPresentFault
    /* We have an error code */
    ENTER_TRAP_FRAME (TRAPFLAG_HAS_ERRORCODE OR TRAPFLAG_ALL)

    /* Bugcheck */
    mov ecx, EXCEPTION_SEGMENT_NOT_PRESENT
    mov rdx, rbp
    call KiSystemFatalException

    jmp $
.ENDP KiSegmentNotPresentFault


PUBLIC KiStackFault
.PROC KiStackFault
    /* We have an error code */
    ENTER_TRAP_FRAME (TRAPFLAG_HAS_ERRORCODE OR TRAPFLAG_ALL)

    /* Bugcheck */
    mov ecx, EXCEPTION_STACK_FAULT
    mov rdx, rbp
    call KiSystemFatalException

    jmp $
.ENDP KiStackFault


PUBLIC KiGeneralProtectionFault
.PROC KiGeneralProtectionFault
    /* We have an error code */
    ENTER_TRAP_FRAME (TRAPFLAG_HAS_ERRORCODE OR TRAPFLAG_ALL)

    TRAPINFO KiGeneralProtectionFault
    mov rdx, [rbp + KTRAP_FRAME_Rip]
    lea rcx, MsgGeneralProtFault[rip]
    call qword ptr FrLdrDbgPrint[rip]

    /* Call the C handler */
    call KiGeneralProtectionFaultHandler

    /* Check for success */
    test eax, eax
    jge KiGpfExit

    /* Dispatch the exception */
    mov edx, 3
    mov r9, 0
    mov r10, 0
    mov r11, 0
    call InternalDispatchException

KiGpfFatal:

    /* Bugcheck */
    mov ecx, UNEXPECTED_KERNEL_MODE_TRAP
    mov rdx, HEX(000D) // EXCEPTION_GP_FAULT
    xor r8, r8
    mov r9, [rbp + KTRAP_FRAME_ErrorCode] // error code
    sub rsp, 8
    mov [rsp + KTRAP_FRAME_P5+8], rbp // trap frame
    call KeBugCheckWithTf

KiGpfExit:
    /* Return */
    LEAVE_TRAP_FRAME
    iretq
.ENDP KiGeneralProtectionFault


PUBLIC KiPageFault
.PROC KiPageFault
    /* We have an error code */
    ENTER_TRAP_FRAME (TRAPFLAG_HAS_ERRORCODE OR TRAPFLAG_ALL)

    TRAPINFO KiPageFault

#if 0
    lea rcx, MsgPageFault[rip]
    mov rdx, [rbp + KTRAP_FRAME_ErrorCode]
    mov r8, [rbp + KTRAP_FRAME_Rip]
    mov r9, [rbp + KTRAP_FRAME_FaultAddress]
    call qword ptr FrLdrDbgPrint[rip]
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
    call MmAccessFault

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
    mov r11d, eax // Param3 = Status
    mov eax, STATUS_IN_PAGE_ERROR // ExceptionCode
    mov edx, 3    // ParamCount
    call InternalDispatchException
    jmp PageFaultReturn

AccessViolation:
    /* Use more proper status code */
    mov eax, KI_EXCEPTION_ACCESS_VIOLATION

SpecialCode:
    /* Setup a normal page fault exception */
    mov edx, 2   // ParamCount
    call InternalDispatchException

PageFaultReturn:
    LEAVE_TRAP_FRAME
    iretq
.ENDP KiPageFault


PUBLIC KiFloatingErrorFault
.PROC KiFloatingErrorFault
    /* Push pseudo error code */
    ENTER_TRAP_FRAME TRAPFLAG_ALL

    UNIMPLEMENTED KiFloatingErrorFault

    jmp $
.ENDP KiFloatingErrorFault


PUBLIC KiAlignmentFault
.PROC KiAlignmentFault
    /* We have an error code */
    ENTER_TRAP_FRAME (TRAPFLAG_HAS_ERRORCODE OR TRAPFLAG_ALL)

    /* Enable interrupts */
    sti

    /* Bugcheck */
    mov ecx, EXCEPTION_ALIGNMENT_CHECK
    mov rdx, rbp
    call KiSystemFatalException

    jmp $
.ENDP KiAlignmentFault


PUBLIC KiMcheckAbort
.PROC KiMcheckAbort
    /* Push pseudo error code */
    ENTER_TRAP_FRAME TRAPFLAG_ALL

    /* Bugcheck */
    mov ecx, HEX(12)
    mov rdx, rbp
    call KiSystemFatalException

    jmp $
.ENDP KiMcheckAbort


PUBLIC KiXmmException
.PROC KiXmmException
    /* Push pseudo error code */
    ENTER_TRAP_FRAME TRAPFLAG_ALL

    /* Call the C handler */
    mov rcx, rbp
    call KiXmmExceptionHandler

    /* Check for success */
    test eax, eax
    jge KiXmmExit

    /* Dispatch the exception */
    mov edx, 3
    mov r9, 0
    mov r10, 0
    mov r11, 0
    call InternalDispatchException

KiXmmExit:
    LEAVE_TRAP_FRAME
    iretq
.ENDP KiXmmException


PUBLIC KiApcInterrupt
.PROC KiApcInterrupt
    /* We have an error code */
    ENTER_TRAP_FRAME (TRAPFLAG_HAS_ERRORCODE OR TRAPFLAG_ALL)

    UNIMPLEMENTED KiApcInterrupt

    jmp $
.ENDP KiApcInterrupt


PUBLIC KiRaiseAssertion
.PROC KiRaiseAssertion
    /* We have an error code */
    ENTER_TRAP_FRAME (TRAPFLAG_HAS_ERRORCODE OR TRAPFLAG_ALL)

    /* Decrement RIP to point to the INT2C instruction (2 bytes, not 1 like INT3) */
    sub qword ptr [rbp + KTRAP_FRAME_Rip], 2

    /* Dispatch the exception */
    mov eax, STATUS_ASSERTION_FAILURE
    mov edx, 0
    mov r9, 0
    mov r10, 0
    mov r11, 0
    call InternalDispatchException

    LEAVE_TRAP_FRAME
    iretq
.ENDP KiRaiseAssertion


PUBLIC KiDebugServiceTrap
.PROC KiDebugServiceTrap
    /* Push pseudo error code */
    ENTER_TRAP_FRAME TRAPFLAG_ALL

    TRAPINFO KiDebugServiceTrap

    /* Increase Rip to skip the int3 */
    inc qword ptr [rbp + KTRAP_FRAME_Rip]

    /* Dispatch the exception */
    mov eax, STATUS_BREAKPOINT
    mov edx, 3
    mov r9, [rbp+KTRAP_FRAME_Rax] // Service
    mov r10, [rbp+KTRAP_FRAME_Rcx] // Buffer
    mov r11, [rbp+KTRAP_FRAME_Rdx] // Length
    call InternalDispatchException

    LEAVE_TRAP_FRAME;
    iretq
.ENDP KiDebugServiceTrap


PUBLIC KiDpcInterrupt
.PROC KiDpcInterrupt
    /* We have an error code */
    ENTER_TRAP_FRAME (TRAPFLAG_HAS_ERRORCODE OR TRAPFLAG_ALL)

    UNIMPLEMENTED KiDpcInterrupt

    jmp $
.ENDP KiDpcInterrupt


PUBLIC KiIpiInterrupt
.PROC KiIpiInterrupt
    /* We have an error code */
    ENTER_TRAP_FRAME (TRAPFLAG_HAS_ERRORCODE OR TRAPFLAG_ALL)

    UNIMPLEMENTED KiIpiInterrupt

    jmp $
.ENDP KiIpiInterrupt


PUBLIC KiUnexpectedInterrupt
.PROC KiUnexpectedInterrupt
    /* The error code is the vector */
    cli
    ENTER_TRAP_FRAME (TRAPFLAG_HAS_ERRORCODE OR TRAPFLAG_ALL)

    /* Set bugcheck parameters */
    mov ecx, TRAP_CAUSE_UNKNOWN
    mov rdx, [rbp + KTRAP_FRAME_ErrorCode] // the vector
    mov r8, 0 // The unknown floating-point exception
    mov r9, 0 // The enabled and asserted status bits
    sub rsp, 8
    mov [rbp + KTRAP_FRAME_P5 + 8], rbp // trap frame
    call KeBugCheckWithTf

    jmp $
.ENDP KiUnexpectedInterrupt

#ifdef _MSC_VER
#undef lgdt
#undef lidt

//void __lgdt(void *Source);
PUBLIC __lgdt
__lgdt:
    lgdt fword ptr [rcx]
    ret

//void __sgdt(void *Destination);
PUBLIC __sgdt
__sgdt:
    sgdt fword ptr [rcx]
    ret

// void __lldt(unsigned short Value)
PUBLIC __lldt
__lldt:
    lldt cx
    ret

//void __sldt(void *Destination);
PUBLIC __sldt
__sldt:
    sldt word ptr [rcx]
    ret

//void __ltr(unsigned short Source);
PUBLIC __ltr
__ltr:
    ltr cx
    ret

//void __str(unsigned short *Destination);
PUBLIC __str
__str:
    str word ptr [rcx]
    ret

#endif

END
