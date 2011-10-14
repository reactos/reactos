/*
 * FILE:            ntoskrnl/ke/amd64/trap.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Traps, Entrypoints and Exitpoints
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ksamd64.inc>
#include <trapamd64.inc>

EXTERN KiDispatchException:PROC
EXTERN FrLdrDbgPrint:DWORD
EXTERN KeBugCheckWithTf:PROC
EXTERN MmAccessFault:PROC
EXTERN KiSystemFatalException:PROC
EXTERN KiNpxNotAvailableFaultHandler:PROC
EXTERN KiGeneralProtectionFaultHandler:PROC
EXTERN KiXmmExceptionHandler:PROC
EXTERN KiDeliverApc:PROC
EXTERN KiDispatchInterrupt:PROC

/* GLOBALS *******************************************************************/

.data

PUBLIC MsgUnimplemented
MsgUnimplemented:
.asciz "WARNING:  %s at %s:%d is UNIMPLEMENTED!\n"


/* Helper Macros *************************************************************/

MACRO(DispatchException, Status, Number, P1, P2, P3)
    mov eax, Status
    mov edx, Number
    mov r9, P1
    mov r10, P2
    mov r11, P3
    call InternalDispatchException
ENDM

MACRO(Fatal, BugcheckCode)
    /* Bugcheck */
    mov ecx, BugcheckCode
    mov rdx, rbp
    call KiSystemFatalException
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
.ENDP


/* SOFTWARE INTERRUPT SERVICES ***********************************************/

PUBLIC KiDivideErrorFault
FUNC KiDivideErrorFault
    /* Push pseudo error code */
    EnterTrap TF_SAVE_ALL

    /* Enable interrupts */
    sti

    /* Dispatch the exception */
    DispatchException STATUS_INTEGER_DIVIDE_BY_ZERO, 0, 0, 0, 0

    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC


PUBLIC KiDebugTrapOrFault
FUNC KiDebugTrapOrFault
    /* Push pseudo error code */
    EnterTrap TF_SAVE_ALL

    /* Check if the frame was from kernelmode */
    test word ptr [rbp + KTRAP_FRAME_SegCs], 3
    jz KiDebugTrapOrFaultKMode

    /* Enable interrupts for user-mode */
    sti

KiDebugTrapOrFaultKMode:
    /* Dispatch the exception */
    DispatchException STATUS_SINGLE_STEP, 0, 0, 0, 0

    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC


PUBLIC KiNmiInterrupt
FUNC KiNmiInterrupt
    /* Push pseudo error code */
    EnterTrap TF_SAVE_ALL

    UNIMPLEMENTED KiNmiInterrupt
    int 3

    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC


PUBLIC KiBreakpointTrap
FUNC KiBreakpointTrap
    /* Push pseudo error code */
    EnterTrap TF_SAVE_ALL

    /* Dispatch the exception */
    DispatchException STATUS_BREAKPOINT, 3, 0, 0, 0

    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC


PUBLIC KiOverflowTrap
FUNC KiOverflowTrap
    /* Push pseudo error code */
    EnterTrap TF_SAVE_ALL

    /* Enable interrupts */
    sti

    /* Dispatch the exception */
    DispatchException STATUS_INTEGER_OVERFLOW, 3, 0, 0, 0

    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC


PUBLIC KiBoundFault
FUNC KiBoundFault
   /* No error code */
    EnterTrap TF_SAVE_ALL

    /* Check if the frame was from kernelmode */
    test word ptr [rbp + KTRAP_FRAME_SegCs], 3
    jnz KiBoundFaultUserMode

    /* Bugcheck */
    Fatal EXCEPTION_BOUND_CHECK

KiBoundFaultUserMode:
    /* Enable interrupts for user-mode */
    sti

    /* Dispatch the exception */
    DispatchException STATUS_ARRAY_BOUNDS_EXCEEDED, 0, 0, 0, 0

    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC


