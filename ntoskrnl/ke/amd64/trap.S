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
EXTERN KeBugCheckWithTf:PROC
EXTERN MmAccessFault:PROC
EXTERN KiSystemFatalException:PROC
EXTERN KiNpxNotAvailableFaultHandler:PROC
EXTERN KiGeneralProtectionFaultHandler:PROC
EXTERN KiXmmExceptionHandler:PROC
EXTERN KiDeliverApc:PROC
EXTERN KiDpcInterruptHandler:PROC
EXTERN PsConvertToGuiThread:PROC
EXTERN MmCreateKernelStack:PROC
EXTERN MmDeleteKernelStack:PROC
EXTERN KdSetOwedBreakpoints:PROC
EXTERN KeAcquireSpinLockAtDpcLevel:PROC
EXTERN KeReleaseSpinLockFromDpcLevel:PROC


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

MACRO(UnexpectedVectorStub, Vector)
    /* This nop is to make the relative jmp address 4 bytes aligned and to
       make the whole code 8 bytes long */
    nop
    /* This is a push instruction with 8bit operand. Since the instruction
       sign extends the value to 32 bits, we need to offset it */
PUBLIC KxUnexpectedInterrupt&Vector
KxUnexpectedInterrupt&Vector:
    push (Vector - 128)
    jmp KiUnexpectedInterrupt
ENDM

PUBLIC KiUnexpectedRange
KiUnexpectedRange:
Vector = 0
REPEAT 256
    UnexpectedVectorStub %Vector
    Vector = Vector+1
ENDR
PUBLIC KiUnexpectedRangeEnd
KiUnexpectedRangeEnd:

PUBLIC KiInterruptDispatchTemplate
KiInterruptDispatchTemplate:
    /* This instruction pushes the return address on the stack, which is the
       address of the interrupt object's DispatchCode member, then jumps
       to the address stored in the interrupt object's DispatchAddress member */
    call qword ptr KiInterruptDispatchTemplate[rip - KINTERRUPT_DispatchCode + KINTERRUPT_DispatchAddress]


// rbp = TrapFrame, eax = ExceptionCode, edx = NumParams, r9,r10,r11 = params
FUNC InternalDispatchException

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
ENDFUNC


/* CPU EXCEPTION HANDLERS ****************************************************/

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

EXTERN KiNmiInterruptHandler:PROC

FUNC KiNmiInterruptWithEf
    /* Generate a KEXCEPTION_FRAME on the stack */
    GENERATE_EXCEPTION_FRAME

    /* Call the C handler */
    lea rcx, [rsp + KEXCEPTION_FRAME_LENGTH]
    lea rdx, [rsp]
    call KiNmiInterruptHandler

    /* Restore the registers from the KEXCEPTION_FRAME */
    RESTORE_EXCEPTION_STATE

    /* Return */
    ret
ENDFUNC

PUBLIC KiNmiInterrupt
FUNC KiNmiInterrupt
    /* Push pseudo error code */
    EnterTrap TF_SAVE_ALL

    call KiNmiInterruptWithEf

    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC


PUBLIC KiBreakpointTrap
FUNC KiBreakpointTrap
    /* Push pseudo error code */
    EnterTrap TF_SAVE_ALL

    /* Check if the frame was from kernelmode */
    test word ptr [rbp + KTRAP_FRAME_SegCs], 3
    jz KiBreakpointTrapKMode

    /* Enable interrupts for user-mode */
    sti

KiBreakpointTrapKMode:
    /* Dispatch the exception */
    DispatchException STATUS_BREAKPOINT, 3, BREAKPOINT_BREAK, 0, 0

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
    /* Push pseudo error code */
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
    /* Push pseudo error code */
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
    DispatchException STATUS_ILLEGAL_INSTRUCTION, 0, 0, 0, 0

    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC


PUBLIC KiNpxNotAvailableFault
FUNC KiNpxNotAvailableFault
    /* Push pseudo error code */
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

    /* Hack for VBox, which "forgets" to push an error code on the stack! */
    and rsp, HEX(FFFFFFFFFFFFFFF0)

    /* A zero error code is pushed */
    EnterTrap (TF_HAS_ERROR_CODE OR TF_SAVE_ALL)

    int 3

    /* Bugcheck */
    Fatal 8 // EXCEPTION_DOUBLE_FAULT
    jmp $
