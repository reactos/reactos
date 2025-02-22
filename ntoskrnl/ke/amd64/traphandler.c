/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         x64 trap handlers
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

VOID
KiRetireDpcListInDpcStack(
    PKPRCB Prcb,
    PVOID DpcStack);

NTSTATUS
KiConvertToGuiThread(
    VOID);

_Requires_lock_not_held_(Prcb->PrcbLock)
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
        /* Acquire the PRCB lock */
        KiAcquirePrcbLock(Prcb);

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
KiNmiInterruptHandler(
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ PKEXCEPTION_FRAME ExceptionFrame)
{
    BOOLEAN ManualSwapGs = FALSE;

    /* Check if the NMI came from kernel mode */
    if ((TrapFrame->SegCs & MODE_MASK) == 0)
    {
        /* Check if GS base is already kernel mode. This is needed, because
           we might be interrupted during an interrupt/exception from user-mode
           before the swapgs instruction. */
        if ((LONG64)__readmsr(MSR_GS_BASE) >= 0)
        {
            /* Swap GS to kernel */
            __swapgs();
            ManualSwapGs = TRUE;
        }
    }

    /* Check if this is a freeze */
    if (KiProcessorFreezeHandler(TrapFrame, ExceptionFrame))
    {
        /* NMI was handled */
        goto Exit;
    }

    /* Handle the NMI */
    KiHandleNmi();

Exit:
    /* Check if we need to swap GS back */
    if (ManualSwapGs)
    {
        /* Swap GS back to user */
        __swapgs();
    }
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
    VOID)
{
    PKTRAP_FRAME TrapFrame;
    PKSERVICE_TABLE_DESCRIPTOR DescriptorTable;
    PKTHREAD Thread;
    PULONG64 KernelParams, UserParams;
    ULONG ServiceNumber, TableIndex, Count;
    ULONG64 UserRsp;

    /* Get a pointer to the trap frame */
    TrapFrame = (PKTRAP_FRAME)((PULONG64)_AddressOfReturnAddress() + 1 + MAX_SYSCALL_PARAMS);

    /* Increase system call count */
    __addgsdword(FIELD_OFFSET(KIPCR, Prcb.KeSystemCalls), 1);

    /* Get the current thread */
    Thread = KeGetCurrentThread();

    /* Set previous mode */
    Thread->PreviousMode = TrapFrame->PreviousMode = UserMode;

    /* We don't have an exception frame yet */
    TrapFrame->ExceptionFrame = 0;

    /* Get the user Stack pointer */
    UserRsp = TrapFrame->Rsp;

    /* Enable interrupts */
    _enable();

    /* If the usermode rsp was not a usermode address, prepare an exception */
    if (UserRsp > MmUserProbeAddress) UserRsp = MmUserProbeAddress;

    /* Get the address of the usermode and kernelmode parameters */
    UserParams = (PULONG64)UserRsp + 1;
    KernelParams = (PULONG64)TrapFrame - MAX_SYSCALL_PARAMS;

    /* Get the system call number from the trap frame and decode it */
    ServiceNumber = (ULONG)TrapFrame->Rax;
    TableIndex = (ServiceNumber >> TABLE_OFFSET_BITS) & ((1 << TABLE_NUMBER_BITS) - 1);
    ServiceNumber &= SERVICE_NUMBER_MASK;

    /* Check for win32k system calls */
    if (TableIndex == WIN32K_SERVICE_INDEX)
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
    DescriptorTable = &((PKSERVICE_TABLE_DESCRIPTOR)Thread->ServiceTable)[TableIndex];

    /* Validate the system call number */
    if (ServiceNumber >= DescriptorTable->Limit)
    {
        /* Check if this is a GUI call and this is not a GUI thread yet */
        if ((TableIndex == WIN32K_SERVICE_INDEX) &&
            (Thread->ServiceTable == KeServiceDescriptorTable))
        {
            /* Convert this thread to a GUI thread.
               It is invalid to change the stack in the middle of a C function,
               therefore we return KiConvertToGuiThread to the system call entry
               point, which then calls the asm function KiConvertToGuiThread,
               which allocates a new stack, switches to it, calls
               PsConvertToGuiThread and resumes in the middle of
               KiSystemCallEntry64 to restart the system call handling.
               If converting fails, the system call returns a failure code. */
            return (PVOID)KiConvertToGuiThread;
        }

        /* Fail the call */
        TrapFrame->Rax = STATUS_INVALID_SYSTEM_SERVICE;
        return (PVOID)NtSyscallFailure;
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
                ASSERT(FALSE);
                break;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
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

