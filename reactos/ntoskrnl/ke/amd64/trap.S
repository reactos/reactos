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
EXTERN KiDpcInterruptHandler:PROC

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
    push rax
    push rcx
    push rdx
    push r8
    push r9
    push r10
    push r11
    sub rsp, 32
    lea rcx, MsgTrapInfo[rip]
    lea rdx, 1b[rip]
    mov r8, [rbp + KTRAP_FRAME_Rip]
    call qword ptr FrLdrDbgPrint[rip]
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdx
    pop rcx
    pop rax
    add rsp, 32
#endif
ENDM

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

    TRAPINFO KiDebugTrapOrFault

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

    lea rcx, MsgDoubleFault[rip]
    mov rdx, [rbp + KTRAP_FRAME_FaultAddress]
    mov r8, rbp
    call qword ptr FrLdrDbgPrint[rip]

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

    //TRAPINFO KiGeneralProtectionFault
    //mov rdx, [rbp + KTRAP_FRAME_Rip]
    //lea rcx, MsgGeneralProtFault[rip]
    //call qword ptr FrLdrDbgPrint[rip]

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

    /* Enable interrupts for the page fault handler */
    sti

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

    /* Disable interrupts again for the debugger */
    cli

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
    ExitTrap (TF_SAVE_ALL or TF_CHECKUSERAPC)
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

    TRAPINFO KiDebugServiceTrap

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

    /* Lower IRQL back to PASSIVE */
    mov rax, PASSIVE_LEVEL
    mov cr8, rax

    /* Return */
    ExitTrap (TF_VOLATILES or TF_IRQL)
.ENDP

EXTERN KiRetireDpcList:PROC
PUBLIC KiRetireDpcListInDpcStack
.PROC KiRetireDpcListInDpcStack
    push rbp
    .pushreg rbp
    mov rbp, rsp
    .setframe rbp, 0
    .endprolog

    /* Switch stack and call the function */
    mov rsp, rdx
    sub rsp, 40
    call KiRetireDpcList

    /* Restore stack, cleanup and return */
    mov rsp, rbp
    pop rbp
    ret
.ENDP

PUBLIC KiDpcInterrupt
.PROC KiDpcInterrupt
   /* No error code */
    EnterTrap (TF_VOLATILES or TF_IRQL)

    /* Call the worker routine */
    call KiDpcInterruptHandler

    /* Return, but don't send an EOI! */
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

PUBLIC KiInterruptDispatch
FUNC KiInterruptDispatch
    /* The error code is a pointer to the interrupt object's code */
    EnterTrap (TF_HAS_ERROR_CODE or TF_SAVE_ALL or TF_IRQL)

    /* Increase interrupt count */
    inc dword ptr gs:[PcInterruptCount];

    /* Load the address of the interrupt object into rcx */
    mov rcx, [rbp + KTRAP_FRAME_ErrorCode]

    /* Substract offset of the DispatchCode member plus 6 for the call instruction */
    sub rcx, KINTERRUPT_DispatchCode + 6

    /* Raise IRQL to SynchronizeIrql */
    movzx rax, byte ptr [rcx + KINTERRUPT_SynchronizeIrql]
    mov cr8, rax

#ifdef CONFIG_SMP
    /* Acquire interrupt lock */
    mov r8, [rcx + KINTERRUPT_ActualLock]

    //KxAcquireSpinLock(Interrupt->ActualLock);
#endif

    /* Call the ISR */
    mov rdx, [rcx + KINTERRUPT_ServiceContext]
    call qword ptr [rcx + KINTERRUPT_ServiceRoutine]

#ifdef CONFIG_SMP
    /* Release interrupt lock */
    //KxReleaseSpinLock(Interrupt->ActualLock);
#endif

    /* Go back to old irql */
    movzx rax, byte ptr [rbp + KTRAP_FRAME_PreviousIrql]
    mov cr8, rax

    /* Return */
    ExitTrap (TF_SAVE_ALL or TF_SEND_EOI)
ENDFUNC