ENDFUNC


PUBLIC KiNpxSegmentOverrunAbort
FUNC KiNpxSegmentOverrunAbort
    /* Push pseudo error code */
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

    /* Check for access violation */
    cmp eax, STATUS_ACCESS_VIOLATION
    je DispatchAccessViolation

    /* Dispatch privileged instruction fault */
    DispatchException eax, 0, 0, 0, 0
    jmp KiGpfExit

DispatchAccessViolation:

    /* Dispatch access violation */
    DispatchException eax, 2, 0, -1, 0

KiGpfExit:
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

    /* If interrupts are off, do not enable them */
    test dword ptr [rbp + KTRAP_FRAME_EFlags], EFLAGS_IF_MASK
    jz IntsDisabled

    /* Enable interrupts for the page fault handler */
    sti

IntsDisabled:

    /* Call page fault handler */
    mov ecx, [rbp + KTRAP_FRAME_ErrorCode] // FaultCode
    // rdx == Address
    mov r8b, [rbp + KTRAP_FRAME_SegCs] // Mode
    and r8b, 1
    mov r9, rbp // TrapInformation
    call MmAccessFault

    /* Check for success */
    test eax, eax
    jl PageFaultError

    /* Check whether the kernel debugger has owed breakpoints to be inserted */
    call KdSetOwedBreakpoints
    /* We succeeded, return */
    jmp PageFaultReturn

PageFaultError:

    /* Set parameter 1 to write/execute flag.
       See https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-exception_record */
    mov r9d, [rbp + KTRAP_FRAME_ErrorCode]
    shr r9d, 1
    and r9d, 9

    /* Set parameter 2 to faulting address */
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

    /* Disable interrupts for the return */
    cli

    /* Return */
    ExitTrap (TF_SAVE_ALL or TF_CHECKUSERAPC)
ENDFUNC


PUBLIC KiFloatingErrorFault
FUNC KiFloatingErrorFault
    /* Push pseudo error code */
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
    /* Push pseudo error code */
    EnterTrap TF_SAVE_ALL

    /* Bugcheck */
    Fatal HEX(12)
    jmp $
ENDFUNC


PUBLIC KiXmmException
FUNC KiXmmException
    /* Push pseudo error code */
    EnterTrap TF_SAVE_ALL

    /* Call the C handler */
    mov rcx, rbp
    call KiXmmExceptionHandler

    /* Check for success */
    test eax, eax
    jge KiXmmExit

    /* Dispatch the exception */
    DispatchException eax, 2, 0, [rbp+KTRAP_FRAME_MxCsr], 0

    // FIXME: STATUS_FLOAT_MULTIPLE_TRAPS / STATUS_FLOAT_MULTIPLE_FAULTS

KiXmmExit:
    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC


/* SOFTWARE INTERRUPT SERVICES ***********************************************/

PUBLIC KiRaiseAssertion
FUNC KiRaiseAssertion
    /* We have an error code */
    EnterTrap (TF_SAVE_ALL)

    /* Decrement RIP to point to the INT2C instruction (2 bytes, not 1 like INT3) */
    sub qword ptr [rbp + KTRAP_FRAME_Rip], 2

    /* Dispatch the exception */
    DispatchException STATUS_ASSERTION_FAILURE, 0, 0, 0, 0

    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC


PUBLIC KiDebugServiceTrap
FUNC KiDebugServiceTrap
   /* No error code */
    EnterTrap TF_SAVE_ALL

    /* Increase Rip to skip the int3 */
    inc qword ptr [rbp + KTRAP_FRAME_Rip]

    /* Dispatch the exception (Params = service, buffer, legth) */
    DispatchException STATUS_BREAKPOINT, 3, [rbp+KTRAP_FRAME_Rax], [rbp+KTRAP_FRAME_Rcx], [rbp+KTRAP_FRAME_Rdx]

    /* Return */
    ExitTrap TF_SAVE_ALL
