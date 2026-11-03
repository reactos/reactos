/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 kernel initialization
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* The public headers read these through x18 */
C_ASSERT(FIELD_OFFSET(KIPCR, CurrentIrql) == 0x38);
C_ASSERT(FIELD_OFFSET(KIPCR, Prcb) == 0x980);
C_ASSERT(FIELD_OFFSET(KIPCR, Prcb.CurrentThread) == 0x988);
C_ASSERT(FIELD_OFFSET(KIPCR, Prcb.Number) == 0x9A4);

/* SCTLR_EL1 bits that open cache maintenance and DC ZVA to user mode */
#define SCTLR_DZE           (1ULL << 14)
#define SCTLR_UCT           (1ULL << 15)
#define SCTLR_NTWE          (1ULL << 18)
#define SCTLR_UCI           (1ULL << 26)

/* CPACR_EL1.FPEN, no traps for FP and SIMD at EL0 and EL1 */
#define CPACR_FPEN          (3ULL << 20)

/* CNTKCTL_EL1, user mode may read the counters */
#define CNTKCTL_EL0PCTEN    0x1
#define CNTKCTL_EL0VCTEN    0x2

extern ULONG_PTR MainSSDT[];
extern UCHAR MainSSPT[];

/* Implemented in ke/arm64/boot.s */
VOID
KiArm64SetPcr(
    _In_ PKIPCR Pcr);

/* GLOBALS *******************************************************************/

KIPCR DECLSPEC_ALIGN(PAGE_SIZE) KiInitialPcr;

/* Boot stack of the first processor */
UCHAR DECLSPEC_ALIGN(16) KiP0BootStackData[KERNEL_STACK_SIZE] = {0};
PVOID KiP0BootStack = &KiP0BootStackData[KERNEL_STACK_SIZE];

KSPIN_LOCK KiFreezeExecutionLock;
static LDR_DATA_TABLE_ENTRY LdrCoreEntries[3];
ULONG64 KeFeatureBits;

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
KiInitMachineDependent(VOID)
{
    /* Nothing to set up later for now */
}

static
VOID
KiInitializePcr(
    _Out_ PKIPCR Pcr,
    _In_ ULONG ProcessorNumber,
    _In_ PKTHREAD IdleThread)
{
    RtlZeroMemory(Pcr, sizeof(*Pcr));

    Pcr->Self = (PKPCR)Pcr;
    Pcr->LockArray = Pcr->Prcb.LockQueue;
    Pcr->MajorVersion = PCR_MAJOR_VERSION;
    Pcr->MinorVersion = PCR_MINOR_VERSION;
    Pcr->StallScaleFactor = INITIAL_STALL_COUNT;

    /* Interrupts stay masked until the idle loop, the IRQL goes up after KD is ready */
    Pcr->CurrentIrql = PASSIVE_LEVEL;

    Pcr->Prcb.MajorVersion = PRCB_MAJOR_VERSION;
    Pcr->Prcb.MinorVersion = PRCB_MINOR_VERSION;
    Pcr->Prcb.BuildType = 0;
#ifndef CONFIG_SMP
    Pcr->Prcb.BuildType |= PRCB_BUILD_UNIPROCESSOR;
#endif
#if DBG
    Pcr->Prcb.BuildType |= PRCB_BUILD_DEBUG;
#endif

    Pcr->Prcb.LegacyNumber = (UCHAR)ProcessorNumber;
    Pcr->Prcb.Number = ProcessorNumber;
    Pcr->Prcb.SetMember = AFFINITY_MASK(ProcessorNumber);
    Pcr->Prcb.MultiThreadProcessorSet = Pcr->Prcb.SetMember;
    Pcr->Prcb.CurrentThread = IdleThread;
    Pcr->Prcb.StartCycles = KxQueryProcessorCycleTime();
}

static
VOID
KiInitializeCpu(
    _In_ PKIPCR Pcr)
{
    ULONG64 Value;

    /* Floating point and SIMD work everywhere, the trap path saves them */
    Value = KeArm64ReadRegister(ARM64_CPACR_EL1);
    KeArm64WriteRegister(ARM64_CPACR_EL1, Value | CPACR_FPEN);

    Value = KeArm64ReadRegister(ARM64_SCTLR_EL1);
    Value |= SCTLR_DZE | SCTLR_UCT | SCTLR_UCI | SCTLR_NTWE;
    KeArm64WriteRegister(ARM64_SCTLR_EL1, Value);

    Value = KeArm64ReadRegister(ARM64_CNTKCTL_EL1);
    KeArm64WriteRegister(ARM64_CNTKCTL_EL1, Value | CNTKCTL_EL0PCTEN | CNTKCTL_EL0VCTEN);
    __isb(_ARM64_BARRIER_SY);

    KiSetProcessorType();
    Pcr->Prcb.FeatureBits = (ULONG)KiGetFeatureBits();
}

