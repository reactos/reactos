/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/kernel.c
 * PURPOSE:         Initializes the kernel
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>
#include <internal/napi.h>

/* GLOBALS *******************************************************************/

PKPRCB KiProcessorBlock[MAXIMUM_PROCESSORS];
KNODE KiNode0;
PKNODE KeNodeBlock[1];
UCHAR KeNumberNodes = 1;
UCHAR KeProcessNodeSeed;
ETHREAD KiInitialThread;
EPROCESS KiInitialProcess;

extern LIST_ENTRY KiProcessListHead;
extern ULONG Ke386GlobalPagesEnabled;
extern KGDTENTRY KiBootGdt[];
extern PVOID trap_stack, init_stack;
extern KTSS KiBootTss;

/* System-defined Spinlocks */
KSPIN_LOCK KiDispatcherLock;
KSPIN_LOCK MmPfnLock;
KSPIN_LOCK MmSystemSpaceLock;
KSPIN_LOCK CcBcbSpinLock;
KSPIN_LOCK CcMasterSpinLock;
KSPIN_LOCK CcVacbSpinLock;
KSPIN_LOCK CcWorkQueueSpinLock;
KSPIN_LOCK NonPagedPoolLock;
KSPIN_LOCK MmNonPagedPoolLock;
KSPIN_LOCK IopCancelSpinLock;
KSPIN_LOCK IopVpbSpinLock;
KSPIN_LOCK IopDatabaseLock;
KSPIN_LOCK IopCompletionLock;
KSPIN_LOCK NtfsStructLock;
KSPIN_LOCK AfdWorkQueueSpinLock;
KSPIN_LOCK KiTimerTableLock[16];
KSPIN_LOCK KiReverseStallIpiLock;

KSPIN_LOCK KiFreezeExecutionLock;
KSPIN_LOCK Ki486CompatibilityLock;

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, KeInit1)
#pragma alloc_text(INIT, KeInit2)
#endif

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KiInitSystem(VOID)
{
    ULONG i;

    /* Initialize Bugcheck Callback data */
    InitializeListHead(&BugcheckCallbackListHead);
    InitializeListHead(&BugcheckReasonCallbackListHead);
    KeInitializeSpinLock(&BugCheckCallbackLock);

    /* Initialize the Timer Expiration DPC */
    KeInitializeDpc(&KiExpireTimerDpc, KiExpireTimers, NULL);
    KeSetTargetProcessorDpc(&KiExpireTimerDpc, 0);

    /* Initialize Profiling data */
    KeInitializeSpinLock(&KiProfileLock);
    InitializeListHead(&KiProfileListHead);
    InitializeListHead(&KiProfileSourceListHead);

    /* Loop the timer table */
    for (i = 0; i < TIMER_TABLE_SIZE; i++)
    {
        /* Initialize the list and entries */
        InitializeListHead(&KiTimerTableListHead[i].Entry);
        KiTimerTableListHead[i].Time.HighPart = 0xFFFFFFFF;
        KiTimerTableListHead[i].Time.LowPart = 0;
    }

    /* Initialize old-style list */
    InitializeListHead(&KiTimerListHead);

    /* Initialize the Swap event and all swap lists */
    KeInitializeEvent(&KiSwapEvent, SynchronizationEvent, FALSE);
    InitializeListHead(&KiProcessInSwapListHead);
    InitializeListHead(&KiProcessOutSwapListHead);
    InitializeListHead(&KiStackInSwapListHead);

    /* Initialize the mutex for generic DPC calls */
    KeInitializeMutex(&KiGenericCallDpcMutex, 0);

    /* Initialize the syscall table */
    KeServiceDescriptorTable[0].Base = MainSSDT;
    KeServiceDescriptorTable[0].Count = NULL;
    KeServiceDescriptorTable[0].Limit = NUMBER_OF_SYSCALLS;
    KeServiceDescriptorTable[1].Limit = 0;
    KeServiceDescriptorTable[0].Number = MainSSPT;

    /* Copy the the current table into the shadow table for win32k */
    RtlCopyMemory(KeServiceDescriptorTableShadow,
                  KeServiceDescriptorTable,
                  sizeof(KeServiceDescriptorTable));
}