ENDFUNC


PUBLIC KiApcInterrupt
.PROC KiApcInterrupt
   /* No error code */
    EnterTrap (TF_SAVE_ALL or TF_IRQL)

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
    mov r8, rbp                       // TrapFrame
    call KiDeliverApc

    /* Disable interrupts */
    cli

    /* Lower IRQL back to PASSIVE */
    mov rax, PASSIVE_LEVEL
    mov cr8, rax

    /* Return */
    ExitTrap (TF_SAVE_ALL or TF_IRQL)
.ENDP

/*
 * VOID
 * KiRetireDpcList(
 *     PKPRCB Prcb);
 */
EXTERN KiRetireDpcList:PROC

/*
 * VOID
 * KiRetireDpcListInDpcStack(
 *     PKPRCB Prcb,
 *     PVOID DpcStack);
 */
PUBLIC KiRetireDpcListInDpcStack
.PROC KiRetireDpcListInDpcStack
    push rbp
    .pushreg rbp
    mov rbp, rsp
    .setframe rbp, 0
    .endprolog

    /* Switch to the DpcStack */
    mov rsp, rdx

    /* The stack is 16 byte aligned, allocate 32 bytes home space */
    sub rsp, 32

    /* Call KiRetireDpcList on the given stack */
    call KiRetireDpcList

    /* Restore stack, cleanup and return */
    mov rsp, rbp
    pop rbp
    ret
.ENDP

PUBLIC KiDpcInterrupt
.PROC KiDpcInterrupt
   /* No error code */
    EnterTrap (TF_SAVE_ALL or TF_IRQL)

    /* Call the worker routine */
    call KiDpcInterruptHandler

    /* Return, but don't send an EOI! */
    ExitTrap (TF_SAVE_ALL or TF_IRQL)
.ENDP


PUBLIC KiIpiInterrupt
.PROC KiIpiInterrupt
   /* No error code */
    EnterTrap (TF_SAVE_ALL or TF_IRQL)

    /* Raise to IPI_LEVEL */
    mov rax, IPI_LEVEL
    mov cr8, rax

    /* End the interrupt */
    mov dword ptr [APIC_EOI], 0

    int 3

    /* Return */
    ExitTrap (TF_SAVE_ALL or TF_IRQL)
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

PUBLIC KiInterruptDispatch
FUNC KiInterruptDispatch
    /* The error code is a pointer to the interrupt object's code */
    EnterTrap (TF_HAS_ERROR_CODE or TF_SAVE_ALL or TF_IRQL)

    /* Increase interrupt count */
    inc dword ptr gs:[PcInterruptCount];

    /* Save rbx and rsi in the trap frame */
    mov [rbp + KTRAP_FRAME_Rbx], rbx
    mov [rbp + KTRAP_FRAME_Rsi], rsi

    /* Load the address of the dispatch code into rbx */
    mov rbx, [rbp + KTRAP_FRAME_ErrorCode]

    /* Substract offset of the DispatchCode member plus 6 for the call instruction */
    sub rbx, KINTERRUPT_DispatchCode + 6

    /* Save the address of the InterruptListEntry in rsi */
    lea rsi, [rbx + KINTERRUPT_InterruptListEntry]

.DoDispatchInterrupt:
    /* Raise IRQL to SynchronizeIrql */
    movzx rax, byte ptr [rbx + KINTERRUPT_SynchronizeIrql]
    mov cr8, rax

    /* Enable interrupts */
    sti

#ifdef CONFIG_SMP
    /* Acquire interrupt lock */
    mov rcx, [rbx + KINTERRUPT_ActualLock]
    call KeAcquireSpinLockAtDpcLevel
#endif

    /* Call the ISR */
    mov rcx, rbx
    mov rdx, [rbx + KINTERRUPT_ServiceContext]
    call qword ptr [rbx + KINTERRUPT_ServiceRoutine]

#ifdef CONFIG_SMP
    /* Release interrupt lock */
    mov rcx, [rbx + KINTERRUPT_ActualLock]
    call KeReleaseSpinLockFromDpcLevel
