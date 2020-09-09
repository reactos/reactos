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

    /* Disable interrupts and go back to old irql */
    _disable();
    KeLowerIrql(OldIrql);
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

DECLSPEC_NORETURN
VOID
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

    /* We don't have an exception frame yet */
    TrapFrame->ExceptionFrame = 0;

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

    /* Check for win32k system calls */
    if (Offset & SERVICE_TABLE_TEST)
    {
        ULONG GdiBatchCount;

        /* Read the GDI batch count from the TEB */
        _SEH2_TRY
        {
            GdiBatchCount = NtCurrentTeb()->GdiBatchCount;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            GdiBatchCount = 0;
        }
        _SEH2_END;

        /* Flush batch, if there are entries */
        if (GdiBatchCount != 0)
        {
            KeGdiFlushUserBatch();
        }
    }

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

    _SEH2_TRY
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
    _SEH2_EXCEPT(1)
    {
        TrapFrame->Rax = _SEH2_GetExceptionCode();
        return (PVOID)NtSyscallFailure;
    }
    _SEH2_END;

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