LARGE_INTEGER
NTAPI
KiComputeReciprocal(IN LONG Divisor,
                    OUT PUCHAR Shift)
{
    LARGE_INTEGER Reciprocal = {{0}};
    LONG BitCount = 0, Remainder = 1;

    /* Start by calculating the remainder */
    while (Reciprocal.HighPart >= 0)
    {
        /* Increase the loop (bit) count */
        BitCount++;

        /* Calculate the current fraction */
        Reciprocal.HighPart = (Reciprocal.HighPart << 1) |
                              (Reciprocal.LowPart >> 31);
        Reciprocal.LowPart <<= 1;

        /* Double the remainder and see if we went past the divisor */
        Remainder <<= 1;
        if (Remainder >= Divisor)
        {
            /* Set the low-bit and calculate the new remainder */
            Remainder -= Divisor;
            Reciprocal.LowPart |= 1;
        }
    }

    /* Check if we have a remainder */
    if (Remainder)
    {
        /* Check if the current fraction value is too large */
        if ((Reciprocal.LowPart == 0xFFFFFFFF) &&
            (Reciprocal.HighPart == 0xFFFFFFFF))
        {
            /* Set the high bit and reduce the bit count */
            Reciprocal.LowPart = 0;
            Reciprocal.HighPart = 0x80000000;
            BitCount--;
        }
        else
        {
            /* Check if only the lowest bits got too large */
            if (Reciprocal.LowPart == 0xFFFFFFFF)
            {
                /* Reset them and increase the high bits instead */
                Reciprocal.LowPart = 0;
                Reciprocal.HighPart++;
            }
            else
            {
                /* All is well, increase the low bits */
                Reciprocal.LowPart++;
            }
        }
    }

    /* Now calculate the actual shift and return the reciprocal */
    *Shift = (UCHAR)BitCount - 64;
    return Reciprocal;
}