#endif

    /* Disable interrupts and go back to old irql */
    cli
    movzx rax, byte ptr [rbp + KTRAP_FRAME_PreviousIrql]
    mov cr8, rax

    /* Check for chained interrupts */
    mov rax, [rbx + KINTERRUPT_InterruptListEntry]
    cmp rax, rsi
    je .Done

    /* Load the next interrupt object into rbx and repeat */
    lea rbx, [rax - KINTERRUPT_InterruptListEntry]
    jmp .DoDispatchInterrupt

.Done:
    /* Restore rbx and rsi */
    mov rbx, [rbp + KTRAP_FRAME_Rbx]
    mov rsi, [rbp + KTRAP_FRAME_Rsi]

    /* Return */
    ExitTrap (TF_SAVE_ALL or TF_SEND_EOI)
ENDFUNC

EXTERN KiSystemCallHandler:PROC

/*! \name KiSystemCallEntry64
 *
 *  \brief This is the entrypoint for syscalls from 64bit user mode
 *
 *  \param rax - The system call number
 *  \param rcx - User mode return address, set by the syscall instruction
 *  \param rdx,r8,r9 - Parameters 2-4 to the service function
 *  \param r10 - Parameter 1 to the service function
 *  \param r11 - RFLAGS saved by the syscall instruction
 *--*/
PUBLIC KiSystemCallEntry64
.PROC KiSystemCallEntry64
    /* The unwind info pretends we have a machine frame */
    .PUSHFRAME
    .ALLOCSTACK (KTRAP_FRAME_LENGTH + MAX_SYSCALL_PARAM_SIZE - MachineFrameLength)
    .SAVEREG rbp, MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rbp
    .ENDPROLOG

    /* Swap gs to kernel, so we can access the PCR */
    swapgs

    /* Save the user mode rsp in the PCR */
    mov gs:[PcUserRsp], rsp

    /* Get the kernel stack from the PCR */
    mov rsp, gs:[PcRspBase]

    /* Allocate a TRAP_FRAME and space for parameters */
    sub rsp, (KTRAP_FRAME_LENGTH + MAX_SYSCALL_PARAM_SIZE)

    /* Save volatile registers and rbp in the trap frame */
    mov [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rax], rax
    mov [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rip], rcx
    mov [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rdx], rdx
    mov [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_R8], r8
    mov [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_R9], r9
    mov [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rcx], r10
    mov [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_EFlags], r11
    mov [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rbp], rbp

    /* Store user stack pointer in the trap frame */
    mov rax, gs:[PcUserRsp]
    mov [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rsp], rax

    /* Set sane segments */
    mov ax, (KGDT64_R3_DATA or RPL_MASK)
    mov ds, ax
    mov es, ax

    /* Save MCXSR and set kernel value */
    stmxcsr [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_MxCsr]
    ldmxcsr gs:[PcMxCsr]

    /* Get the current thread and the trap frame */
    mov rax, gs:[PcCurrentThread]
    mov rcx, [rax + ThTrapFrame]

    /* Save the old trap frame */
    lea rdx, [rsp + MAX_SYSCALL_PARAM_SIZE]
    mov [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_TrapFrame], rcx
    mov [rax + ThTrapFrame], rdx

#if DBG
    /* Check IRQL */
    mov rax, cr8
    test eax, eax
    jz KiSystemCall64Again
    int HEX(2C)
#endif

GLOBAL_LABEL KiSystemCall64Again

    /* Call the C-handler (will enable interrupts) */
    call KiSystemCallHandler

    /* The return value from KiSystemCallHandler is the address of the Nt-function */
    mov rcx, [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rcx]
    mov rdx, [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rdx]
    mov r8, [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_R8]
    mov r9, [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_R9]
    call rax

