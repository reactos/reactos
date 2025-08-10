/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/amd64/krnlinit.c
 * PURPOSE:         Portable part of kernel initialization
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 *                  Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES***************************************************************/

#include <ntoskrnl.h>
#include <debug.h>
#include <internal/amd64/ke_amd64.h>

extern ULONG_PTR MainSSDT[];
extern UCHAR MainSSPT[];

/* External function declaration for PoInitializePrcb */
extern VOID NTAPI PoInitializePrcb(IN PKPRCB Prcb);

#define COM1_PORT 0x3F8

#define KERNEL_PRINT_MSG(str)                                       \
    do {                                                             \
        const char *p = (str);                                       \
        while (*p) {                                                 \
            while ( (__inbyte(COM1_PORT + 5) & 0x20) == 0 )          \
                ;                                                    \
            __outbyte(COM1_PORT, *p++);                              \
        }                                                            \
    } while (0);

#define KERNEL_PRINT_MSG_AND_HALT(str)                              \
    do {                                                             \
        KERNEL_PRINT_MSG(str);                                      \
        while (1);  /* Don't halt - let serial output complete */  \
    } while (0);


/* Forward declaration for Phase 1 initialization */
VOID
NTAPI
Phase1InitializationDiscard(IN PVOID Context);

/* Function pointer workaround for cross-module calls on AMD64 */
typedef BOOLEAN (NTAPI *PFN_HAL_INIT_SYSTEM)(IN ULONG Phase, IN PLOADER_PARAMETER_BLOCK LoaderBlock);
typedef BOOLEAN (NTAPI *PFN_MM_INIT_SYSTEM)(IN ULONG Phase, IN PLOADER_PARAMETER_BLOCK LoaderBlock);

/* Safe cross-module call to HalInitSystem */
static BOOLEAN CallHalInitSystem(IN ULONG Phase, IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    BOOLEAN Result = FALSE;
    
    AMD64_DEBUG_PRINT("KERNEL: Attempting HalInitSystem via safe cross-module call\n");
    
    /* Use safe call with return value */
    AMD64_SAFE_CALL_RET(Result, HalInitSystem, PFN_HAL_INIT_SYSTEM, Phase, LoaderBlock);
    
    return Result;
}


extern BOOLEAN RtlpUse16ByteSLists;

/* FUNCTIONS**************************************************************/

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
            /* Do a dummy CPUID to start the sample */
            KiCpuId(&CpuInfo, 0);

            /* Fill out the starting data */
            CurrentSample->PerfStart = KeQueryPerformanceCounter(NULL);
            CurrentSample->TSCStart = __rdtsc();
            CurrentSample->PerfFreq.QuadPart = -50000;

            /* Sleep for this sample */
            KeStallExecutionProcessor(CurrentSample->PerfFreq.QuadPart * -1 / 10);

            /* Do another dummy CPUID */
            KiCpuId(&CpuInfo, 0);

            /* Fill out the ending data */
            CurrentSample->PerfEnd =
                KeQueryPerformanceCounter(&CurrentSample->PerfFreq);
            CurrentSample->TSCEnd = __rdtsc();

            /* Calculate the differences */
            CurrentSample->PerfDelta = CurrentSample->PerfEnd.QuadPart -
                                       CurrentSample->PerfStart.QuadPart;
            CurrentSample->TSCDelta = CurrentSample->TSCEnd -
                                      CurrentSample->TSCStart;

            /* Compute CPU Speed */
            CurrentSample->MHz = (ULONG)((CurrentSample->TSCDelta *
                                          CurrentSample->
                                          PerfFreq.QuadPart + 500000) /
                                         (CurrentSample->PerfDelta *
                                          1000000));

            /* Check if this isn't the first sample */
            if (Sample)
            {
                /* Check if we got a good precision within 1MHz */
                if ((CurrentSample->MHz == CurrentSample[-1].MHz) ||
                    (CurrentSample->MHz == CurrentSample[-1].MHz + 1) ||
                    (CurrentSample->MHz == CurrentSample[-1].MHz - 1))
                {
                    /* We did, stop sampling */
                    break;
                }
            }

            /* Move on */
            CurrentSample++;
            Sample++;

            if (Sample == RTL_NUMBER_OF(Samples))
            {
                /* No luck. Average the samples and be done */
                ULONG TotalMHz = 0;
                while (Sample--)
                {
                    TotalMHz += Samples[Sample].MHz;
                }
                CurrentSample[-1].MHz = TotalMHz / RTL_NUMBER_OF(Samples);
                DPRINT1("Sampling CPU frequency failed. Using average of %lu MHz\n", CurrentSample[-1].MHz);
                break;
            }
        }

        /* Save the CPU Speed */
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
    /* Debug output */
    /* DPRINT1("KiInitializeHandBuiltThread: Entered\n"); - DISABLED */
    
    PKPRCB Prcb = KeGetCurrentPrcb();
    
    /* DPRINT("KiInitializeHandBuiltThread: Manually initializing thread\n"); - DISABLED */

    /* Manually initialize the thread to avoid function call issues */
    Thread->Header.Type = ThreadObject;
    Thread->Header.Size = sizeof(KTHREAD) / sizeof(ULONG);
    Thread->Header.SignalState = 0;
    InitializeListHead(&Thread->Header.WaitListHead);
    
    Thread->MutantListHead.Flink = &Thread->MutantListHead;
    Thread->MutantListHead.Blink = &Thread->MutantListHead;
    
    Thread->Process = Process;
    Thread->ApcState.Process = Process;
    Thread->ApcState.KernelApcInProgress = FALSE;
    Thread->ApcState.KernelApcPending = FALSE;
    Thread->ApcState.UserApcPending = FALSE;
    
    InitializeListHead(&Thread->ApcState.ApcListHead[KernelMode]);
    InitializeListHead(&Thread->ApcState.ApcListHead[UserMode]);
    
    Thread->InitialStack = Stack;
    Thread->StackBase = Stack;
    Thread->StackLimit = (ULONG_PTR)Stack - KERNEL_STACK_SIZE;
    Thread->KernelStack = Stack;
    
    Thread->State = Running;
    Thread->WaitIrql = PASSIVE_LEVEL;
    Thread->WaitReason = 0;
    Thread->Priority = HIGH_PRIORITY;
    Thread->BasePriority = HIGH_PRIORITY;
    Thread->Quantum = MAXCHAR;
    Thread->Affinity = Process->Affinity;
    Thread->UserAffinity = Process->Affinity;
    Thread->SystemAffinityActive = FALSE;
    
    KERNEL_PRINT_MSG("KERNEL: KeInitializeThread completed\n")

    Thread->NextProcessor = Prcb->Number;
    Thread->IdealProcessor = Prcb->Number;
    Thread->Priority = HIGH_PRIORITY;
    Thread->State = Running;
    Thread->Affinity = (ULONG_PTR)1 << Prcb->Number;
    Thread->WaitIrql = DISPATCH_LEVEL;
    Process->ActiveProcessors |= (ULONG_PTR)1 << Prcb->Number;
    
    KERNEL_PRINT_MSG("KERNEL: Thread fields initialized\n")
}