/* The core modules go on the list early so exception dispatch can find their unwind data */
CODE_SEG("INIT")
static
VOID
KiInitModuleList(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PLIST_ENTRY Entry;
    ULONG i;

    InitializeListHead(&PsLoadedModuleList);

    for (Entry = LoaderBlock->LoadOrderListHead.Flink, i = 0;
         (Entry != &LoaderBlock->LoadOrderListHead) && (i < 3);
         Entry = Entry->Flink, i++)
    {
        LdrEntry = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
        LdrCoreEntries[i] = *LdrEntry;
        InsertTailList(&PsLoadedModuleList, &LdrCoreEntries[i].InLoadOrderLinks);
    }
}

VOID
NTAPI
KiInitializeHandBuiltThread(
    _In_ PKTHREAD Thread,
    _In_ PKPROCESS Process,
    _In_ PVOID Stack)
{
    PKPRCB Prcb = KeGetCurrentPrcb();

    KeInitializeThread(Process, Thread, NULL, NULL, NULL, NULL, NULL, Stack);

    Thread->NextProcessor = Prcb->Number;
    Thread->IdealProcessor = Prcb->Number;
    Thread->Priority = HIGH_PRIORITY;
    Thread->State = Running;
    Thread->Affinity = AFFINITY_MASK(Prcb->Number);
    Thread->WaitIrql = DISPATCH_LEVEL;
    Process->ActiveProcessors |= AFFINITY_MASK(Prcb->Number);
}

CODE_SEG("INIT")
VOID
NTAPI
KiInitializeKernel(
    _In_ PKPROCESS InitProcess,
    _In_ PKTHREAD InitThread,
    _In_ PVOID IdleStack,
    _In_ PKPRCB Prcb,
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG_PTR PageDirectory[2];
    PVOID DpcStack;
    ULONG i;

    Prcb->MultiThreadSetMaster = Prcb;

    InitializeListHead(&KeBugcheckCallbackListHead);
    InitializeListHead(&KeBugcheckReasonCallbackListHead);
    KeInitializeSpinLock(&BugCheckCallbackLock);

    KeInitializeDpc(&KiTimerExpireDpc, KiTimerExpiration, NULL);
    KeSetTargetProcessorDpc(&KiTimerExpireDpc, 0);

    KeInitializeSpinLock(&KiProfileLock);
    InitializeListHead(&KiProfileListHead);
    InitializeListHead(&KiProfileSourceListHead);

    for (i = 0; i < TIMER_TABLE_SIZE; i++)
    {
        InitializeListHead(&KiTimerTableListHead[i].Entry);
        KiTimerTableListHead[i].Time.HighPart = 0xFFFFFFFF;
        KiTimerTableListHead[i].Time.LowPart = 0;
    }

    KeInitializeEvent(&KiSwapEvent, SynchronizationEvent, FALSE);
    InitializeListHead(&KiProcessInSwapListHead);
    InitializeListHead(&KiProcessOutSwapListHead);
    InitializeListHead(&KiStackInSwapListHead);

    ExInitializeFastMutex(&KiGenericCallDpcMutex);

    KeServiceDescriptorTable[0].Base = MainSSDT;
    KeServiceDescriptorTable[0].Count = NULL;
    KeServiceDescriptorTable[0].Limit = KiServiceLimit;
    KeServiceDescriptorTable[1].Limit = 0;
    KeServiceDescriptorTable[0].Number = MainSSPT;
    RtlCopyMemory(KeServiceDescriptorTableShadow,
                  KeServiceDescriptorTable,
                  sizeof(KeServiceDescriptorTable));

    InitializeListHead(&KiProcessListHead);

    /* The idle process keeps the loader's page tables */
    PageDirectory[0] = 0;
    PageDirectory[1] = 0;
    KeInitializeProcess(InitProcess, 0, MAXULONG_PTR, PageDirectory, FALSE);
    InitProcess->QuantumReset = MAXCHAR;

    KiInitializeHandBuiltThread(InitThread, InitProcess, IdleStack);

    ExpInitializeExecutive(0, LoaderBlock);

    KiTimeIncrementReciprocal = KiComputeReciprocal(KeMaximumIncrement,
                                                    &KiTimeIncrementShiftCount);

    Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
    Prcb->MinimumDpcRate = KiMinimumDpcRate;
    Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;

    DpcStack = MmCreateKernelStack(FALSE, 0);
    if (!DpcStack)
        KeBugCheckEx(NO_PAGES_AVAILABLE, 1, 0, 0, 0);
    Prcb->DpcStack = DpcStack;
}