GLOBAL_LABEL KiSystemServiceExit

    ASSERT_TRAP_FRAME_INTS_ENABLED rsp + MAX_SYSCALL_PARAM_SIZE

    /* Check for pending user APCs */
    mov rcx, gs:[PcCurrentThread]
    cmp byte ptr [rcx + ThApcState + AsUserApcPending], 0
    jz NoUserApcPending

    /* Save missing regs in the trap frame */
    mov [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rax], rax
    mov [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rbp], rbp
    mov [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_R9], rbp
    mov rax, [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rsp]
    mov [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_R8], rax
    mov rax, [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rip]
    mov [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rcx], rax
    mov rax, [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_EFlags]
    mov [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_R11], rax
    xor rax, rax
    mov [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rdx], rax
    mov [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_R10], rax
    pxor xmm0, xmm0
    movdqa [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Xmm0], xmm0
    movdqa [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Xmm1], xmm0
    movdqa [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Xmm2], xmm0
    movdqa [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Xmm3], xmm0
    movdqa [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Xmm4], xmm0
    movdqa [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Xmm5], xmm0

    lea rcx, [rsp + MAX_SYSCALL_PARAM_SIZE]
    call KiInitiateUserApc

NoUserApcPending:
    /* Disable interrupts for return */
    cli

    /* Restore MCXSR */
    ldmxcsr [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_MxCsr]

    /* Restore old trap frame */
    mov rcx, gs:[PcCurrentThread]
    mov rdx, [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_TrapFrame]
    mov [rcx + KTHREAD_TrapFrame], rdx

    /* Prepare user mode return address (rcx) and eflags (r11) for sysret */
    mov rcx, [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rip]
    mov r11, [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_EFlags]

    /* Load user mode stack (It was copied to the trap frame) */
    mov rsp, [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rsp]

    /* r8 points to the user stack */
    mov r8, rsp

    /* r9 matches rbp */
    mov r9, rbp

    /* Swap gs back to user */
    swapgs

    /* Zero out volatiles */
    pxor xmm0, xmm0
    pxor xmm1, xmm1
    pxor xmm2, xmm2
    pxor xmm3, xmm3
    pxor xmm4, xmm4
    pxor xmm5, xmm5
    xor rdx, rdx
    xor r10, r10

    /* return to user mode */
    sysretq

.ENDP


/*!
 * VOID
 * DECLSPEC_NORETURN
 * KiExceptionExit(
 *     _In_ PKTRAP_FRAME TrapFrame@<rcx>,
 *     _In_ PKEXCEPTION_FRAME ExceptionFrame@<rdx>);
 */
PUBLIC KiExceptionExit
KiExceptionExit:

    /* Restore registers from exception frame */
    movaps xmm6, [rdx + ExXmm6]
    movaps xmm7, [rdx + ExXmm7]
    movaps xmm8, [rdx + ExXmm8]
    movaps xmm9, [rdx + ExXmm9]
    movaps xmm10, [rdx + ExXmm10]
    movaps xmm11, [rdx + ExXmm11]
    movaps xmm12, [rdx + ExXmm12]
    movaps xmm13, [rdx + ExXmm13]
    movaps xmm14, [rdx + ExXmm14]
    movaps xmm15, [rdx + ExXmm15]
    mov rbx, [rdx + ExRbx]
    mov rdi, [rdx + ExRdi]
    mov rsi, [rdx + ExRsi]
    mov r12, [rdx + ExR12]
    mov r13, [rdx + ExR13]
    mov r14, [rdx + ExR14]
    mov r15, [rdx + ExR15]

    /* Point rsp at the trap frame */
    mov rsp, rcx
    /* Fall through */

/*
 * Internal function. Exits to user-mode with rsp pointing to the trap frame.
 * All non-volatile register context must be set up already.
 * Used by KiInitializeContextThread to set up the init path for a new thread.
 */
PUBLIC KiServiceExit3
.PROC KiServiceExit3
    .PUSHFRAME
    .ALLOCSTACK (KTRAP_FRAME_LENGTH - MachineFrameLength)
    .ENDPROLOG

    /* Return */
    mov rbp, rsp
    ExitTrap (TF_SEGMENTS or TF_CHECKUSERAPC)
.ENDP


PUBLIC KiSystemCallEntry32
KiSystemCallEntry32:
    swapgs
    int 3