VOID
NTAPI
KiInitSpinLocks(IN PKPRCB Prcb,
                IN CCHAR Number)
{
    ULONG i;

    /* Initialize Dispatcher Fields */
    Prcb->QueueIndex = 1;
    Prcb->ReadySummary = 0;
    Prcb->DeferredReadyListHead.Next = NULL;
    for (i = 0; i < 32; i++)
    {
        /* Initialize the ready list */
        InitializeListHead(&Prcb->DispatcherReadyListHead[i]);
    }

    /* Initialize DPC Fields */
    InitializeListHead(&Prcb->DpcData[DPC_NORMAL].DpcListHead);
    KeInitializeSpinLock(&Prcb->DpcData[DPC_NORMAL].DpcLock);
    Prcb->DpcData[DPC_NORMAL].DpcQueueDepth = 0;
    Prcb->DpcData[DPC_NORMAL].DpcCount = 0;
    Prcb->DpcRoutineActive = FALSE;
    Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
    Prcb->MinimumDpcRate = KiMinimumDpcRate;
    Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;
    KeInitializeDpc(&Prcb->CallDpc, NULL, NULL);
    KeSetTargetProcessorDpc(&Prcb->CallDpc, Number);
    KeSetImportanceDpc(&Prcb->CallDpc, HighImportance);

    /* Initialize the Wait List Head */
    InitializeListHead(&Prcb->WaitListHead);

    /* Initialize Queued Spinlocks */
    Prcb->LockQueue[LockQueueDispatcherLock].Next = NULL;
    Prcb->LockQueue[LockQueueDispatcherLock].Lock = &KiDispatcherLock;
    Prcb->LockQueue[LockQueueExpansionLock].Next = NULL;
    Prcb->LockQueue[LockQueueExpansionLock].Lock = NULL;
    Prcb->LockQueue[LockQueuePfnLock].Next = NULL;
    Prcb->LockQueue[LockQueuePfnLock].Lock = &MmPfnLock;
    Prcb->LockQueue[LockQueueSystemSpaceLock].Next = NULL;
    Prcb->LockQueue[LockQueueSystemSpaceLock].Lock = &MmSystemSpaceLock;
    Prcb->LockQueue[LockQueueBcbLock].Next = NULL;
    Prcb->LockQueue[LockQueueBcbLock].Lock = &CcBcbSpinLock;
    Prcb->LockQueue[LockQueueMasterLock].Next = NULL;
    Prcb->LockQueue[LockQueueMasterLock].Lock = &CcMasterSpinLock;
    Prcb->LockQueue[LockQueueVacbLock].Next = NULL;
    Prcb->LockQueue[LockQueueVacbLock].Lock = &CcVacbSpinLock;
    Prcb->LockQueue[LockQueueWorkQueueLock].Next = NULL;
    Prcb->LockQueue[LockQueueWorkQueueLock].Lock = &CcWorkQueueSpinLock;
    Prcb->LockQueue[LockQueueNonPagedPoolLock].Next = NULL;
    Prcb->LockQueue[LockQueueNonPagedPoolLock].Lock = &NonPagedPoolLock;
    Prcb->LockQueue[LockQueueMmNonPagedPoolLock].Next = NULL;
    Prcb->LockQueue[LockQueueMmNonPagedPoolLock].Lock = &MmNonPagedPoolLock;
    Prcb->LockQueue[LockQueueIoCancelLock].Next = NULL;
    Prcb->LockQueue[LockQueueIoCancelLock].Lock = &IopCancelSpinLock;
    Prcb->LockQueue[LockQueueIoVpbLock].Next = NULL;
    Prcb->LockQueue[LockQueueIoVpbLock].Lock = &IopVpbSpinLock;
    Prcb->LockQueue[LockQueueIoDatabaseLock].Next = NULL;
    Prcb->LockQueue[LockQueueIoDatabaseLock].Lock = &IopDatabaseLock;
    Prcb->LockQueue[LockQueueIoCompletionLock].Next = NULL;
    Prcb->LockQueue[LockQueueIoCompletionLock].Lock = &IopCompletionLock;
    Prcb->LockQueue[LockQueueNtfsStructLock].Next = NULL;
    Prcb->LockQueue[LockQueueNtfsStructLock].Lock = &NtfsStructLock;
    Prcb->LockQueue[LockQueueAfdWorkQueueLock].Next = NULL;
    Prcb->LockQueue[LockQueueAfdWorkQueueLock].Lock = &AfdWorkQueueSpinLock;
    Prcb->LockQueue[LockQueueUnusedSpare16].Next = NULL;
    Prcb->LockQueue[LockQueueUnusedSpare16].Lock = NULL;

    /* Loop timer locks */
    for (i = 0; i < LOCK_QUEUE_TIMER_TABLE_LOCKS; i++)
    {
        /* Initialize the lock and setup the Queued Spinlock */
        KeInitializeSpinLock(&KiTimerTableLock[i]);
        Prcb->LockQueue[i].Next = NULL;
        Prcb->LockQueue[i].Lock = &KiTimerTableLock[i];
    }

    /* Check if this is the boot CPU */
    if (!Number)
    {
        /* Initialize the lock themselves */
        KeInitializeSpinLock(&KiDispatcherLock);
        KeInitializeSpinLock(&KiReverseStallIpiLock);
        KeInitializeSpinLock(&MmPfnLock);
        KeInitializeSpinLock(&MmSystemSpaceLock);
        KeInitializeSpinLock(&CcBcbSpinLock);
        KeInitializeSpinLock(&CcMasterSpinLock);
        KeInitializeSpinLock(&CcVacbSpinLock);
        KeInitializeSpinLock(&CcWorkQueueSpinLock);
        KeInitializeSpinLock(&IopCancelSpinLock);
        KeInitializeSpinLock(&IopCompletionLock);
        KeInitializeSpinLock(&IopDatabaseLock);
        KeInitializeSpinLock(&IopVpbSpinLock);
        KeInitializeSpinLock(&NonPagedPoolLock);
        KeInitializeSpinLock(&MmNonPagedPoolLock);
        KeInitializeSpinLock(&NtfsStructLock);
        KeInitializeSpinLock(&AfdWorkQueueSpinLock);
        KeInitializeDispatcher(); // ROS OLD DISPATCHER
    }
}

