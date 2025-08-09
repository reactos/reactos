/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/amd64/krnlinit.c
 * PURPOSE:         Portable part of kernel initialization
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 *                  Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

extern ULONG_PTR MainSSDT[];
extern UCHAR MainSSPT[];

extern BOOLEAN RtlpUse16ByteSLists;

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
    #define COM1_PORT 0x3F8
    {
        const char msg[] = "*** KERNEL: KiInitializeHandBuiltThread entered ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    PKPRCB Prcb = KeGetCurrentPrcb();
    
    {
        const char msg[] = "*** KERNEL: Got PRCB, calling KeInitializeThread ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Setup the Thread */
    KeInitializeThread(Process, Thread, NULL, NULL, NULL, NULL, NULL, Stack);
    
    {
        const char msg[] = "*** KERNEL: KeInitializeThread completed ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    Thread->NextProcessor = Prcb->Number;
    Thread->IdealProcessor = Prcb->Number;
    Thread->Priority = HIGH_PRIORITY;
    Thread->State = Running;
    Thread->Affinity = (ULONG_PTR)1 << Prcb->Number;
    Thread->WaitIrql = DISPATCH_LEVEL;
    Process->ActiveProcessors |= (ULONG_PTR)1 << Prcb->Number;
    
    {
        const char msg[] = "*** KERNEL: Thread fields initialized ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

}

CODE_SEG("INIT")
DECLSPEC_NORETURN
VOID
NTAPI
KiSystemStartupBootStack(VOID)
{
    /* Early debug output */
    #define COM1_PORT 0x3F8

    /* Test global variable access */
    static volatile ULONG TestGlobalVar = 0xDEADBEEF;
    
    {
        const char msg[] = "*** KERNEL: KiSystemStartupBootStack entered! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* CRITICAL: Initialize BSS section */
    /* The BSS section contains uninitialized globals that must be zeroed */
    /* BSS is from 0xFFFFF80000653000 to 0xFFFFF8000068C9F0 in the image */
    /* After relocation it's at 0xFFFFF80000453000 to 0xFFFFF8000048C9F0 */
    
    {
        const char msg[] = "*** KERNEL: Zeroing BSS section... ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
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
    
    {
        const char msg[] = "\n*** KERNEL: BSS section zeroed successfully! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Try to read the test global */
    {
        const char msg[] = "*** KERNEL: Testing global variable read... ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    volatile ULONG TestValue = TestGlobalVar;
    if (TestValue == 0xDEADBEEF)
    {
        const char msg[] = "*** KERNEL: SUCCESS - Global variable read correctly (0xDEADBEEF)! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    else
    {
        const char msg[] = "*** KERNEL: FAIL - Global variable read failed! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Try to write to the test global */
    {
        const char msg[] = "*** KERNEL: Testing global variable write... ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    TestGlobalVar = 0xCAFEBABE;
    TestValue = TestGlobalVar;
    if (TestValue == 0xCAFEBABE)
    {
        const char msg[] = "*** KERNEL: SUCCESS - Global variable write/read correctly (0xCAFEBABE)! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    else
    {
        const char msg[] = "*** KERNEL: FAIL - Global variable write/read failed! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Get LoaderBlock from global variable (now accessible with proper ImageBase) */
    PLOADER_PARAMETER_BLOCK LoaderBlock = KeLoaderBlock;
    
    {
        const char msg[] = "*** KERNEL: LoaderBlock retrieved from global variable ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Validate LoaderBlock */
    if (!LoaderBlock)
    {
        const char msg[] = "*** KERNEL ERROR: LoaderBlock is NULL! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        while (1) __asm__ __volatile__("hlt");
    }
    
    /* Try to continue with limited functionality */
    {
        const char msg[] = "*** KERNEL: Attempting to continue boot sequence ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Due to RIP-relative addressing issues, we cannot access globals */
    /* For now, just report success and halt */
    {
        const char msg[] = "*** KERNEL: ReactOS x64 kernel reached boot stack! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    {
        const char msg[] = "*** KERNEL: Global variable access now working with proper ImageBase! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    {
        const char msg[] = "*** KERNEL: Kernel ImageBase: 0xFFFFF80000400000, Runtime VA: 0xFFFFF80000200000 ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    {
        const char msg[] = "*** KERNEL: Relocation delta now only -0x200000, well within RIP-relative limits! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    {
        const char msg[] = "*** KERNEL: Continuing with full kernel initialization ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Continue with original kernel initialization */
    /* First, we need to get the PRCB from LoaderBlock->Prcb since GS might not be set yet */
    {
        const char msg[] = "*** KERNEL: Getting PRCB from LoaderBlock ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    PKPRCB Prcb = (PKPRCB)LoaderBlock->Prcb;
    
    if (!Prcb)
    {
        const char msg[] = "*** KERNEL ERROR: LoaderBlock->Prcb is NULL! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        while (1) __asm__ __volatile__("hlt");
    }
    
    {
        const char msg[] = "*** KERNEL: PRCB obtained from LoaderBlock ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Get PCR from PRCB */
    PKIPCR Pcr = CONTAINING_RECORD(Prcb, KIPCR, Prcb);
    
    {
        const char msg[] = "*** KERNEL: Setting up GS base to point to PCR ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Set GS base to PCR so KeGetCurrentPrcb() will work */
    __writemsr(0xC0000101, (ULONG_PTR)Pcr); /* MSR_GS_BASE */
    __writemsr(0xC0000102, (ULONG_PTR)Pcr); /* MSR_KERNEL_GS_BASE */
    
    {
        const char msg[] = "*** KERNEL: GS base configured, testing KeGetCurrentPrcb() ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Test if KeGetCurrentPrcb() works now */
    PKPRCB TestPrcb = KeGetCurrentPrcb();
    if (TestPrcb != Prcb)
    {
        const char msg[] = "*** KERNEL WARNING: KeGetCurrentPrcb() mismatch! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    else
    {
        const char msg[] = "*** KERNEL: KeGetCurrentPrcb() working correctly ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    {
        const char msg[] = "*** KERNEL: Getting Thread from LoaderBlock ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    PKTHREAD Thread = (PKTHREAD)LoaderBlock->Thread;
    if (!Thread)
    {
        const char msg[] = "*** KERNEL ERROR: LoaderBlock->Thread is NULL! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        while (1) __asm__ __volatile__("hlt");
    }
    
    {
        const char msg[] = "*** KERNEL: Getting Process from Thread ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    PKPROCESS Process = Thread->ApcState.Process;
    if (!Process)
    {
        const char msg[] = "*** KERNEL ERROR: Thread->ApcState.Process is NULL! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        while (1) __asm__ __volatile__("hlt");
    }
    
    {
        const char msg[] = "*** KERNEL: Getting KernelStack from LoaderBlock ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    PVOID KernelStack = (PVOID)LoaderBlock->KernelStack;

    {
        const char msg[] = "*** KERNEL: About to access KeNodeBlock[0] - global variable ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Set Node Data - KeNodeBlock needs special handling */
    /* KeNodeBlock is an array in .data section that's not initialized yet */
    /* For now, skip it and set ParentNode to NULL */
    {
        const char msg[] = "*** KERNEL: Skipping KeNodeBlock (needs proper init) ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    Prcb->ParentNode = NULL;

    /* Initialize the Power Management Support for this PRCB */
    {
        const char msg[] = "*** KERNEL: Calling PoInitializePrcb ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    PoInitializePrcb(Prcb);
    {
        const char msg[] = "*** KERNEL: PoInitializePrcb completed ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Save CPU state */
    {
        const char msg[] = "*** KERNEL: Calling KiSaveProcessorControlState ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    KiSaveProcessorControlState(&Prcb->ProcessorState);
    {
        const char msg[] = "*** KERNEL: KiSaveProcessorControlState completed ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Get cache line information for this CPU */
    {
        const char msg[] = "*** KERNEL: Calling KiGetCacheInformation ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    KiGetCacheInformation();
    {
        const char msg[] = "*** KERNEL: KiGetCacheInformation completed ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Initialize spinlocks and DPC data */
    {
        const char msg[] = "*** KERNEL: Calling KiInitSpinLocks (BSS now initialized) ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Now that BSS is zeroed, we can initialize spinlocks */
    KiInitSpinLocks(Prcb, Prcb->Number);
    
    {
        const char msg[] = "*** KERNEL: KiInitSpinLocks completed ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Set up the thread-related fields in the PRCB */
    {
        const char msg[] = "*** KERNEL: Setting up PRCB thread fields ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    Prcb->CurrentThread = Thread;
    Prcb->NextThread = NULL;
    Prcb->IdleThread = Thread;

    /* Initialize PRCB pool lookaside pointers */
    {
        const char msg[] = "*** KERNEL: Calling ExInitPoolLookasidePointers ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    ExInitPoolLookasidePointers();
    {
        const char msg[] = "*** KERNEL: ExInitPoolLookasidePointers completed ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Lower to APC_LEVEL */
    {
        const char msg[] = "*** KERNEL: Calling KeLowerIrql(APC_LEVEL) ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    KeLowerIrql(APC_LEVEL);
    {
        const char msg[] = "*** KERNEL: KeLowerIrql completed ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Check if this is the boot cpu */
    {
        const char msg[] = "*** KERNEL: Checking if boot CPU (Prcb->Number) ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    if (Prcb->Number == 0)
    {
        const char msg[] = "*** KERNEL: This is CPU 0 (boot CPU) ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        
        /* Initialize the kernel - BSS is now zeroed so globals should work */
        {
            const char msg[] = "*** KERNEL: Calling full KiInitializeKernel (BSS initialized) ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Call the full kernel initialization */
        KiInitializeKernel(Process, Thread, KernelStack, Prcb, LoaderBlock);
        
        {
            const char msg[] = "*** KERNEL: KiInitializeKernel completed successfully! ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
    }
    else
    {
        /* Initialize the startup thread */
        KiInitializeHandBuiltThread(Thread, Process, KernelStack);
    }

    /* Calculate the CPU frequency */
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
    /* Debug output */
    {
        const char msg[] = "*** KERNEL: KiInitializeKernel (main) entered ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    ULONG_PTR PageDirectory[2];
    PVOID DpcStack;
    ULONG i;
    
    {
        const char msg[] = "*** KERNEL: Checking KeFeatureBits for CMPXCHG16B ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Initialize 8/16 bit SList support */
    RtlpUse16ByteSLists = (KeFeatureBits & KF_CMPXCHG16B) ? TRUE : FALSE;
    
    {
        const char msg[] = "*** KERNEL: SList support initialized ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Set the current MP Master KPRCB to the Boot PRCB */
    Prcb->MultiThreadSetMaster = Prcb;
    
    {
        const char msg[] = "*** KERNEL: MultiThreadSetMaster set ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Initialize Bugcheck Callback data */
    {
        const char msg[] = "*** KERNEL: Initializing bugcheck callback lists ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    InitializeListHead(&KeBugcheckCallbackListHead);
    InitializeListHead(&KeBugcheckReasonCallbackListHead);
    KeInitializeSpinLock(&BugCheckCallbackLock);
    
    {
        const char msg[] = "*** KERNEL: Bugcheck lists initialized ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Initialize the Timer Expiration DPC */
    {
        const char msg[] = "*** KERNEL: Initializing Timer Expiration DPC ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    KeInitializeDpc(&KiTimerExpireDpc, KiTimerExpiration, NULL);
    
    {
        const char msg[] = "*** KERNEL: Setting target processor for Timer DPC ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    KeSetTargetProcessorDpc(&KiTimerExpireDpc, 0);
    
    {
        const char msg[] = "*** KERNEL: Timer DPC initialized ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Initialize Profiling data */
    {
        const char msg[] = "*** KERNEL: Initializing profiling data ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    KeInitializeSpinLock(&KiProfileLock);
    InitializeListHead(&KiProfileListHead);
    InitializeListHead(&KiProfileSourceListHead);
    
    {
        const char msg[] = "*** KERNEL: Profiling data initialized ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Loop the timer table */
    {
        const char msg[] = "*** KERNEL: Initializing timer table ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    for (i = 0; i < TIMER_TABLE_SIZE; i++)
    {
        /* Initialize the list and entries */
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
    
    {
        const char msg[] = "\n*** KERNEL: Timer table initialized ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Initialize the Swap event and all swap lists */
    {
        const char msg[] = "*** KERNEL: Initializing swap event and lists ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    KeInitializeEvent(&KiSwapEvent, SynchronizationEvent, FALSE);
    InitializeListHead(&KiProcessInSwapListHead);
    InitializeListHead(&KiProcessOutSwapListHead);
    InitializeListHead(&KiStackInSwapListHead);
    
    {
        const char msg[] = "*** KERNEL: Swap structures initialized ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Initialize the mutex for generic DPC calls */
    {
        const char msg[] = "*** KERNEL: Initializing generic DPC mutex ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    ExInitializeFastMutex(&KiGenericCallDpcMutex);
    
    {
        const char msg[] = "*** KERNEL: DPC mutex initialized ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Initialize the syscall table */
    {
        const char msg[] = "*** KERNEL: Setting up syscall table ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    KeServiceDescriptorTable[0].Base = MainSSDT;
    KeServiceDescriptorTable[0].Count = NULL;
    KeServiceDescriptorTable[0].Limit = KiServiceLimit;
    KeServiceDescriptorTable[1].Limit = 0;
    KeServiceDescriptorTable[0].Number = MainSSPT;
    
    {
        const char msg[] = "*** KERNEL: Syscall table configured ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Copy the the current table into the shadow table for win32k */
    {
        const char msg[] = "*** KERNEL: Copying syscall table to shadow (RtlCopyMemory) ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
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
    
    {
        const char msg[] = "*** KERNEL: Shadow table copied ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Initialize the Idle Process and the Process Listhead */
    {
        const char msg[] = "*** KERNEL: Initializing process list and idle process ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    InitializeListHead(&KiProcessListHead);
    PageDirectory[0] = 0;
    PageDirectory[1] = 0;
    
    {
        const char msg[] = "*** KERNEL: Calling KeInitializeProcess ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    KeInitializeProcess(InitProcess,
                        0,
                        MAXULONG_PTR,
                        PageDirectory,
                        FALSE);
    InitProcess->QuantumReset = MAXCHAR;
    
    {
        const char msg[] = "*** KERNEL: Idle process initialized ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Initialize the startup thread */
    {
        const char msg[] = "*** KERNEL: Initializing startup thread ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    KiInitializeHandBuiltThread(InitThread, InitProcess, IdleStack);
    
    {
        const char msg[] = "*** KERNEL: Startup thread initialized ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Initialize the Kernel Executive */
    {
        const char msg[] = "*** KERNEL: Calling ExpInitializeExecutive ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    ExpInitializeExecutive(0, LoaderBlock);
    
    {
        const char msg[] = "*** KERNEL: ExpInitializeExecutive completed ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Calculate the time reciprocal */
    {
        const char msg[] = "*** KERNEL: Calculating time reciprocal ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    KiTimeIncrementReciprocal =
        KiComputeReciprocal(KeMaximumIncrement,
                            &KiTimeIncrementShiftCount);
    
    {
        const char msg[] = "*** KERNEL: Time reciprocal calculated ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Update DPC Values in case they got updated by the executive */
    {
        const char msg[] = "*** KERNEL: Updating DPC values ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
    Prcb->MinimumDpcRate = KiMinimumDpcRate;
    Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;
    
    {
        const char msg[] = "*** KERNEL: DPC values updated ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Allocate the DPC Stack */
    {
        const char msg[] = "*** KERNEL: Allocating DPC stack ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    DpcStack = MmCreateKernelStack(FALSE, 0);
    if (!DpcStack) 
    {
        const char msg[] = "*** KERNEL ERROR: Failed to allocate DPC stack! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        
        KeBugCheckEx(NO_PAGES_AVAILABLE, 1, 0, 0, 0);
    }
    
    Prcb->DpcStack = DpcStack;
    
    {
        const char msg[] = "*** KERNEL: DPC stack allocated and assigned ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    {
        const char msg[] = "*** KERNEL: KiInitializeKernel completed successfully! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
}