PUBLIC KiZwSystemService
FUNC KiZwSystemService
    push rbp
    .pushreg rbp
    sub rsp, KTRAP_FRAME_LENGTH
    .allocstack KTRAP_FRAME_LENGTH
    mov [rsp + KTRAP_FRAME_Rsi], rsi
    .savereg rsi, KTRAP_FRAME_Rsi
    mov [rsp + KTRAP_FRAME_Rdi], rdi
    .savereg rdi, KTRAP_FRAME_Rdi
    mov rbp, rsp
    .setframe rbp, 0
    .endprolog

    /* Get current thread */
    mov r11, gs:[PcCurrentThread]

    /* Save PreviousMode in the trap frame */
    mov dil, byte ptr [r11 + KTHREAD_PreviousMode]
    mov byte ptr [rbp + KTRAP_FRAME_PreviousMode], dil

    /* Save the old trap frame in TrapFrame.Rdx */
    mov rdi, [r11 + KTHREAD_TrapFrame]
    mov [rbp + KTRAP_FRAME_Rdx], rdi

    /* Set the new trap frame and previous mode */
    mov [r11 + ThTrapFrame], rbp
    mov byte ptr [r11 + KTHREAD_PreviousMode], 0

    /* allocate space for parameters */
    sub rsp, r10
    and rsp, HEX(0fffffffffffffff0)

    /* Save rcx */
    mov [rbp + KTRAP_FRAME_Rcx], rcx

    /* copy parameters to the new location */
    lea rsi, [rbp + KTRAP_FRAME_LENGTH + 16]
    lea rdi, [rsp]
    mov rcx, r10
    shr rcx, 3
    rep movsq

    /* Restore rcx */
    mov rcx, [rbp + KTRAP_FRAME_Rcx]

    /* Call the service function */
    call rax

    /* Restore the old trap frame */
    mov r11, gs:[PcCurrentThread]
    mov rsi, [rbp + KTRAP_FRAME_Rdx]
    mov [r11 + KTHREAD_TrapFrame], rsi

    /* Restore PreviousMode from the trap frame */
    mov dil, byte ptr [rbp + KTRAP_FRAME_PreviousMode]
    mov byte ptr [r11 + KTHREAD_PreviousMode], dil

    /* Restore rdi and rsi */
    mov rsi, [rbp + KTRAP_FRAME_Rsi]
    mov rdi, [rbp + KTRAP_FRAME_Rdi]

    /* Cleanup the stack and return */
    lea rsp, [rbp + KTRAP_FRAME_LENGTH]
    pop rbp
    ret

ENDFUNC

PUBLIC KiConvertToGuiThread
FUNC KiConvertToGuiThread

    sub rsp, 40
    .allocstack 40
    .endprolog

    /* Check if we already have a large stack */
    mov rax, gs:[PcCurrentThread]
    cmp byte ptr [rax + KTHREAD_LargeStack], 0
    jnz AlreadyLargeStack

    // NewStack = (ULONG_PTR)MmCreateKernelStack(TRUE, 0);
    mov cl, 1
    xor rdx, rdx
    call MmCreateKernelStack

    /* Check for failure */
    test rax, rax
    jz KiConvertToGuiThreadFailed

    /* OldStack = KeSwitchKernelStack((PVOID)NewStack, (PVOID)(NewStack - KERNEL_LARGE_STACK_COMMIT )); */
    mov rcx, rax
    mov rdx, rax
    sub rdx, KERNEL_LARGE_STACK_COMMIT
    call KeSwitchKernelStack

    // MmDeleteKernelStack(OldStack, FALSE);
    mov rcx, rax
    xor rdx, rdx
    call MmDeleteKernelStack

AlreadyLargeStack:

    /* Call the worker function */
    call PsConvertToGuiThread

    /* Check for failure */
    test eax, eax
    js KiConvertToGuiThreadFailed

    /* Disable interrupts for return */
    cli

    // Restore register parameters
    mov rcx, [rsp + 48 + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rip]
    mov rdx, [rsp + 48 + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rdx]
    mov r8, [rsp + 48 + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_R8]
    mov r9, [rsp + 48 + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_R9]

    /* Run KiSystemCallHandler again */
    add rsp, 48
    jmp KiSystemCall64Again