VOID
NTAPI
KiInitializePcr(IN ULONG ProcessorNumber,
                IN PKIPCR Pcr,
                IN PKIDTENTRY Idt,
                IN PKGDTENTRY Gdt,
                IN PKTSS Tss,
                IN PKTHREAD IdleThread,
                IN PVOID DpcStack)
{
    /* Setup the TIB */
    Pcr->NtTib.ExceptionList = EXCEPTION_CHAIN_END;
    Pcr->NtTib.StackBase = 0;
    Pcr->NtTib.StackLimit = 0;
    Pcr->NtTib.Self = 0;

    /* Set the Current Thread */
    //Pcr->PrcbData.CurrentThread = IdleThread;

    /* Set pointers to ourselves */
    Pcr->Self = (PKPCR)Pcr;
    Pcr->Prcb = &Pcr->PrcbData;

    /* Set the PCR Version */
    Pcr->MajorVersion = PCR_MAJOR_VERSION;
    Pcr->MinorVersion = PCR_MINOR_VERSION;

    /* Set the PCRB Version */
    Pcr->PrcbData.MajorVersion = 1;
    Pcr->PrcbData.MinorVersion = 1;

    /* Set the Build Type */
    Pcr->PrcbData.BuildType = 0;

    /* Set the Processor Number and current Processor Mask */
    Pcr->PrcbData.Number = (UCHAR)ProcessorNumber;
    Pcr->PrcbData.SetMember = 1 << ProcessorNumber;

    /* Set the PRCB for this Processor */
    KiProcessorBlock[ProcessorNumber] = Pcr->Prcb;

    /* Start us out at PASSIVE_LEVEL */
    Pcr->Irql = PASSIVE_LEVEL;

    /* Set the GDI, IDT, TSS and DPC Stack */
    Pcr->GDT = (PVOID)Gdt;
    Pcr->IDT = Idt;
    Pcr->TSS = Tss;
    Pcr->PrcbData.DpcStack = DpcStack;
}