PUBLIC KiInvalidOpcodeFault
FUNC KiInvalidOpcodeFault
   /* No error code */
    EnterTrap TF_SAVE_ALL

    /* Enable interrupts */
    sti

    /* Check if the frame was from kernelmode */
    test word ptr [rbp + KTRAP_FRAME_SegCs], 3
    jz KiInvalidOpcodeKernel

    // FIXME: handle STATUS_INVALID_LOCK_SEQUENCE

KiInvalidOpcodeKernel:
    /* Kernel mode fault */

    /* Dispatch the exception */
    DispatchException STATUS_ILLEGAL_INSTRUCTION, 3, 0, 0, 0

    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC


PUBLIC KiNpxNotAvailableFault
FUNC KiNpxNotAvailableFault
   /* No error code */
    EnterTrap TF_SAVE_ALL

    /* Call the C handler */
    mov rcx, rbp
    call KiNpxNotAvailableFaultHandler

    /* Check the return status code */
    test eax, eax
    jz KiNpxNotAvailableFaultExit

    /* Dispatch the exception */
    DispatchException eax, 3, 0, 0, 0

KiNpxNotAvailableFaultExit:
    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC


PUBLIC KiDoubleFaultAbort
FUNC KiDoubleFaultAbort
   /* No error code */
    EnterTrap TF_SAVE_ALL

    /* Bugcheck */
    Fatal 8 // EXCEPTION_DOUBLE_FAULT
    jmp $
ENDFUNC


PUBLIC KiNpxSegmentOverrunAbort
FUNC KiNpxSegmentOverrunAbort
   /* No error code */
    EnterTrap TF_SAVE_ALL

    /* Bugcheck */
    Fatal EXCEPTION_NPX_OVERRUN
    jmp $
ENDFUNC


PUBLIC KiInvalidTssFault
FUNC KiInvalidTssFault
    /* We have an error code */
    EnterTrap (TF_HAS_ERROR_CODE OR TF_SAVE_ALL)

    /* Bugcheck */
    Fatal EXCEPTION_INVALID_TSS
    jmp $
ENDFUNC


PUBLIC KiSegmentNotPresentFault
FUNC KiSegmentNotPresentFault
    /* We have an error code */
    EnterTrap (TF_HAS_ERROR_CODE OR TF_SAVE_ALL)

    /* Bugcheck */
    Fatal EXCEPTION_SEGMENT_NOT_PRESENT
    jmp $
ENDFUNC


PUBLIC KiStackFault
FUNC KiStackFault
    /* We have an error code */
    EnterTrap (TF_HAS_ERROR_CODE OR TF_SAVE_ALL)

    /* Bugcheck */
    Fatal EXCEPTION_STACK_FAULT
    jmp $
ENDFUNC


PUBLIC KiGeneralProtectionFault
FUNC KiGeneralProtectionFault
    /* We have an error code */
    EnterTrap (TF_HAS_ERROR_CODE OR TF_SAVE_ALL)

    /* Call the C handler */
    mov rcx, rbp
    call KiGeneralProtectionFaultHandler

    /* Check for success */
    test eax, eax
    jge KiGpfExit

    /* Dispatch the exception */
    DispatchException eax, 3, 0, 0, 0

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
    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC


PUBLIC KiPageFault
FUNC KiPageFault
    /* We have an error code */
    EnterTrap (TF_HAS_ERROR_CODE OR TF_SAVE_ALL)

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
    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC


PUBLIC KiFloatingErrorFault
FUNC KiFloatingErrorFault
   /* No error code */
    EnterTrap TF_SAVE_ALL

    UNIMPLEMENTED KiFloatingErrorFault
    int 3

    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC


PUBLIC KiAlignmentFault
FUNC KiAlignmentFault
    /* We have an error code */
    EnterTrap (TF_HAS_ERROR_CODE OR TF_SAVE_ALL)

    /* Bugcheck */
    Fatal EXCEPTION_ALIGNMENT_CHECK
    jmp $
ENDFUNC


PUBLIC KiMcheckAbort
FUNC KiMcheckAbort
   /* No error code */
    EnterTrap TF_SAVE_ALL

    /* Bugcheck */
    Fatal HEX(12)
    jmp $
ENDFUNC