KiConvertToGuiThreadFailed:

    /* Clean up the stack and return failure */
    add rsp, 40
    mov eax, HEX(C0000017) // STATUS_NO_MEMORY
    ret

ENDFUNC


EXTERN KiSetTrapContextInternal:PROC

/*
 * VOID
 * KiSetTrapContext(
 *     _Out_ PKTRAP_FRAME TrapFrame,
 *     _In_ PCONTEXT Context,
 *     _In_ KPROCESSOR_MODE RequestorMode);
 */
PUBLIC KiSetTrapContext
.PROC KiSetTrapContext

    /* Generate a KEXCEPTION_FRAME on the stack */
    GENERATE_EXCEPTION_FRAME

    call KiSetTrapContextInternal

    /* Restore the registers from the KEXCEPTION_FRAME */
    RESTORE_EXCEPTION_STATE

    /* Return */
    ret

.ENDP


/*
 * VOID
 * KiDeliverApc(
 *     _In_ KPROCESSOR_MODE DeliveryMode,
 *     _In_ PKEXCEPTION_FRAME ExceptionFrame,
 *     _In_ PKTRAP_FRAME TrapFrame);
 *
 */
EXTERN KiDeliverApc:PROC

/*
 * VOID
 * KiInitiateUserApc(
 *     _In_ PKTRAP_FRAME TrapFrame@<rcx>);
 *
 * This function is called to deliver user mode APCs.
 * It clobbers all non-volatile registers, except rax.
 */
PUBLIC KiInitiateUserApc
.PROC KiInitiateUserApc

    /* Generate a KEXCEPTION_FRAME on the stack */
    GENERATE_EXCEPTION_FRAME

    /* Raise IRQL to APC_LEVEL */
    mov rax, APC_LEVEL
    mov cr8, rax

    /* Get the current thread */
    mov rbp, gs:[PcCurrentThread]

    /* Save the trap frame in rsi */
    mov rsi, rcx

    /* Enable interrupts */
    sti

    /* Call the C function */
    mov ecx, 1
    mov rdx, rsp
    mov r8, rsi
    call KiDeliverApc

    /* Disable interrupts again */
    cli

    /* Go back to PASSIVE_LEVEL */
    mov rax, PASSIVE_LEVEL
    mov cr8, rax

    /* Restore the registers from the KEXCEPTION_FRAME */
    RESTORE_EXCEPTION_STATE

    /* Return */
    ret

.ENDP


PUBLIC KiInitializeSegments
KiInitializeSegments:
    mov ax, KGDT64_R3_DATA or RPL_MASK
    mov gs, ax
    swapgs
    mov gs, ax
    ret

/*!
 * VOID
 * KiSwitchKernelStackHelper(
 *     LONG_PTR StackOffset,
 *     PVOID OldStackBase);
 */
PUBLIC KiSwitchKernelStackHelper
KiSwitchKernelStackHelper:

    /* Pop return address from the current stack */
    pop rax

    /* Switch to new stack */
    lea rsp, [rsp + rcx]

    /* Push return address on the new stack */
    push rax

    /* Return on new stack */
    mov rax, rdx
    ret

EXTERN KiSwitchKernelStack:PROC

PUBLIC KeSwitchKernelStack
FUNC KeSwitchKernelStack

    /* Save rcx and allocate callee home space */
    mov [rsp + P1Home], rcx
    .savereg rcx, P1Home
    sub rsp, 40
    .allocstack 40
    .endprolog

    /* Call the C handler, which returns the old stack in rax */
    call KiSwitchKernelStack

    /* Restore rcx (StackBase) */
    mov rcx, [rsp + 40 + P1Home]

    /* Switch to new stack: RSP += (StackBase - OldStackBase) */
    sub rcx, rax
    add rsp, rcx

    /* Deallocate the home frame */
    add rsp, 40
    ret

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

PUBLIC __swapgs
__swapgs:
    swapgs
    ret

#endif

END
