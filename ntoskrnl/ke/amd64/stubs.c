/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         stubs
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <fltkernel.h>

#define NDEBUG
#include <debug.h>

ULONG ProcessCount;
BOOLEAN CcPfEnablePrefetcher;
SIZE_T KeXStateLength = sizeof(XSAVE_FORMAT);

VOID
KiRetireDpcListInDpcStack(
    PKPRCB Prcb,
    PVOID DpcStack);

NTSTATUS
KiConvertToGuiThread(
    VOID);

VOID
NTAPI
KiDpcInterruptHandler(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKTHREAD NewThread, OldThread;
    KIRQL OldIrql;

    /* Raise to DISPATCH_LEVEL */
    OldIrql = KfRaiseIrql(DISPATCH_LEVEL);

    /* Send an EOI */
    KiSendEOI();

    /* Check for pending timers, pending DPCs, or pending ready threads */
    if ((Prcb->DpcData[0].DpcQueueDepth) ||
        (Prcb->TimerRequest) ||
        (Prcb->DeferredReadyListHead.Next))
    {
        /* Retire DPCs while under the DPC stack */
        KiRetireDpcListInDpcStack(Prcb, Prcb->DpcStack);
    }

    /* Enable interrupts */
    _enable();

    /* Check for quantum end */
    if (Prcb->QuantumEnd)
    {
        /* Handle quantum end */
        Prcb->QuantumEnd = FALSE;
        KiQuantumEnd();
    }
    else if (Prcb->NextThread)
    {
        /* Capture current thread data */
        OldThread = Prcb->CurrentThread;
        NewThread = Prcb->NextThread;

        /* Set new thread data */
        Prcb->NextThread = NULL;
        Prcb->CurrentThread = NewThread;

        /* The thread is now running */
        NewThread->State = Running;
        OldThread->WaitReason = WrDispatchInt;

        /* Make the old thread ready */
        KxQueueReadyThread(OldThread, Prcb);

        /* Swap to the new thread */
        KiSwapContext(APC_LEVEL, OldThread);
    }

    /* Go back to old irql and disable interrupts */
    KeLowerIrql(OldIrql);
    _disable();
}


VOID
FASTCALL
KeZeroPages(IN PVOID Address,
            IN ULONG Size)
{
    /* Not using XMMI in this routine */
    RtlZeroMemory(Address, Size);
}

PVOID
KiSwitchKernelStackHelper(
    LONG_PTR StackOffset,
    PVOID OldStackBase);

/*
 * Kernel stack layout (example pointers):
 * 0xFFFFFC0F'2D008000 KTHREAD::StackBase
 *    [XSAVE_AREA size == KeXStateLength = 0x440]
 * 0xFFFFFC0F'2D007BC0 KTHREAD::StateSaveArea _XSAVE_FORMAT
 * 0xFFFFFC0F'2D007B90 KTHREAD::InitialStack
 *    [0x190 bytes KTRAP_FRAME]
 * 0xFFFFFC0F'2D007A00 KTHREAD::TrapFrame
 *    [KSTART_FRAME] or ...
 *    [KSWITCH_FRAME]
 * 0xFFFFFC0F'2D007230 KTHREAD::KernelStack
 */

PVOID
NTAPI
KiSwitchKernelStack(PVOID StackBase, PVOID StackLimit)
{
    PKTHREAD CurrentThread;
    PVOID OldStackBase;
    LONG_PTR StackOffset;
    SIZE_T StackSize;
    PKIPCR Pcr;

    /* Get the current thread */
    CurrentThread = KeGetCurrentThread();

    /* Save the old stack base */
    OldStackBase = CurrentThread->StackBase;

    /* Get the size of the current stack */
    StackSize = (ULONG_PTR)CurrentThread->StackBase - CurrentThread->StackLimit;
    ASSERT(StackSize <= (ULONG_PTR)StackBase - (ULONG_PTR)StackLimit);

    /* Copy the current stack contents to the new stack */
    RtlCopyMemory((PUCHAR)StackBase - StackSize,
                  (PVOID)CurrentThread->StackLimit,
                  StackSize);

    /* Calculate the offset between the old and the new stack */
    StackOffset = (PUCHAR)StackBase - (PUCHAR)CurrentThread->StackBase;

    /* Disable interrupts while messing with the stack */
    _disable();

    /* Set the new trap frame */
    CurrentThread->TrapFrame = (PKTRAP_FRAME)Add2Ptr(CurrentThread->TrapFrame,
                                                     StackOffset);

    /* Set the new initial stack */
    CurrentThread->InitialStack = Add2Ptr(CurrentThread->InitialStack,
                                          StackOffset);

    /* Set the new stack limits */
    CurrentThread->StackBase = StackBase;
    CurrentThread->StackLimit = (ULONG_PTR)StackLimit;
    CurrentThread->LargeStack = TRUE;

    /* Adjust RspBase in the PCR */
    Pcr = (PKIPCR)KeGetPcr();
    Pcr->Prcb.RspBase += StackOffset;

    /* Adjust Rsp0 in the TSS */
    Pcr->TssBase->Rsp0 += StackOffset;

    return OldStackBase;
}