PUBLIC KiXmmException
FUNC KiXmmException
   /* No error code */
    EnterTrap TF_SAVE_ALL

    /* Call the C handler */
    mov rcx, rbp
    call KiXmmExceptionHandler

    /* Check for success */
    test eax, eax
    jge KiXmmExit

    /* Dispatch the exception */
    DispatchException eax, 3, 0, 0, 0

KiXmmExit:
    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC


PUBLIC KiRaiseAssertion
FUNC KiRaiseAssertion
    /* We have an error code */
    EnterTrap (TF_HAS_ERROR_CODE OR TF_SAVE_ALL)

    /* Decrement RIP to point to the INT2C instruction (2 bytes, not 1 like INT3) */
    sub qword ptr [rbp + KTRAP_FRAME_Rip], 2

    /* Dispatch the exception */
    DispatchException STATUS_ASSERTION_FAILURE, 0, 0, 0, 0

    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC


PUBLIC KiDebugServiceTrap
.PROC KiDebugServiceTrap
   /* No error code */
    EnterTrap TF_SAVE_ALL

    /* Increase Rip to skip the int3 */
    inc qword ptr [rbp + KTRAP_FRAME_Rip]

    /* Dispatch the exception (Params = service, buffer, legth) */
    DispatchException STATUS_BREAKPOINT, 3, [rbp+KTRAP_FRAME_Rax], [rbp+KTRAP_FRAME_Rcx], [rbp+KTRAP_FRAME_Rdx]

    /* Return */
    ExitTrap TF_SAVE_ALL
.ENDP


PUBLIC KiApcInterrupt
.PROC KiApcInterrupt
   /* No error code */
    EnterTrap (TF_VOLATILES or TF_IRQL)

    /* Raise to APC_LEVEL */
    mov rax, APC_LEVEL
    mov cr8, rax

    /* End the interrupt */
    mov dword ptr [APIC_EOI], 0

    /* Enable interrupts  */
    sti

    /* Call the worker routine */
    mov cl, [rbp + KTRAP_FRAME_SegCs] // ProcessorMode
    and cl, 1
    mov rdx, 0                        // ExceptionFrame
    mov r8, rdx                       // TrapFrame
    call KiDeliverApc

    /* Disable interrupts */
    cli

    /* Return */
    ExitTrap (TF_VOLATILES or TF_IRQL)
.ENDP


PUBLIC KiDpcInterrupt
.PROC KiDpcInterrupt
   /* No error code */
    EnterTrap (TF_VOLATILES or TF_IRQL)

    /* Raise to DISPATCH_LEVEL */
    mov rax, DISPATCH_LEVEL
    mov cr8, rax

    /* End the interrupt */
    mov dword ptr [APIC_EOI], 0

    /* Call the worker routine */
    sti
    call KiDispatchInterrupt
    cli

    /* Return */
    ExitTrap (TF_VOLATILES or TF_IRQL)
.ENDP


PUBLIC KiIpiInterrupt
.PROC KiIpiInterrupt
   /* No error code */
    EnterTrap (TF_VOLATILES or TF_IRQL)

    /* Raise to IPI_LEVEL */
    mov rax, IPI_LEVEL
    mov cr8, rax

    /* End the interrupt */
    mov dword ptr [APIC_EOI], 0

    int 3

    /* Return */
    ExitTrap (TF_VOLATILES or TF_IRQL)
.ENDP


PUBLIC KiUnexpectedInterrupt
FUNC KiUnexpectedInterrupt
    /* The error code is the vector */
    EnterTrap (TF_HAS_ERROR_CODE OR TF_SAVE_ALL)

#if 0
    /* Set bugcheck parameters */
    mov ecx, TRAP_CAUSE_UNKNOWN
    mov rdx, [rbp + KTRAP_FRAME_ErrorCode] // the vector
    mov r8, 0 // The unknown floating-point exception
    mov r9, 0 // The enabled and asserted status bits
    sub rsp, 8
    mov [rbp + KTRAP_FRAME_P5 + 8], rbp // trap frame
    call KeBugCheckWithTf
    jmp $
#endif
    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC

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