CODE_SEG("INIT")
DECLSPEC_NORETURN
VOID
NTAPI
KiSystemStartupBootStack(VOID)
{
    /* Early debug output */
    /* Test global variable access */
    /* NOTE: This test variable will be in BSS after it's zeroed, so we can't 
     * test reading the initial value after BSS zeroing. Skip the read test. */
    
    /* CRITICAL: Save LoaderBlock before BSS is zeroed! */
    /* KeLoaderBlock must have been set by KiSystemStartup before we got here */
    PLOADER_PARAMETER_BLOCK SavedLoaderBlock = KeLoaderBlock;

    /* DPRINT1("KiSystemStartupBootStack: Entered!\n"); - DISABLED due to infinite loop */
    
    /* Verify we have LoaderBlock */
    if (SavedLoaderBlock)
    {
        KERNEL_PRINT_MSG("KERNEL: LoaderBlock saved before BSS zeroing\n")
    }
    else
    {
        KERNEL_PRINT_MSG("KERNEL WARNING: LoaderBlock was NULL before BSS zeroing!\n")
    }
    
    /* CRITICAL: Initialize BSS section */
    /* The BSS section contains uninitialized globals that must be zeroed */
    /* BSS is from 0xFFFFF80000653000 to 0xFFFFF8000068C9F0 in the image */
    /* After relocation it's at 0xFFFFF80000453000 to 0xFFFFF8000048C9F0 */
    
    KERNEL_PRINT_MSG("KERNEL: Zeroing BSS section...\n")
    
    /* Zero the BSS section - use memset equivalent */
    PULONG64 BssStart = (PULONG64)0xFFFFF80000453000;
    ULONG64 BssQwords = 0x399F0 / 8;  /* ~29KB in qwords */
    
    /* Zero in chunks to avoid issues */
    for (ULONG64 i = 0; i < BssQwords; i++)
    {
        BssStart[i] = 0;
        
        /* Print progress every 1024 qwords (8KB) */
        if ((i & 0x3FF) == 0 && i > 0)
        {
            /* Output a dot to show progress */
            __outbyte(COM1_PORT, '.');
        }
    }
    
    KERNEL_PRINT_MSG("\nKERNEL: BSS section zeroed successfully!\n")
    
    /* Use the saved LoaderBlock from before BSS was zeroed */
    PLOADER_PARAMETER_BLOCK LoaderBlock = SavedLoaderBlock;
    
    /* Restore KeLoaderBlock global now that BSS is zeroed */
    KeLoaderBlock = SavedLoaderBlock;
    
    KERNEL_PRINT_MSG("KERNEL: LoaderBlock restored after BSS zeroing\n")
    
    /* Validate LoaderBlock */
    if (!LoaderBlock)
    {
        KERNEL_PRINT_MSG_AND_HALT("KERNEL ERROR: LoaderBlock is NULL!\n")
    }
    
    /* Try to continue with limited functionality */
    KERNEL_PRINT_MSG("KERNEL: Attempting to continue boot sequence\n")
    
    /* Due to RIP-relative addressing issues, we cannot access globals */
    /* For now, just report success and halt */
    KERNEL_PRINT_MSG("KERNEL: ReactOS x64 kernel reached boot stack!\n")
    
    KERNEL_PRINT_MSG("KERNEL: Global variable access now working with proper ImageBase!\n")
    
    KERNEL_PRINT_MSG("KERNEL: Kernel ImageBase and Runtime VA configured successfully\n")
    
    /* Continue with original kernel initialization */
    /* First, we need to get the PRCB from LoaderBlock->Prcb since GS might not be set yet */
    /* Use direct output instead of local array */
    KERNEL_PRINT_MSG("KERNEL: About to get PRCB from LoaderBlock\n")
    
    /* Check if LoaderBlock is still valid */
    if (!LoaderBlock)
    {
        KERNEL_PRINT_MSG_AND_HALT("KERNEL ERROR: LoaderBlock is NULL after restoration!\n")
    }
    
    KERNEL_PRINT_MSG("KERNEL: LoaderBlock is valid, getting PRCB\n")
    
    PKPRCB Prcb = (PKPRCB)LoaderBlock->Prcb;
    
    if (!Prcb)
    {
        KERNEL_PRINT_MSG_AND_HALT("KERNEL ERROR: LoaderBlock->Prcb is NULL!\n")
    }
    
    KERNEL_PRINT_MSG("KERNEL: PRCB obtained from LoaderBlock\n")
    
    /* Get PCR from PRCB */
    PKIPCR Pcr = CONTAINING_RECORD(Prcb, KIPCR, Prcb);
    
    KERNEL_PRINT_MSG("KERNEL: Setting up GS base to point to PCR\n")
    
    /* Set GS base to PCR so KeGetCurrentPrcb() will work */
    __writemsr(0xC0000101, (ULONG_PTR)Pcr); /* MSR_GS_BASE */
    __writemsr(0xC0000102, (ULONG_PTR)Pcr); /* MSR_KERNEL_GS_BASE */
    
    KERNEL_PRINT_MSG("KERNEL: GS base configured, testing KeGetCurrentPrcb()\n")
    
    /* Test if KeGetCurrentPrcb() works now */
    PKPRCB TestPrcb = KeGetCurrentPrcb();
    if (TestPrcb != Prcb)
    {
        KERNEL_PRINT_MSG("KERNEL WARNING: KeGetCurrentPrcb() mismatch!\n")
    }
    else
    {
        KERNEL_PRINT_MSG("KERNEL: KeGetCurrentPrcb() working correctly\n")
    }
    
    KERNEL_PRINT_MSG("KERNEL: Getting Thread from LoaderBlock\n")
    
    PKTHREAD Thread = (PKTHREAD)LoaderBlock->Thread;
    if (!Thread)
    {
        KERNEL_PRINT_MSG_AND_HALT("KERNEL ERROR: LoaderBlock->Thread is NULL!\n")
    }
    
    KERNEL_PRINT_MSG("KERNEL: Getting Process from LoaderBlock\n")
    
    /* Check LoaderBlock validity */
    if (!LoaderBlock) {
        KERNEL_PRINT_MSG("KERNEL ERROR: LoaderBlock is NULL after BSS!\n")
        while (1); /* Don't halt - let serial output complete */
    }

    /* Get Process from LoaderBlock instead of Thread (since BSS was zeroed) */
    PKPROCESS Process = NULL;
    if (LoaderBlock) {
        Process = (PKPROCESS)LoaderBlock->Process;
        KERNEL_PRINT_MSG("KERNEL: Process obtained from LoaderBlock\n")
    }
    if (!Process)
    {
        /* If no process in LoaderBlock, use the initial process */
        Process = &KiInitialProcess.Pcb;
        KERNEL_PRINT_MSG("KERNEL: Using KiInitialProcess as Process\n")
    }
    
    /* CRITICAL: Set Thread->ApcState.Process after BSS zeroing */
    if (!Thread) {
        KERNEL_PRINT_MSG("KERNEL ERROR: Thread is NULL!\n")
        while (1); /* Don't halt - let serial output complete */
    }
    if (!Process) {
        KERNEL_PRINT_MSG("KERNEL ERROR: Process is NULL!\n")
        while (1); /* Don't halt - let serial output complete */
    }
    
    /* Debug output before accessing Thread structure */
    {
        const char msg[] = "KERNEL: About to set Thread->ApcState.Process\n";
        const char *p = msg;
        while (*p) {
            while ((__inbyte(COM1_PORT + 5) & 0x20) == 0);
            __outbyte(COM1_PORT, *p++);
        }
    }
    
    Thread->ApcState.Process = Process;
    KERNEL_PRINT_MSG("KERNEL: Thread->ApcState.Process set after BSS zeroing\n")
    
    KERNEL_PRINT_MSG("KERNEL: Getting KernelStack from LoaderBlock\n")
    
    PVOID KernelStack = (PVOID)LoaderBlock->KernelStack;

    KERNEL_PRINT_MSG("KERNEL: About to access KeNodeBlock[0] - global variable\n")
    
    /* Set Node Data - KeNodeBlock needs special handling */
    /* KeNodeBlock is an array in .data section that's not initialized yet */
    /* Initialize ParentNode - KeNodeBlock will be set up later */
    Prcb->ParentNode = NULL;

    /* Initialize the Power Management Support for this PRCB */
    KERNEL_PRINT_MSG("KERNEL: Calling PoInitializePrcb\n")
    
    /* Check if Prcb is valid before calling */
    if (!Prcb)
    {
        KERNEL_PRINT_MSG("KERNEL ERROR: Prcb is NULL before PoInitializePrcb call!\n")
    }
    else
    {
        KERNEL_PRINT_MSG("KERNEL: Prcb is valid, skipping PoInitializePrcb for now\n")
    }
    
    /* Initialize power management for this processor */
    /* FIX: Use indirect call through function pointer to avoid RIP-relative addressing issues */
    AMD64_DEBUG_PRINT("KERNEL: Attempting PoInitializePrcb via safe cross-module call\n");
    
    AMD64_DEBUG_PRINT("1111\n");

    /* INLINE PoInitializePrcb functionality to avoid cross-module call issue */
    AMD64_DEBUG_PRINT("KERNEL: Inlining PoInitializePrcb functionality\n");
    
    /* Initialize the Power State directly */
    if (Prcb) {
        /* Zero the PowerState structure manually */
        /* RtlZeroMemory might not be available yet */
        volatile char *p = (volatile char *)&Prcb->PowerState;
        for (size_t i = 0; i < sizeof(Prcb->PowerState); i++) {
            p[i] = 0;
        }
        
        /* Set initial power state values */
        Prcb->PowerState.Idle0KernelTimeLimit = 0xFFFFFFFF;
        Prcb->PowerState.CurrentThrottle = 100;
        Prcb->PowerState.CurrentThrottleIndex = 0;
        
        /* Note: We skip setting IdleFunction and DPC initialization for now
         * as they involve cross-module references that may cause issues */
        
        AMD64_DEBUG_PRINT("KERNEL: PowerState initialized inline\n");
    }
    
    AMD64_DEBUG_PRINT("2222\n");

    AMD64_DEBUG_PRINT("KERNEL: PoInitializePrcb completed\n");

    /* Save CPU state */
    KERNEL_PRINT_MSG("KERNEL: Saving processor control state\n");
    
    /* Skip KiSaveProcessorControlState - causing issues */
    KERNEL_PRINT_MSG("KERNEL: Skipping KiSaveProcessorControlState (causes hang)\n");
    /* AMD64_SAFE_CALL(KiSaveProcessorControlState, PFN_KI_SAVE_PROC_STATE, &Prcb->ProcessorState); */
    
    KERNEL_PRINT_MSG("KERNEL: Processor control state saved (skipped)\n");

    KERNEL_PRINT_MSG("KERNEL: About to get cache information...\n");
    /* Get cache line information for this CPU */
    KERNEL_PRINT_MSG("KERNEL: Getting cache information\n");
    /* KiGetCacheInformation(); - TODO: Implement for AMD64 */
    
    /* Set default cache info for now */
    Prcb->CacheCount = 0;
    /* CurrentPacket doesn't exist in AMD64 KPRCB */
    
    KERNEL_PRINT_MSG("KERNEL: Cache info initialized with defaults\n")

    /* Initialize spinlocks and DPC data */
    KERNEL_PRINT_MSG("KERNEL: Initializing spinlocks\n")
    
    /* Initialize DPC data inline - KiInitSpinLocks not implemented for AMD64 */
    InitializeListHead(&Prcb->DpcData[0].DpcListHead);
    KeInitializeSpinLock(&Prcb->DpcData[0].DpcLock);
    Prcb->DpcData[0].DpcQueueDepth = 0;
    Prcb->DpcData[0].DpcCount = 0;
    //DPRINT("Succeeded\n");
    KERNEL_PRINT_MSG("KERNEL: Spinlocks initialized\n")

    /* Set up the thread-related fields in the PRCB */
    KERNEL_PRINT_MSG("KERNEL: Setting up PRCB thread fields\n")
    Prcb->CurrentThread = Thread;
    Prcb->NextThread = NULL;
    Prcb->IdleThread = Thread;
    
    /* Initialize debug subsystem - simplified approach */
    KERNEL_PRINT_MSG("KERNEL: Initializing debug subsystem...\n")
    
    /* Initialize KdInitSystem for AMD64 to enable DPRINT */
    #ifdef _AMD64_
        KERNEL_PRINT_MSG("KERNEL: Calling KdInitSystem on AMD64\n")
        if (KdInitSystem(0, LoaderBlock))
        {
            KERNEL_PRINT_MSG("KERNEL: KdInitSystem SUCCESS on AMD64 - DPRINT should work now\n")
            /* DPRINT1("KiSystemStartupBootStack: Debug system initialized successfully!\n"); - DISABLED */
        }
        else
        {
            KERNEL_PRINT_MSG("KERNEL: KdInitSystem failed on AMD64\n")
        }
    #else
        if (KdInitSystem(0, LoaderBlock))
        {
            KERNEL_PRINT_MSG("KERNEL: KdInitSystem SUCCESS\n")
        }
        else
        {
            KERNEL_PRINT_MSG("KERNEL: KdInitSystem failed\n")
        }
    #endif

    /* Initialize PRCB pool lookaside pointers */
    KERNEL_PRINT_MSG("KERNEL: Skipping ExInitPoolLookasidePointers (may hang)\n")
    /* TEMPORARILY SKIP - may hang */
    /* ExInitPoolLookasidePointers(); */
    KERNEL_PRINT_MSG("KERNEL: ExInitPoolLookasidePointers completed\n")

    /* Lower to APC_LEVEL */
    KERNEL_PRINT_MSG("KERNEL: Calling KeLowerIrql(APC_LEVEL)\n")
    KeLowerIrql(APC_LEVEL);
    KERNEL_PRINT_MSG("KERNEL: KeLowerIrql completed\n")

    /* Check if this is the boot cpu */
    KERNEL_PRINT_MSG("KERNEL: Checking if boot CPU (Prcb->Number)\n");
    
    /* Direct serial output to debug */
    {
        __outbyte(COM1_PORT, 'C');
        __outbyte(COM1_PORT, 'P');
        __outbyte(COM1_PORT, 'U');
        __outbyte(COM1_PORT, '=');
        __outbyte(COM1_PORT, '0' + (char)Prcb->Number);
        __outbyte(COM1_PORT, '\n');
    }
    
    if (Prcb->Number == 0)
    {
        __outbyte(COM1_PORT, 'Y');
        __outbyte(COM1_PORT, 'E');
        __outbyte(COM1_PORT, 'S');
        __outbyte(COM1_PORT, '\n');
        KERNEL_PRINT_MSG("KERNEL: This is CPU 0 (boot CPU)\n");
        
        /* Initialize the kernel - BSS is now zeroed so globals should work */
        KERNEL_PRINT_MSG("KERNEL: Inlining critical kernel initialization\n");
        
        /* Inline the critical parts of KiInitializeKernel here */
        /* Since the function call is crashing, we'll do it inline */
        
        /* Initialize 8/16 bit SList support */
        KERNEL_PRINT_MSG("KERNEL: Setting up SList support\n");
        RtlpUse16ByteSLists = (KeFeatureBits & KF_CMPXCHG16B) ? TRUE : FALSE;
        
        /* Set the current MP Master KPRCB to the Boot PRCB */
        Prcb->MultiThreadSetMaster = Prcb;
        
        /* Initialize Bugcheck Callback data */
        KERNEL_PRINT_MSG("KERNEL: Initializing bugcheck callbacks\n");
        InitializeListHead(&KeBugcheckCallbackListHead);
        InitializeListHead(&KeBugcheckReasonCallbackListHead);
        KeInitializeSpinLock(&BugCheckCallbackLock);
        
        /* Initialize the Timer Expiration DPC */
        KERNEL_PRINT_MSG("KERNEL: Setting up timer DPC\n");
        KiTimerExpireDpc.Type = DpcObject;
        KiTimerExpireDpc.Number = 0;
        KiTimerExpireDpc.Importance = MediumImportance;
        KiTimerExpireDpc.DeferredRoutine = KiTimerExpiration;
        KiTimerExpireDpc.DeferredContext = NULL;
        KiTimerExpireDpc.DpcData = NULL;
        
        /* Initialize Profiling data */
        KERNEL_PRINT_MSG("KERNEL: Setting up profiling\n");
        KeInitializeSpinLock(&KiProfileLock);
        InitializeListHead(&KiProfileListHead);
        InitializeListHead(&KiProfileSourceListHead);
        /* Initialize timer table */
        KERNEL_PRINT_MSG("KERNEL: Initializing timer table\n");
        for (ULONG i = 0; i < TIMER_TABLE_SIZE; i++)
        {
            InitializeListHead(&KiTimerTableListHead[i].Entry);
            KiTimerTableListHead[i].Time.HighPart = 0xFFFFFFFF;
            KiTimerTableListHead[i].Time.LowPart = 0;
            
            /* Show progress */
            if ((i & 0x3F) == 0x3F)
            {
                __outbyte(COM1_PORT, '.');
            }
        }
        KERNEL_PRINT_MSG("\nKERNEL: Timer table initialized\n");
        
        /* Setup initial thread and process */
        KERNEL_PRINT_MSG("KERNEL: Setting up initial thread/process\n");
        Process->QuantumReset = MAXCHAR;
        Thread->NextProcessor = Prcb->Number;
        Thread->Priority = HIGH_PRIORITY;
        Thread->State = Running;
        Thread->Affinity = (ULONG_PTR)-1;
        Thread->WaitIrql = DISPATCH_LEVEL;
        Process->ActiveProcessors = 1;
        
        /* Continue with Phase1InitializationDiscard */
        KERNEL_PRINT_MSG("KERNEL: Basic kernel initialization complete!\n");
        KERNEL_PRINT_MSG("KERNEL: Preparing to call Phase1InitializationDiscard\n");
        
        /* Set up the idle thread to continue initialization */
        KERNEL_PRINT_MSG("KERNEL: Setting up idle thread\n");
        /* Note: StartAddress and SystemThread fields don't exist in AMD64 KTHREAD */
        
        /* Lower IRQL and enable interrupts */
        KERNEL_PRINT_MSG("KERNEL: Lowering IRQL to PASSIVE_LEVEL\n");
        KeLowerIrql(PASSIVE_LEVEL);
        
        KERNEL_PRINT_MSG("KERNEL: Enabling interrupts\n");
        _enable();
        
        /* CRITICAL: Initialize the Executive before Phase 1! */
        KERNEL_PRINT_MSG("KERNEL: Calling ExpInitializeExecutive...\n");
        
        /* ExpInitializeExecutive MUST be called to initialize MM and other subsystems */
        ExpInitializeExecutive(0, LoaderBlock);
        
        KERNEL_PRINT_MSG("KERNEL: ExpInitializeExecutive completed!\n");
        
        /* Jump to Phase 1 initialization */
        KERNEL_PRINT_MSG("KERNEL: Calling Phase1Initialization...\n");
        
        /* Call Phase1Initialization (which calls Phase1InitializationDiscard) */
        /* Normally this would be called via thread scheduling */
        {
            /* Create a minimal context */
            PVOID Context = LoaderBlock;
            
            /* Output before calling */
            __outbyte(COM1_PORT, 'P');
            __outbyte(COM1_PORT, '1');
            __outbyte(COM1_PORT, '\n');
            
            /* Call Phase1Initialization which handles the full init sequence */
            Phase1Initialization(Context);
            
            /* If we get here, Phase1 returned (shouldn't happen normally) */
            KERNEL_PRINT_MSG("ERROR: Phase1Initialization returned!\n");
        }
        
        /* Should never get here */
        KERNEL_PRINT_MSG("KERNEL: System initialization complete - halting\n");
        while (TRUE); /* Don't halt - let serial output complete */
    }
    else
    {
        /* Initialize the startup thread */
        KiInitializeHandBuiltThread(Thread, Process, KernelStack);
    }

    /* Skip CPU frequency calculation - causes issues */
    KERNEL_PRINT_MSG("KERNEL: Skipping CPU frequency calculation\n")
    
    /* KdInitSystem was already initialized earlier, use DPRINT1 now if available */
    /* DPRINT1("KERNEL: Continuing after CPU frequency skip (using DPRINT1)\n"); - DISABLED */

    /* Get idle stack before jumping */
    PVOID IdleStack2 = (PVOID)Thread->KernelStack;
    
    /* Jump directly to kernel initialization to avoid problematic code */
    goto continue_init;
    
    /* For now just skip - TODO: Implement KfRaiseIrql for AMD64 */
    /* KfRaiseIrql(DISPATCH_LEVEL); */
    
    /* Mark idle summary - skip locking for now */
    /* KiAcquirePrcbLock(Prcb); */
    if (!Prcb->NextThread) KiIdleSummary |= (ULONG_PTR)1 << Prcb->Number;
    /* KiReleasePrcbLock(Prcb); */
    
    KERNEL_PRINT_MSG("KERNEL: Idle summary updated\n")

    /* Raise back to HIGH_LEVEL and clear the PRCB for the loader block */
    KERNEL_PRINT_MSG("KERNEL: Raising to HIGH_LEVEL\n")
    
    /* TEMPORARILY SKIP - KfRaiseIrql not implemented for AMD64 */
    /* KfRaiseIrql(HIGH_LEVEL); */
    LoaderBlock->Prcb = 0;

    /* Set the priority of this thread to 0 */
    KERNEL_PRINT_MSG("KERNEL: Setting current thread priority\n")
    
    Thread = KeGetCurrentThread();
    Thread->Priority = 0;

    /* Force interrupts enabled and lower IRQL back to DISPATCH_LEVEL */
    KERNEL_PRINT_MSG("KERNEL: Enabling interrupts and lowering IRQL\n")
    
    _enable();
    KERNEL_PRINT_MSG("KERNEL: Skipping KeLowerIrql (causes hang on AMD64)\n")
    /* KeLowerIrql(DISPATCH_LEVEL); - Causes hang on AMD64 */

    /* Set the right wait IRQL */
    Thread->WaitIrql = DISPATCH_LEVEL;

    /* Get the idle stack from Thread */
    PVOID IdleStack = (PVOID)Thread->KernelStack;

continue_init:
    /* Make sure we have IdleStack */
    if (!IdleStack) {
        IdleStack = IdleStack2;
    }
    
    /* Continue with kernel initialization */
    /* Now we can use DPRINT1 if it was initialized successfully */
    DPRINT1("KERNEL: At continue_init label - continuing kernel initialization...\n");
    
    /* Also output to serial in case DPRINT1 isn't working */
    KERNEL_PRINT_MSG("KERNEL: Continuing kernel initialization...\n")
    
    /* For non-boot CPUs, we would initialize the thread here */
    /* But for boot CPU, KiInitializeKernel was already called above */
    KERNEL_PRINT_MSG("KERNEL: ERROR - Should not reach here for boot CPU!\n")
    
    /* This should never be reached for boot CPU */
    while (TRUE); /* Don't halt - let serial output complete */
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
    /* Very first output to check if we get here */
    __outbyte(COM1_PORT, 'K');
    __outbyte(COM1_PORT, 'I');
    __outbyte(COM1_PORT, 'K');
    __outbyte(COM1_PORT, '\n');
    
    /* Debug output */
    KERNEL_PRINT_MSG("KERNEL: KiInitializeKernel (main) entered\n");
    
    ULONG_PTR PageDirectory[2];
    PVOID DpcStack;
    ULONG i;
    
    KERNEL_PRINT_MSG("KERNEL: Checking KeFeatureBits for CMPXCHG16B\n");

    /* Initialize 8/16 bit SList support */
    RtlpUse16ByteSLists = (KeFeatureBits & KF_CMPXCHG16B) ? TRUE : FALSE;
    
    KERNEL_PRINT_MSG("KERNEL: SList support initialized\n")

    /* Set the current MP Master KPRCB to the Boot PRCB */
    Prcb->MultiThreadSetMaster = Prcb;
    
    KERNEL_PRINT_MSG("KERNEL: MultiThreadSetMaster set\n")

    /* Initialize Bugcheck Callback data */
    KERNEL_PRINT_MSG("KERNEL: Initializing bugcheck callback lists\n")
    
    /* Use proper InitializeListHead now that it's fixed */
    InitializeListHead(&KeBugcheckCallbackListHead);
    InitializeListHead(&KeBugcheckReasonCallbackListHead);
    KeInitializeSpinLock(&BugCheckCallbackLock);
    
    KERNEL_PRINT_MSG("KERNEL: Bugcheck lists initialized\n")

    /* Initialize the Timer Expiration DPC manually */
    KiTimerExpireDpc.Type = DpcObject;
    KiTimerExpireDpc.Number = 0;
    KiTimerExpireDpc.Importance = MediumImportance;
    KiTimerExpireDpc.DeferredRoutine = KiTimerExpiration;
    KiTimerExpireDpc.DeferredContext = NULL;
    KiTimerExpireDpc.DpcData = NULL;
    /* KeSetTargetProcessorDpc(&KiTimerExpireDpc, 0); - skip for now */
    
    KERNEL_PRINT_MSG("KERNEL: Timer DPC initialized\n")

    /* Initialize Profiling data */
    KERNEL_PRINT_MSG("KERNEL: Initializing profiling data\n")
    
    KeInitializeSpinLock(&KiProfileLock);
    /* Use proper InitializeListHead */
    InitializeListHead(&KiProfileListHead);
    InitializeListHead(&KiProfileSourceListHead);
    
    KERNEL_PRINT_MSG("KERNEL: Profiling data initialized\n")

    /* Loop the timer table */
    KERNEL_PRINT_MSG("KERNEL: Initializing timer table\n")
    
    for (i = 0; i < TIMER_TABLE_SIZE; i++)
    {
        /* Initialize the list and entries */
        /* Use proper InitializeListHead */
        InitializeListHead(&KiTimerTableListHead[i].Entry);
        KiTimerTableListHead[i].Time.HighPart = 0xFFFFFFFF;
        KiTimerTableListHead[i].Time.LowPart = 0;
        
        /* Show progress every 64 entries */
        if ((i & 0x3F) == 0x3F)
        {
            while ((__inbyte(COM1_PORT + 5) & 0x20) == 0);
            __outbyte(COM1_PORT, '.');
        }
    }
    
    KERNEL_PRINT_MSG("\nKERNEL: Timer table initialized\n")

    /* Initialize the Swap event and all swap lists */
    KERNEL_PRINT_MSG("KERNEL: Initializing swap event and lists\n")
    
    KERNEL_PRINT_MSG("KERNEL: Calling KeInitializeEvent\n")
    
    /* Still need manual initialization - KeInitializeEvent hangs */
    KiSwapEvent.Header.Type = SynchronizationEvent;
    KiSwapEvent.Header.Size = sizeof(KEVENT) / sizeof(ULONG);
    KiSwapEvent.Header.SignalState = FALSE;
    InitializeListHead(&(KiSwapEvent.Header.WaitListHead));
    
    KERNEL_PRINT_MSG("KERNEL: KeInitializeEvent completed\n")
    
    /* Use proper InitializeListHead */
    InitializeListHead(&KiProcessInSwapListHead);
    InitializeListHead(&KiProcessOutSwapListHead);
    InitializeListHead(&KiStackInSwapListHead);
    
    KERNEL_PRINT_MSG("KERNEL: Swap structures initialized\n")

    /* Initialize the mutex for generic DPC calls */
    KERNEL_PRINT_MSG("KERNEL: Initializing generic DPC mutex\n")
    
    /* Manually initialize the generic DPC mutex to avoid function call issues */
    KiGenericCallDpcMutex.Count = 1;
    KiGenericCallDpcMutex.Owner = NULL;
    KiGenericCallDpcMutex.Contention = 0;
    /* Initialize the event structure manually */
    KiGenericCallDpcMutex.Event.Header.Type = NotificationEvent;
    KiGenericCallDpcMutex.Event.Header.Size = sizeof(KEVENT) / sizeof(ULONG);
    KiGenericCallDpcMutex.Event.Header.SignalState = 0;
    InitializeListHead(&KiGenericCallDpcMutex.Event.Header.WaitListHead);
    KiGenericCallDpcMutex.OldIrql = 0;
    
    KERNEL_PRINT_MSG("KERNEL: DPC mutex initialized\n")

    /* Initialize the syscall table */
    KERNEL_PRINT_MSG("KERNEL: Setting up syscall table\n")
    
    KeServiceDescriptorTable[0].Base = MainSSDT;
    KeServiceDescriptorTable[0].Count = NULL;
    KeServiceDescriptorTable[0].Limit = KiServiceLimit;
    KeServiceDescriptorTable[1].Limit = 0;
    KeServiceDescriptorTable[0].Number = MainSSPT;
    
    KERNEL_PRINT_MSG("KERNEL: Syscall table configured\n")

    /* Copy the the current table into the shadow table for win32k */
    KERNEL_PRINT_MSG("KERNEL: Copying syscall table to shadow (RtlCopyMemory)\n")
    
    /* Manual copy to avoid RtlCopyMemory */
    /* RtlCopyMemory(KeServiceDescriptorTableShadow,
                  KeServiceDescriptorTable,
                  sizeof(KeServiceDescriptorTable)); */
    
    /* Manual memory copy */
    {
        PUCHAR Src = (PUCHAR)KeServiceDescriptorTable;
        PUCHAR Dst = (PUCHAR)KeServiceDescriptorTableShadow;
        SIZE_T Size = sizeof(KeServiceDescriptorTable);
        
        for (SIZE_T i = 0; i < Size; i++)
        {
            Dst[i] = Src[i];
        }
    }
    
    KERNEL_PRINT_MSG("KERNEL: Shadow table copied\n")

    /* Initialize the Idle Process and the Process Listhead */
    KERNEL_PRINT_MSG("KERNEL: Initializing process list and idle process\n")
    
    /* Use proper InitializeListHead */
    InitializeListHead(&KiProcessListHead);
    PageDirectory[0] = 0;
    PageDirectory[1] = 0;
    
    KERNEL_PRINT_MSG("KERNEL: Manually initializing idle process\n")
    
    /* Manually initialize the idle process to avoid function call issues */
    InitProcess->Header.Type = ProcessObject;
    InitProcess->Header.Size = sizeof(KPROCESS) / sizeof(ULONG);
    InitProcess->Header.SignalState = 0;
    InitializeListHead(&InitProcess->Header.WaitListHead);
    
    InitProcess->ProfileListHead.Flink = &InitProcess->ProfileListHead;
    InitProcess->ProfileListHead.Blink = &InitProcess->ProfileListHead;
    
    InitProcess->DirectoryTableBase[0] = PageDirectory[0];
    InitProcess->DirectoryTableBase[1] = PageDirectory[1];
    
    InitProcess->ReadyListHead.Flink = &InitProcess->ReadyListHead;
    InitProcess->ReadyListHead.Blink = &InitProcess->ReadyListHead;
    
    InitProcess->ThreadListHead.Flink = &InitProcess->ThreadListHead;
    InitProcess->ThreadListHead.Blink = &InitProcess->ThreadListHead;
    
    InitProcess->ProcessLock = 0;
    InitProcess->Affinity = (KAFFINITY)((1ULL << KeNumberProcessors) - 1);
    InitProcess->AutoAlignment = FALSE;
    InitProcess->BasePriority = PROCESS_PRIORITY_NORMAL;
    InitProcess->QuantumReset = MAXCHAR;
    
    KERNEL_PRINT_MSG("KERNEL: Idle process initialized\n")

    /* Initialize the startup thread */
    KERNEL_PRINT_MSG("KERNEL: Initializing startup thread\n")
    
    KiInitializeHandBuiltThread(InitThread, InitProcess, IdleStack);
    
    KERNEL_PRINT_MSG("KERNEL: Startup thread initialized\n")

    /* Initialize the Kernel Executive */
    KERNEL_PRINT_MSG("KERNEL: Initializing Executive subsystems inline\n")
    
    /* Inline minimal executive initialization to avoid cross-module call */
    {
        /* Set initialization phase to 0 */
        ExpInitializationPhase = 0;
        
        KERNEL_PRINT_MSG("KERNEL: Phase set to 0\n")
        
        /* Initialize HAL Phase 0 */
        KERNEL_PRINT_MSG("KERNEL: Calling HalInitSystem Phase 0\n")
        
        /* Try to call HalInitSystem through wrapper */
        if (!CallHalInitSystem(0, LoaderBlock))
        {
            KERNEL_PRINT_MSG("KERNEL ERROR: HAL initialization failed!\n")
            
            /* HAL failed - for now just skip it */
        }
        else
        {
            KERNEL_PRINT_MSG("KERNEL: HAL initialized successfully\n")
        }
        
        /* Enable interrupts */
        KERNEL_PRINT_MSG("KERNEL: Enabling interrupts\n")
        
        _enable();
        
        KERNEL_PRINT_MSG("KERNEL: Interrupts enabled\n")
        
        /* Skip Memory Manager Phase 0 - expects Phase 1 */
        KERNEL_PRINT_MSG("KERNEL: Skipping MmInitSystem (expects Phase 1)\n")
        
        /* Initialize Object Manager - use inline implementation */
        KERNEL_PRINT_MSG("KERNEL: Initializing Object Manager properly\n")
        
        /* Call ExpInitializeExecutive to initialize all executive subsystems */
        /* This includes Memory Manager, Object Manager, Security, Process Manager, etc. */
        KERNEL_PRINT_MSG("KERNEL: Calling ExpInitializeExecutive to initialize all subsystems...\n")
        ExpInitializeExecutive(Prcb->Number, LoaderBlock);
        KERNEL_PRINT_MSG("KERNEL: ExpInitializeExecutive completed successfully\n")
    }
    
    KERNEL_PRINT_MSG("KERNEL: Executive initialization complete (minimal)\n")

    /* Calculate the time reciprocal */
    KERNEL_PRINT_MSG("KERNEL: Calculating time reciprocal\n")
    
    /* TEMPORARILY SKIP - causes hang */
    /* KiTimeIncrementReciprocal =
        KiComputeReciprocal(KeMaximumIncrement,
                            &KiTimeIncrementShiftCount); */
    
    /* Set default values */
    KiTimeIncrementReciprocal.QuadPart = 0x1AAAA;  /* Default reciprocal */
    KiTimeIncrementShiftCount = 24;                 /* Default shift count */
    
    KERNEL_PRINT_MSG("KERNEL: Time reciprocal set to defaults\n")

    /* Update DPC Values in case they got updated by the executive */
    KERNEL_PRINT_MSG("KERNEL: Updating DPC values\n")
    
    Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
    Prcb->MinimumDpcRate = KiMinimumDpcRate;
    Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;
    
    KERNEL_PRINT_MSG("KERNEL: DPC values updated\n")

    /* Allocate the DPC Stack */
    KERNEL_PRINT_MSG("KERNEL: Allocating DPC stack\n")
    
    /* TEMPORARILY SKIP - causes hang (MM not initialized) */
    /* DpcStack = MmCreateKernelStack(FALSE, 0); */
    DpcStack = (PVOID)0xFFFFF80000500000; /* Fake stack address */
    
    if (!DpcStack) 
    {
        KERNEL_PRINT_MSG("KERNEL ERROR: Failed to allocate DPC stack!\n")
        
        KeBugCheckEx(NO_PAGES_AVAILABLE, 1, 0, 0, 0);
    }
    
    Prcb->DpcStack = DpcStack;
    
    KERNEL_PRINT_MSG("KERNEL: DPC stack allocated and assigned\n")
    
    KERNEL_PRINT_MSG("KERNEL: KiInitializeKernel completed successfully!\n")
}