NTSTATUS
NTAPI
KeUserModeCallback(
    IN ULONG RoutineIndex,
    IN PVOID Argument,
    IN ULONG ArgumentLength,
    OUT PVOID *Result,
    OUT PULONG ResultLength)
{
    ULONG_PTR OldStack;
    PULONG_PTR NewStack;
    PULONG_PTR UserStackPointer;
    NTSTATUS CallbackStatus;
#ifdef _M_IX86
    PEXCEPTION_REGISTRATION_RECORD ExceptionList;
#endif // _M_IX86
    PTEB Teb;
    ULONG GdiBatchCount = 0;
    ASSERT(KeGetCurrentThread()->ApcState.KernelApcInProgress == FALSE);
    ASSERT(KeGetPreviousMode() == UserMode);

    /* Get the current user-mode stack */
    UserStackPointer = KiGetUserModeStackAddress();
    OldStack = *UserStackPointer;

    /* Enter a SEH Block */
    _SEH2_TRY
    {
        /* Calculate and align the stack size */
        NewStack = (PULONG_PTR)ALIGN_DOWN_POINTER_BY(OldStack - ArgumentLength, sizeof(PVOID));

        /* Make sure it's writable */
        ProbeForWrite((PVOID)(NewStack - 6 * sizeof(ULONG_PTR)),
                      ArgumentLength + 6 * sizeof(ULONG_PTR),
                      sizeof(ULONG_PTR));

        /* Copy the buffer into the stack */
        RtlCopyMemory(NewStack, Argument, ArgumentLength);

        /* Write the arguments */
        NewStack -= 6;
        NewStack[0] = 0;
        NewStack[1] = RoutineIndex;
        NewStack[2] = (ULONG_PTR)(NewStack + 6);
        NewStack[3] = ArgumentLength;

        /* Save the exception list */
        Teb = KeGetCurrentThread()->Teb;
#ifdef _M_IX86
        ExceptionList = Teb->NtTib.ExceptionList;
#endif // _M_IX86

        /* Jump to user mode */
        *UserStackPointer = (ULONG_PTR)NewStack;
        CallbackStatus = KiCallUserMode(Result, ResultLength);
        if (CallbackStatus != STATUS_CALLBACK_POP_STACK)
        {
#ifdef _M_IX86
            /* Only restore the exception list if we didn't crash in ring 3 */
            Teb->NtTib.ExceptionList = ExceptionList;
#endif // _M_IX86
        }
        else
        {
            /* Otherwise, pop the stack */
            OldStack = *UserStackPointer;
        }

        /* Read the GDI Batch count */
        GdiBatchCount = Teb->GdiBatchCount;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Get the SEH exception */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Check if we have GDI Batch operations */
    if (GdiBatchCount)
    {
        *UserStackPointer -= 256;
        KeGdiFlushUserBatch();
    }

    /* Restore stack and return */
    *UserStackPointer = OldStack;
    return CallbackStatus;
}

VOID
FASTCALL
KiIdleLoop(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKTHREAD OldThread, NewThread;

    /* Now loop forever */
    while (TRUE)
    {
        /* Start of the idle loop: disable interrupts */
        _enable();
        YieldProcessor();
        YieldProcessor();
        _disable();

        /* Check for pending timers, pending DPCs, or pending ready threads */
        if ((Prcb->DpcData[0].DpcQueueDepth) ||
            (Prcb->TimerRequest) ||
            (Prcb->DeferredReadyListHead.Next))
        {
            /* Quiesce the DPC software interrupt */
            HalClearSoftwareInterrupt(DISPATCH_LEVEL);

            /* Handle it */
            KiRetireDpcList(Prcb);
        }

        /* Check if a new thread is scheduled for execution */
        if (Prcb->NextThread)
        {
            /* Enable interrupts */
            _enable();

            /* Capture current thread data */
            OldThread = Prcb->CurrentThread;
            NewThread = Prcb->NextThread;

            /* Set new thread data */
            Prcb->NextThread = NULL;
            Prcb->CurrentThread = NewThread;

            /* The thread is now running */
            NewThread->State = Running;

            /* Do the swap at SYNCH_LEVEL */
            KfRaiseIrql(SYNCH_LEVEL);

            /* Switch away from the idle thread */
            KiSwapContext(APC_LEVEL, OldThread);

            /* Go back to DISPATCH_LEVEL */
            KeLowerIrql(DISPATCH_LEVEL);
        }
        else
        {
            /* Continue staying idle. Note the HAL returns with interrupts on */
            Prcb->PowerState.IdleFunction(&Prcb->PowerState);
        }
    }
}


/*! \name KiInitializeUserApc
 *
 *  \brief
 *      Prepares the current trap frame (which must have come from user mode)
 *      with the ntdll.KiUserApcDispatcher entrypoint, copying a CONTEXT
 *      record with the context from the old trap frame to the threads user
 *      mode stack.
 *
 *  \param ExceptionFrame
 *  \param TrapFrame
 *  \param NormalRoutine
 *  \param NormalContext
 *  \param SystemArgument1
 *  \param SystemArgument2
 *
 *  \remarks
 *      This function is called from KiDeliverApc, when the trap frame came
 *      from user mode. This happens before a systemcall or interrupt exits back
 *      to usermode or when a thread is started from PspUserThreadstartup.
 *      The trap exit code will then leave to KiUserApcDispatcher which in turn
 *      calls the NormalRoutine, passing NormalContext, SystemArgument1 and
 *      SystemArgument2 as parameters. When that function returns, it calls
 *      NtContinue to return back to the kernel, where the old context that was
 *      saved on the usermode stack is restored and execution is transferred
 *      back to usermode, where the original trap originated from.
 *
 *--*/
VOID
NTAPI
KiInitializeUserApc(IN PKEXCEPTION_FRAME ExceptionFrame,
                    IN PKTRAP_FRAME TrapFrame,
                    IN PKNORMAL_ROUTINE NormalRoutine,
                    IN PVOID NormalContext,
                    IN PVOID SystemArgument1,
                    IN PVOID SystemArgument2)
{
    CONTEXT Context;
    ULONG64 AlignedRsp, Stack;
    EXCEPTION_RECORD SehExceptRecord;

    /* Sanity check, that the trap frame is from user mode */
    ASSERT((TrapFrame->SegCs & MODE_MASK) != KernelMode);

    /* Convert the current trap frame to a context */
    Context.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    KeTrapFrameToContext(TrapFrame, ExceptionFrame, &Context);

    /* We jump to KiUserApcDispatcher in ntdll */
    TrapFrame->Rip = (ULONG64)KeUserApcDispatcher;

    /* Setup Ring 3 segments */
    TrapFrame->SegCs = KGDT64_R3_CODE | RPL_MASK;
    TrapFrame->SegDs = KGDT64_R3_DATA | RPL_MASK;
    TrapFrame->SegEs = KGDT64_R3_DATA | RPL_MASK;
    TrapFrame->SegFs = KGDT64_R3_CMTEB | RPL_MASK;
    TrapFrame->SegGs = KGDT64_R3_DATA | RPL_MASK;
    TrapFrame->SegSs = KGDT64_R3_DATA | RPL_MASK;

    /* Sanitize EFLAGS, enable interrupts */
    TrapFrame->EFlags = (Context.EFlags & EFLAGS_USER_SANITIZE);
    TrapFrame->EFlags |= EFLAGS_INTERRUPT_MASK;

    /* Set parameters for KiUserApcDispatcher */
    Context.P1Home = (ULONG64)NormalContext;
    Context.P2Home = (ULONG64)SystemArgument1;
    Context.P3Home = (ULONG64)SystemArgument2;
    Context.P4Home = (ULONG64)NormalRoutine;

    /* Check if thread has IOPL and force it enabled if so */
    //if (KeGetCurrentThread()->Iopl) TrapFrame->EFlags |= EFLAGS_IOPL;

    /* Align Stack to 16 bytes and allocate space */
    AlignedRsp = Context.Rsp & ~15;
    Stack = AlignedRsp - sizeof(CONTEXT);
    TrapFrame->Rsp = Stack;

    /* The stack must be 16 byte aligned for KiUserApcDispatcher */
    ASSERT((Stack & 15) == 0);

    /* Protect with SEH */
    _SEH2_TRY
    {
         /* Probe the stack */
        ProbeForWrite((PCONTEXT)Stack,  sizeof(CONTEXT), 8);

        /* Copy the context */
        RtlCopyMemory((PCONTEXT)Stack, &Context, sizeof(CONTEXT));
    }
    _SEH2_EXCEPT((RtlCopyMemory(&SehExceptRecord, _SEH2_GetExceptionInformation()->ExceptionRecord, sizeof(EXCEPTION_RECORD)), EXCEPTION_EXECUTE_HANDLER))
    {
        /* Dispatch the exception */
        SehExceptRecord.ExceptionAddress = (PVOID)TrapFrame->Rip;
        KiDispatchException(&SehExceptRecord,
                            ExceptionFrame,
                            TrapFrame,
                            UserMode,
                            TRUE);
    }
    _SEH2_END;
}

VOID
NTAPI
KiSwapProcess(IN PKPROCESS NewProcess,
              IN PKPROCESS OldProcess)
{
    PKIPCR Pcr = (PKIPCR)KeGetPcr();

#ifdef CONFIG_SMP
    /* Update active processor mask */
    InterlockedXor64((PLONG64)&NewProcess->ActiveProcessors, Pcr->Prcb.SetMember);
    InterlockedXor64((PLONG64)&OldProcess->ActiveProcessors, Pcr->Prcb.SetMember);
#endif

    /* Update CR3 */
    __writecr3(NewProcess->DirectoryTableBase[0]);

    /* Update IOPM offset */
    Pcr->TssBase->IoMapBase = NewProcess->IopmOffset;
}

#define MAX_SYSCALL_PARAMS 16

NTSTATUS
NtSyscallFailure(void)
{
    /* This is the failure function */
    return (NTSTATUS)KeGetCurrentThread()->TrapFrame->Rax;
}

PVOID
KiSystemCallHandler(
    _In_ ULONG64 ReturnAddress,
    _In_ ULONG64 P2,
    _In_ ULONG64 P3,
    _In_ ULONG64 P4)
{
    PKTRAP_FRAME TrapFrame;
    PKSERVICE_TABLE_DESCRIPTOR DescriptorTable;
    PKTHREAD Thread;
    PULONG64 KernelParams, UserParams;
    ULONG ServiceNumber, Offset, Count;
    ULONG64 UserRsp;

    /* Get a pointer to the trap frame */
    TrapFrame = (PKTRAP_FRAME)((PULONG64)_AddressOfReturnAddress() + 1 + MAX_SYSCALL_PARAMS);

    /* Save some values in the trap frame */
    TrapFrame->Rip = ReturnAddress;
    TrapFrame->Rdx = P2;
    TrapFrame->R8 = P3;
    TrapFrame->R9 = P4;

    /* Increase system call count */
    __addgsdword(FIELD_OFFSET(KIPCR, Prcb.KeSystemCalls), 1);

    /* Get the current thread */
    Thread = KeGetCurrentThread();

    /* Set previous mode */
    Thread->PreviousMode = TrapFrame->PreviousMode = UserMode;

    /* Save the old trap frame and set the new */
    TrapFrame->TrapFrame = (ULONG64)Thread->TrapFrame;
    Thread->TrapFrame = TrapFrame;

    /* Before enabling interrupts get the user rsp from the KPCR */
    UserRsp = __readgsqword(FIELD_OFFSET(KIPCR, UserRsp));
    TrapFrame->Rsp = UserRsp;

    /* Enable interrupts */
    _enable();

    /* If the usermode rsp was not a usermode address, prepare an exception */
    if (UserRsp > MmUserProbeAddress) UserRsp = MmUserProbeAddress;

    /* Get the address of the usermode and kernelmode parameters */
    UserParams = (PULONG64)UserRsp + 1;
    KernelParams = (PULONG64)TrapFrame - MAX_SYSCALL_PARAMS;

    /* Get the system call number from the trap frame and decode it */
    ServiceNumber = (ULONG)TrapFrame->Rax;
    Offset = (ServiceNumber >> SERVICE_TABLE_SHIFT) & SERVICE_TABLE_MASK;
    ServiceNumber &= SERVICE_NUMBER_MASK;

    /* Get descriptor table */
    DescriptorTable = (PVOID)((ULONG_PTR)Thread->ServiceTable + Offset);

    /* Validate the system call number */
    if (ServiceNumber >= DescriptorTable->Limit)
    {
        /* Check if this is a GUI call */
        if (!(Offset & SERVICE_TABLE_TEST))
        {
            /* Fail the call */
            TrapFrame->Rax = STATUS_INVALID_SYSTEM_SERVICE;
            return (PVOID)NtSyscallFailure;
        }

        /* Convert us to a GUI thread 
           To be entirely correct. we return KiConvertToGuiThread,
           which allocates a new stack, switches to it, calls
           PsConvertToGuiThread and resumes in the middle of
           KiSystemCallEntry64 to restart the system call handling. */
        return (PVOID)KiConvertToGuiThread;
    }

    /* Get stack bytes and calculate argument count */
    Count = DescriptorTable->Number[ServiceNumber] / 8;

    __try
    {
        switch (Count)
        {
            case 16: KernelParams[15] = UserParams[15];
            case 15: KernelParams[14] = UserParams[14];
            case 14: KernelParams[13] = UserParams[13];
            case 13: KernelParams[12] = UserParams[12];
            case 12: KernelParams[11] = UserParams[11];
            case 11: KernelParams[10] = UserParams[10];
            case 10: KernelParams[9] = UserParams[9];
            case 9: KernelParams[8] = UserParams[8];
            case 8: KernelParams[7] = UserParams[7];
            case 7: KernelParams[6] = UserParams[6];
            case 6: KernelParams[5] = UserParams[5];
            case 5: KernelParams[4] = UserParams[4];
            case 4:
            case 3:
            case 2:
            case 1:
            case 0:
                break;

            default:
                __debugbreak();
                break;
        }
    }
    __except(1)
    {
        TrapFrame->Rax = _SEH2_GetExceptionCode();
        return (PVOID)NtSyscallFailure;
    }


    return (PVOID)DescriptorTable->Base[ServiceNumber];
}


// FIXME: we need to
VOID
KiSystemService(IN PKTHREAD Thread,
                IN PKTRAP_FRAME TrapFrame,
                IN ULONG Instruction)
{
    UNIMPLEMENTED;
    __debugbreak();
}

NTSYSAPI
NTSTATUS
NTAPI
NtCallbackReturn
( IN PVOID Result OPTIONAL, IN ULONG ResultLength, IN NTSTATUS Status )
{
    UNIMPLEMENTED;
    __debugbreak();
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
NtSetLdtEntries
(ULONG Selector1, LDT_ENTRY LdtEntry1, ULONG Selector2, LDT_ENTRY LdtEntry2)
{
    UNIMPLEMENTED;
    __debugbreak();
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
NtVdmControl(IN ULONG ControlCode,
             IN PVOID ControlData)
{
    /* Not supported */
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
KiCallUserMode(
    IN PVOID *OutputBuffer,
    IN PULONG OutputLength)
{
    UNIMPLEMENTED;
    __debugbreak();
    return STATUS_UNSUCCESSFUL;
}

ULONG ProcessCount;
BOOLEAN CcPfEnablePrefetcher;


