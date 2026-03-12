/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/amd64/krnlinit.c
 * PURPOSE:         Portable part of kernel initialization
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

extern ULONG_PTR MainSSDT[];
extern UCHAR MainSSPT[];

extern BOOLEAN RtlpUse16ByteSLists;

/* GLOBAL DATA ***************************************************************/

/* * Synchronization barrier for Multi-core startup.
 * 0 = Master core initializing.
 * 1 = Kernel structures ready for secondary cores.
 */
volatile LONG KiMasterPhase = 0;

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
KiInitializeKernel(IN PKPROCESS InitProcess,
                   IN PKTHREAD InitThread,
                   IN PVOID IdleStack,
                   IN PKPRCB Prcb,
                   IN PLOADER_PARAMETER_BLOCK LoaderBlock);

CODE_SEG("INIT")
VOID
KiCalculateCpuFrequency(
    IN PKPRCB Prcb)
{
    if (Prcb->FeatureBits & KF_RDTSC)
    {
        ULONG Sample = 0;
        CPU_INFO CpuInfo;
        KI_SAMPLE_MAP Samples[10];
        PKI_SAMPLE_MAP CurrentSample = Samples;

        /* Start sampling loop */
        for (;;)
        {
            /* Do a dummy CPUID to serialize the instruction stream for accurate TSC */
            KiCpuId(&CpuInfo, 0);

            /* Fill out the starting data */
            CurrentSample->PerfStart = KeQueryPerformanceCounter(NULL);
            CurrentSample->TSCStart = __rdtsc();
            
            /* * OPTIMIZATION: Use a fixed 50ms interval for more reliable 
             * samples on both high-speed i9 and legacy Pentium chips.
             */
            KeStallExecutionProcessor(50000);

            /* Do another dummy CPUID to end the sample */
            KiCpuId(&CpuInfo, 0);

            /* Fill out the ending data */
            CurrentSample->PerfEnd = KeQueryPerformanceCounter(&CurrentSample->PerfFreq);
            CurrentSample->TSCEnd = __rdtsc();

            /* Calculate the differences */
            CurrentSample->PerfDelta = CurrentSample->PerfEnd.QuadPart -
                                       CurrentSample->PerfStart.QuadPart;
            CurrentSample->TSCDelta = CurrentSample->TSCEnd -
                                      CurrentSample->TSCStart;

            /* Compute CPU Speed */
            CurrentSample->MHz = (ULONG)((CurrentSample->TSCDelta *
                                          CurrentSample->PerfFreq.QuadPart + 500000) /
                                         (CurrentSample->PerfDelta * 1000000));

            /* Check if this isn't the first sample */
            if (Sample > 0)
            {
                LONG MHzDelta = (LONG)CurrentSample->MHz - (LONG)CurrentSample[-1].MHz;

                /* * UNIVERSAL CHECK: 
                 * 1. 1MHz precision for legacy/stable CPUs.
                 * 2. 3MHz tolerance for modern CPUs with Turbo Jitter (Sample 3+).
                 */
                if (MHzDelta >= -1 && MHzDelta <= 1)
                {
                    break;
                }
                else if (Sample >= 2 && (MHzDelta >= -3 && MHzDelta <= 3))
                {
                    /* On modern i9, we accept minor jitter to speed up boot */
                    break;
                }
            }

            /* Move on to next sample if needed */
            CurrentSample++;
            Sample++;

            if (Sample == RTL_NUMBER_OF(Samples))
            {
                /* No luck with precision. Average the results. */
                ULONG TotalMHz = 0;
                for (ULONG i = 0; i < RTL_NUMBER_OF(Samples); i++)
                {
                    TotalMHz += Samples[i].MHz;
                }
                CurrentSample[-1].MHz = TotalMHz / RTL_NUMBER_OF(Samples);
                DPRINT1("Sampling CPU frequency failed to stabilize. Using average: %lu MHz\n", CurrentSample[-1].MHz);
                break;
            }
        }

        /* Save the CPU Speed to the PRCB */
        Prcb->MHz = CurrentSample[-1].MHz;
    }
}