VOID
NTAPI
KiInitializeKernel(IN PKPROCESS InitProcess,
                   IN PKTHREAD InitThread,
                   IN PVOID IdleStack,
                   IN PKPRCB Prcb,
                   IN CCHAR Number,
                   IN PROS_LOADER_PARAMETER_BLOCK LoaderBlock)
{
    BOOLEAN NpxPresent;
    ULONG FeatureBits;
    LARGE_INTEGER PageDirectory;
    PVOID DpcStack;

    /* Detect and set the CPU Type */
    KiSetProcessorType();

    /* Set CR0 features based on detected CPU */
    KiSetCR0Bits();

    /* Check if an FPU is present */
    NpxPresent = KiIsNpxPresent();

    /* Initialize the Power Management Support for this PRCB */
    PoInitializePrcb(Prcb);

    /* Bugcheck if this is a 386 CPU */
    if (Prcb->CpuType == 3) KeBugCheckEx(0x5D, 0x386, 0, 0, 0);

    /* Get the processor features for the CPU */
    FeatureBits = KiGetFeatureBits();

    /* Save feature bits */
    Prcb->FeatureBits = FeatureBits;

    /* Get cache line information for this CPU */
    KiGetCacheInformation();

    /* Initialize spinlocks and DPC data */
    KiInitSpinLocks(Prcb, Number);

    /* Check if this is the Boot CPU */
    if (!Number)
    {
        /* Set Node Data */
        KeNodeBlock[0] = &KiNode0;
        Prcb->ParentNode = KeNodeBlock[0];
        KeNodeBlock[0]->ProcessorMask = Prcb->SetMember;

        /* Set boot-level flags */
        KeI386NpxPresent = NpxPresent;
        KeI386CpuType = Prcb->CpuType;
        KeI386CpuStep = Prcb->CpuStep;
        KeProcessorArchitecture = 0;
        KeProcessorLevel = (USHORT)Prcb->CpuType;
        if (Prcb->CpuID) KeProcessorRevision = Prcb->CpuStep;
        KeFeatureBits = FeatureBits;
        KeI386FxsrPresent = (KeFeatureBits & KF_FXSR) ? TRUE : FALSE;
        KeI386XMMIPresent = (KeFeatureBits & KF_XMMI) ? TRUE : FALSE;

        /* Set the current MP Master KPRCB to the Boot PRCB */
        Prcb->MultiThreadSetMaster = Prcb;

        /* Initialize some spinlocks */
        KeInitializeSpinLock(&KiFreezeExecutionLock);
        KeInitializeSpinLock(&Ki486CompatibilityLock);

        /* Initialize portable parts of the OS */
        KiInitSystem();

        /* Initialize the Idle Process and the Process Listhead */
        InitializeListHead(&KiProcessListHead);
        PageDirectory.QuadPart = 0;
        KeInitializeProcess(InitProcess,
                            0,
                            0xFFFFFFFF,
                            PageDirectory);
        InitProcess->QuantumReset = MAXCHAR;
    }
    else
    {
        /* FIXME */
        DPRINT1("SMP Boot support not yet present\n");
    }

#if 0
    /* Setup the Idle Thread */
    KeInitializeThread(InitProcess,
                       InitThread,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       IdleStack);
#endif
    InitThread->NextProcessor = Number;
    InitThread->Priority = HIGH_PRIORITY;
    InitThread->State = Running;
    InitThread->Affinity = 1 << Number;
    InitThread->WaitIrql = DISPATCH_LEVEL;
    InitProcess->ActiveProcessors = 1 << Number;

    /* Set up the thread-related fields in the PRCB */
    //Prcb->CurrentThread = InitThread;
    Prcb->NextThread = NULL;
    //Prcb->IdleThread = InitThread;

    /* Initialize the Debugger */
    KdInitSystem (0, &KeLoaderBlock);

    /* Initialize the Kernel Executive */
    ExpInitializeExecutive();

    /* Only do this on the boot CPU */
    if (!Number)
    {
        /* Calculate the time reciprocal */
        KiTimeIncrementReciprocal =
            KiComputeReciprocal(KeMaximumIncrement,
                                &KiTimeIncrementShiftCount);

        /* Update DPC Values in case they got updated by the executive */
        Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
        Prcb->MinimumDpcRate = KiMinimumDpcRate;
        Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;

        /* Allocate the DPC Stack */
        DpcStack = MmCreateKernelStack(FALSE);
        if (!DpcStack) KeBugCheckEx(NO_PAGES_AVAILABLE, 1, 0, 0, 0);
        Prcb->DpcStack = DpcStack;

        /* Allocate the IOPM save area. */
        Ki386IopmSaveArea = ExAllocatePoolWithTag(PagedPool,
                                                  PAGE_SIZE * 2,
                                                  TAG('K', 'e', ' ', ' '));
        if (!Ki386IopmSaveArea)
        {
            /* Bugcheck. We need this for V86/VDM support. */
            KeBugCheckEx(NO_PAGES_AVAILABLE, 2, PAGE_SIZE * 2, 0, 0);
        }
    }

    /* Free Initial Memory */
    MiFreeInitMemory();

    while (1)
    {
        LARGE_INTEGER Timeout;
        Timeout.QuadPart = 0x7fffffffffffffffLL;
        KeDelayExecutionThread(KernelMode, FALSE, &Timeout);
    }

    /* Bug Check and loop forever if anything failed */
    KEBUGCHECK(0);
    for(;;);
}