CODE_SEG("INIT")
DECLSPEC_NORETURN
VOID
NTAPI
KiSystemStartupBootStack(VOID)
{
    PLOADER_PARAMETER_BLOCK LoaderBlock = KeLoaderBlock;
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKTHREAD Thread = (PKTHREAD)KeLoaderBlock->Thread;
    PKPROCESS Process = Thread->ApcState.Process;
    PVOID KernelStack = (PVOID)KeLoaderBlock->KernelStack;

    Prcb->ParentNode = KeNodeBlock[0];
    Prcb->ParentNode->ProcessorMask |= Prcb->SetMember;

    PoInitializePrcb(Prcb);
    KiSaveProcessorControlState(&Prcb->ProcessorState);
    KiGetCacheInformation();
    KiInitSpinLocks(Prcb, Prcb->Number);

    Prcb->CurrentThread = Thread;
    Prcb->NextThread = NULL;
    Prcb->IdleThread = Thread;

    ExInitPoolLookasidePointers();

    KeLowerIrql(APC_LEVEL);

    if (Prcb->Number == 0)
        KiInitializeKernel(Process, Thread, KernelStack, Prcb, LoaderBlock);
    else
        KiInitializeHandBuiltThread(Thread, Process, KernelStack);

    /* Become the idle thread */
    KfRaiseIrql(DISPATCH_LEVEL);
    KeSetPriorityThread(Thread, 0);

    KiAcquirePrcbLock(Prcb);
    if (!Prcb->NextThread)
        KiIdleSummary |= Prcb->SetMember;
    KiReleasePrcbLock(Prcb);

    KfRaiseIrql(HIGH_LEVEL);
    LoaderBlock->Prcb = 0;

    Thread = KeGetCurrentThread();
    Thread->Priority = 0;

    _enable();
    KeLowerIrql(DISPATCH_LEVEL);
    Thread->WaitIrql = DISPATCH_LEVEL;

    KiIdleLoop();
}

CODE_SEG("INIT")
DECLSPEC_NORETURN
VOID
NTAPI
KiSystemStartup(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PKTHREAD InitialThread;
    PKIPCR Pcr;
    CCHAR Cpu;

    Cpu = KeNumberProcessors++;

    if (Cpu == 0)
    {
        KeLoaderBlock = LoaderBlock;
        LoaderBlock->KernelStack = (ULONG_PTR)KiP0BootStack;
        LoaderBlock->Thread = (ULONG_PTR)&KiInitialThread.Tcb;
        LoaderBlock->Process = (ULONG_PTR)&KiInitialProcess.Pcb;
        LoaderBlock->Prcb = (ULONG_PTR)&KiInitialPcr.Prcb;
    }

    /* Nothing may touch the PCR before x18 points at it */
    Pcr = CONTAINING_RECORD(LoaderBlock->Prcb, KIPCR, Prcb);
    InitialThread = (PKTHREAD)LoaderBlock->Thread;
    KiInitializePcr(Pcr, Cpu, InitialThread);
    KiArm64SetPcr(Pcr);

    KiProcessorBlock[Cpu] = &Pcr->Prcb;
    InitialThread->ApcState.Process = (PVOID)LoaderBlock->Process;

    KiInitializeCpu(Pcr);

    if (Cpu == 0)
    {
        KeFeatureBits = Pcr->Prcb.FeatureBits;
        KiInitModuleList(LoaderBlock);
        KeInitExceptions();
    }

    HalInitializeProcessor(Cpu, KeLoaderBlock);
    KeActiveProcessors |= AFFINITY_MASK(Cpu);

    if (Cpu == 0)
    {
        KdInitSystem(0, KeLoaderBlock);
        if (KdPollBreakIn())
            DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);
    }

    KfRaiseIrql(HIGH_LEVEL);
    KiSwitchToBootStack(LoaderBlock->KernelStack);
}