VOID
NTAPI
KiInitializeHandBuiltThread(
    IN PKTHREAD Thread,
    IN PKPROCESS Process,
    IN PVOID Stack)
{
    PKPRCB Prcb = KeGetCurrentPrcb();

    /* Setup the Thread */
    KeInitializeThread(Process, Thread, NULL, NULL, NULL, NULL, NULL, Stack);

    Thread->NextProcessor = Prcb->Number;
    Thread->IdealProcessor = Prcb->Number;
    Thread->Priority = HIGH_PRIORITY;
    Thread->State = Running;
    Thread->Affinity = (ULONG_PTR)1 << Prcb->Number;
    Thread->WaitIrql = DISPATCH_LEVEL;
    Process->ActiveProcessors |= (ULONG_PTR)1 << Prcb->Number;
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

    /* Set Node Data */
    Prcb->ParentNode = KeNodeBlock[0];
    Prcb->ParentNode->ProcessorMask |= Prcb->SetMember;

    /* Initialize the Power Management Support for this PRCB */
    PoInitializePrcb(Prcb);

    /* Save CPU state */
    KiSaveProcessorControlState(&Prcb->ProcessorState);

    /* Get cache line information for this CPU */
    KiGetCacheInformation();

    /* Initialize spinlocks and DPC data */
    KiInitSpinLocks(Prcb, Prcb->Number);

    /* Set up the thread-related fields in the PRCB */
    Prcb->CurrentThread = Thread;
    Prcb->NextThread = NULL;
    Prcb->IdleThread = Thread;

    /* Initialize PRCB pool lookaside pointers */
    ExInitPoolLookasidePointers();

    /* Lower to APC_LEVEL */
    KeLowerIrql(APC_LEVEL);

    /* --- THE UNIVERSAL BARRIER --- */
    if (Prcb->Number == 0)
    {
        /* Master Core: Runs global kernel initialization */
        KiInitializeKernel(Process, Thread, KernelStack, Prcb, LoaderBlock);

        /* Signal secondary cores that it is safe to proceed */
        InterlockedExchange(&KiMasterPhase, 1);
    }
    else
    {
        /* Secondary Cores: Wait for Master to finish to avoid race conditions */
        while (InterlockedCompareExchange(&KiMasterPhase, 1, 1) != 1)
        {
            /* Use the x86 'PAUSE' instruction to reduce power/heat during wait */
            YieldProcessor();
        }

        /* Initialize the startup thread for this specific core */
        KiInitializeHandBuiltThread(Thread, Process, KernelStack);
    }

    /* Calculate the CPU frequency (Per-core for older non-invariant TSC systems) */
    KiCalculateCpuFrequency(Prcb);

    /* Raise to Dispatch */
    KfRaiseIrql(DISPATCH_LEVEL);

    /* Set the Idle Priority to 0. This will jump into Phase 1 */
    KeSetPriorityThread(Thread, 0);

    /* If there's no thread scheduled, put this CPU in the Idle summary */
    KiAcquirePrcbLock(Prcb);
    if (!Prcb->NextThread) KiIdleSummary |= (ULONG_PTR)1 << Prcb->Number;
    KiReleasePrcbLock(Prcb);

    /* Raise back to HIGH_LEVEL and clear the PRCB for the loader block */
    KfRaiseIrql(HIGH_LEVEL);
    LoaderBlock->Prcb = 0;

    /* Set the priority of this thread to 0 */
    Thread = KeGetCurrentThread();
    Thread->Priority = 0;

    /* Force interrupts enabled and lower IRQL back to DISPATCH_LEVEL */
    _enable();
    KeLowerIrql(DISPATCH_LEVEL);

    /* Set the right wait IRQL */
    Thread->WaitIrql = DISPATCH_LEVEL;

    /* Jump into the idle loop */
    KiIdleLoop();
}

CODE_SEG("INIT")
VOID
NTAPI
KiInitializeKernel(IN PKPROCESS InitProcess,
                   IN PKTHREAD InitThread,
                   IN PVOID IdleStack,
                   IN PKPRCB Prcb,
                   IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG_PTR PageDirectory[2];
    PVOID DpcStack;
    ULONG i;

    /* Initialize 8/16 bit SList support */
    RtlpUse16ByteSLists = (KeFeatureBits & KF_CMPXCHG16B) ? TRUE : FALSE;

    /* Set the current MP Master KPRCB to the Boot PRCB */
    Prcb->MultiThreadSetMaster = Prcb;

    /* Initialize Bugcheck Callback data */
    InitializeListHead(&KeBugcheckCallbackListHead);
    InitializeListHead(&KeBugcheckReasonCallbackListHead);
    KeInitializeSpinLock(&BugCheckCallbackLock);

    /* Initialize the Timer Expiration DPC */
    KeInitializeDpc(&KiTimerExpireDpc, KiTimerExpiration, NULL);
    KeSetTargetProcessorDpc(&KiTimerExpireDpc, 0);

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

    /* Initialize the Swap event and all swap lists */
    KeInitializeEvent(&KiSwapEvent, SynchronizationEvent, FALSE);
    InitializeListHead(&KiProcessInSwapListHead);
    InitializeListHead(&KiProcessOutSwapListHead);
    InitializeListHead(&KiStackInSwapListHead);

    /* Initialize the mutex for generic DPC calls */
    ExInitializeFastMutex(&KiGenericCallDpcMutex);

    /* Initialize the syscall table */
    KeServiceDescriptorTable[0].Base = MainSSDT;
    KeServiceDescriptorTable[0].Count = NULL;
    KeServiceDescriptorTable[0].Limit = KiServiceLimit;
    KeServiceDescriptorTable[1].Limit = 0;
    KeServiceDescriptorTable[0].Number = MainSSPT;

    /* Copy the current table into the shadow table for win32k */
    RtlCopyMemory(KeServiceDescriptorTableShadow,
                  KeServiceDescriptorTable,
                  sizeof(KeServiceDescriptorTable));

    /* Initialize the Idle Process and the Process Listhead */
    InitializeListHead(&KiProcessListHead);
    PageDirectory[0] = 0;
    PageDirectory[1] = 0;
    KeInitializeProcess(InitProcess,
                        0,
                        MAXULONG_PTR,
                        PageDirectory,
                        FALSE);
    InitProcess->QuantumReset = MAXCHAR;

    /* Initialize the startup thread */
    KiInitializeHandBuiltThread(InitThread, InitProcess, IdleStack);

    /* Initialize the Kernel Executive */
    ExpInitializeExecutive(0, LoaderBlock);

    /* Calculate the time reciprocal */
    KiTimeIncrementReciprocal =
        KiComputeReciprocal(KeMaximumIncrement,
                            &KiTimeIncrementShiftCount);

    /* Update DPC Values in case they got updated by the executive */
    Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
    Prcb->MinimumDpcRate = KiMinimumDpcRate;
    Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;

    /* Allocate the DPC Stack */
    DpcStack = MmCreateKernelStack(FALSE, 0);
    if (!DpcStack) KeBugCheckEx(NO_PAGES_AVAILABLE, 1, 0, 0, 0);
    Prcb->DpcStack = DpcStack;
}