VOID
NTAPI
KiSystemStartup(IN PROS_LOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Currently hacked for CPU 0 only */
    ULONG Cpu = 0;
    PKIPCR Pcr = (PKIPCR)KPCR_BASE;
    PKPRCB Prcb;

    /* Initialize the PCR */
    RtlZeroMemory(Pcr, PAGE_SIZE);
    KiInitializePcr(Cpu,
                    Pcr,
                    KiIdt,
                    KiBootGdt,
                    &KiBootTss,
                    &KiInitialThread.Tcb,
                    trap_stack);
    Prcb = Pcr->Prcb;

    /* Set us as the current process */
    KiInitialThread.Tcb.ApcState.Process = &KiInitialProcess.Pcb;

    /* Clear DR6/7 to cleanup bootloader debugging */
    Pcr->PrcbData.ProcessorState.SpecialRegisters.KernelDr6 = 0;
    Pcr->PrcbData.ProcessorState.SpecialRegisters.KernelDr7 = 0;

    /*
     * Low-level GDT, TSS and LDT Setup, most of which Freeldr should have done
     * instead, and we should only add some extra information. This would be
     * required for future NTLDR compatibility.
     */
    KiInitializeGdt(NULL);
    Ki386BootInitializeTSS();
    Ki386InitializeLdt();

    /* Setup CPU-related fields */
    Pcr->Number = Cpu;
    Pcr->SetMember = 1 << Cpu;
    Pcr->SetMemberCopy = 1 << Cpu;
    Prcb->SetMember = 1 << Cpu;

    /* Initialize the Processor with HAL */
    HalInitializeProcessor(Cpu, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

    /* Set active processors */
    KeActiveProcessors |= Pcr->SetMember;
    KeNumberProcessors++;

    /* Raise to HIGH_LEVEL */
    KfRaiseIrql(HIGH_LEVEL);

    /* Call main kernel intialization */
    KiInitializeKernel(&KiInitialProcess.Pcb,
                       &KiInitialThread.Tcb,
                       init_stack,
                       Prcb,
                       Cpu,
                       LoaderBlock);
}

VOID
INIT_FUNCTION
NTAPI
KeInit2(VOID)
{
    ULONG Protect;

    /* Check if Fxsr was found */
    if (KeI386FxsrPresent)
    {
        /* Enable it. FIXME: Send an IPI */
        Ke386SetCr4(Ke386GetCr4() | X86_CR4_OSFXSR);

        /* Check if XMM was found too */
        if (KeI386XMMIPresent)
        {
            /* Enable it: FIXME: Send an IPI. */
            Ke386SetCr4(Ke386GetCr4() | X86_CR4_OSXMMEXCPT);

            /* FIXME: Implement and enable XMM Page Zeroing for Mm */
        }
    }

    if (KeFeatureBits & KF_GLOBAL_PAGE)
    {
        ULONG Flags;
        /* Enable global pages */
        Ke386GlobalPagesEnabled = TRUE;
        Ke386SaveFlags(Flags);
        Ke386DisableInterrupts();
        Ke386SetCr4(Ke386GetCr4() | X86_CR4_PGE);
        Ke386RestoreFlags(Flags);
    }

    if (KeFeatureBits & KF_FAST_SYSCALL)
    {
        extern void KiFastCallEntry(void);

        /* CS Selector of the target segment. */
        Ke386Wrmsr(0x174, KGDT_R0_CODE, 0);
        /* Target ESP. */
        Ke386Wrmsr(0x175, 0, 0);
        /* Target EIP. */
        Ke386Wrmsr(0x176, (ULONG_PTR)KiFastCallEntry, 0);
    }

    /* Does the CPU Support 'prefetchnta' (SSE)  */
    if(KeFeatureBits & KF_XMMI)
    {
        Protect = MmGetPageProtect(NULL, (PVOID)RtlPrefetchMemoryNonTemporal);
        MmSetPageProtect(NULL, (PVOID)RtlPrefetchMemoryNonTemporal, Protect | PAGE_IS_WRITABLE);
        /* Replace the ret by a nop */
        *(PCHAR)RtlPrefetchMemoryNonTemporal = 0x90;
        MmSetPageProtect(NULL, (PVOID)RtlPrefetchMemoryNonTemporal, Protect);
    }

    /* Set IDT to writable */
    Protect = MmGetPageProtect(NULL, (PVOID)KiIdt);
    MmSetPageProtect(NULL, (PVOID)KiIdt, Protect | PAGE_IS_WRITABLE);
}