#define MAX_SYSCALL_PARAM_SIZE (16 * 8)
#define HOME_SIZE 6*8
#define SYSCALL_ALLOCATION (MAX_SYSCALL_PARAM_SIZE + HOME_SIZE)

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

    /* Old stack pointer is in rcx, lie and say we saved it in rbp */
    .setframe rbp, 0
    .endprolog

    /* Swap gs to kernel, so we can access the PCR */
    swapgs

    /* Save the user mode rsp in the PCR */
    mov gs:[PcUserRsp], rsp

    /* Get the kernel stack from the PCR */
    mov rsp, gs:[PcRspBase]

    /* Allocate a TRAP_FRAME and space for parameters */
    sub rsp, (KTRAP_FRAME_LENGTH + MAX_SYSCALL_PARAM_SIZE + HOME_SIZE)
#if DBG
    /* Save rbp and load it with the old stack pointer */
    mov [rsp + HOME_SIZE + MAX_SYSCALL_PARAM_SIZE + HOME_SIZE + KTRAP_FRAME_Rbp], rbp
    mov rbp, gs:[PcUserRsp]
#endif

    /* Save important volatiles in the trap frame */
    mov [rsp + HOME_SIZE + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rax], rax
    mov [rsp + HOME_SIZE + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rcx], rcx
    mov [rsp + HOME_SIZE + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_R10], r10
    mov [rsp + HOME_SIZE + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_R11], r11

    /* Set sane segments */
    mov ax, (KGDT64_R3_DATA or RPL_MASK)
    mov ds, ax
    mov es, ax

    /* Call the C-handler (will enable interrupts) */
    lea rcx, [rsp + SYSCALL_ALLOCATION]
    call KiSystemCallHandler

    /* Deallocate the handlers home stack frame */
    add rsp, HOME_SIZE

    /* The return value is the address of the Nt-function */
    mov rcx, [rsp + 0]
    mov rdx, [rsp + 8]
    mov r8, [rsp + 16]
    mov r9, [rsp + 24]
    call rax

#if DBG
    /* Restore rbp */
    mov rbp, [rsp + SYSCALL_ALLOCATION + KTRAP_FRAME_Rbp]
#endif

    /* Disable interrupts for return */
    cli

    /* Restore old trap frame */
    mov rcx, gs:[PcCurrentThread]
    mov rdx, [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_TrapFrame]
    mov [rcx + KTHREAD_TrapFrame], rdx

    /* Prepare user mode return address (rcx) and eflags (r11) for sysret */
    mov rcx, [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rcx]
    mov r11, [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_R11]

    /* Load user mode stack (It was copied to the trap frame) */
    mov rsp, [rsp + MAX_SYSCALL_PARAM_SIZE + KTRAP_FRAME_Rsp]

    /* Swap gs back to user */
    swapgs

    /* return to user mode */
    .byte HEX(48) // REX prefix to return to long mode
    sysret
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
    mov rsi, [rsp + KTRAP_FRAME_Rdx]
    mov [r11 + KTHREAD_TrapFrame], rsi

    /* Restore rdi and rsi */
    mov rsi, [rbp + KTRAP_FRAME_Rsi]
    mov rdi, [rbp + KTRAP_FRAME_Rdi]

    /* Cleanup the stack and return */
    lea rsp, [rbp + KTRAP_FRAME_LENGTH]
    pop rbp
    ret

ENDFUNC


KiExitToUserApc:
    int 3

/*!
 * VOID
 * DECLSPEC_NORETURN
 * KiServiceExit(IN PKTRAP_FRAME TrapFrame, IN NTSTATUS Status));
 */
PUBLIC KiServiceExit
KiServiceExit:
    mov [rcx + KTRAP_FRAME_Rax], rdx
    mov rbp, rcx
    mov rsp, rcx

    /* Return */
    //ExitTrap TF_SAVE_ALL

/*!
 * VOID
 * DECLSPEC_NORETURN
 * KiServiceExit2(IN PKTRAP_FRAME TrapFrame);
 */
PUBLIC KiServiceExit2
.PROC KiServiceExit2
    .ENDPROLOG

    mov rbp, rcx
    mov rsp, rcx

    /* Return */
    ExitTrap TF_SAVE_ALL
.ENDP

PUBLIC KiInitializeSegments
KiInitializeSegments:
    mov ax, KGDT64_R3_DATA or RPL_MASK
    mov gs, ax
    swapgs
    mov gs, ax
    ret


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
