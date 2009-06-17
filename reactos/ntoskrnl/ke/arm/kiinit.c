/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/arm/kiinit.c
 * PURPOSE:         Implements the kernel entry point for ARM machines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

KINTERRUPT KxUnexpectedInterrupt;
BOOLEAN KeIsArmV6;
ULONG KeNumberProcessIds;
ULONG KeNumberTbEntries;
extern PVOID KiArmVectorTable;
#define __ARMV6__ KeIsArmV6

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
KiInitMachineDependent(VOID)
{
    //
    // There is nothing to do on ARM
    //
    return;
}

VOID
NTAPI
KiInitializeKernel(IN PKPROCESS InitProcess,
                   IN PKTHREAD InitThread,
                   IN PVOID IdleStack,
                   IN PKPRCB Prcb,
                   IN CCHAR Number,
                   IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    LARGE_INTEGER PageDirectory;
    PKPCR Pcr;
    ULONG i;

    //
    // Initialize the platform
    //
    HalInitializeProcessor(Number, LoaderBlock);
    
    //
    // Save loader block
    //
    KeLoaderBlock = LoaderBlock;

    //
    // Setup KPRCB
    //
    Prcb->MajorVersion = 1;
    Prcb->MinorVersion = 1;
    Prcb->BuildType = 0;
#ifndef CONFIG_SMP
    Prcb->BuildType |= PRCB_BUILD_UNIPROCESSOR;
#endif
#if DBG
    Prcb->BuildType |= PRCB_BUILD_DEBUG;
#endif
    Prcb->CurrentThread = InitThread;
    Prcb->NextThread = NULL;
    Prcb->IdleThread = InitThread;
    Prcb->Number = Number;
    Prcb->SetMember = 1 << Number;
    Prcb->PcrPage = LoaderBlock->u.Arm.PcrPage;

    //
    // Initialize spinlocks and DPC data
    //
    KiInitSpinLocks(Prcb, Number);

    //
    // Set the PRCB in the processor block
    //
    KiProcessorBlock[(ULONG)Number] = Prcb;
    Pcr = (PKPCR)KeGetPcr();

    //
    // Set processor information
    //
    KeProcessorArchitecture = PROCESSOR_ARCHITECTURE_ARM;
    KeFeatureBits = 0;
    KeProcessorLevel = (USHORT)(Pcr->ProcessorId >> 8);
    KeProcessorRevision = (USHORT)(Pcr->ProcessorId & 0xFF);

    //
    // Set stack pointers
    //
    Pcr->InitialStack = IdleStack;
    Pcr->StackLimit = (PVOID)((ULONG_PTR)IdleStack - KERNEL_STACK_SIZE);

    //
    // Check if this is the Boot CPU
    //
    if (!Number)
    {
        //
        // Setup the unexpected interrupt
        //
        KxUnexpectedInterrupt.DispatchAddress = KiUnexpectedInterrupt;
        for (i = 0; i < 4; i++)
        {
            //
            // Copy the template code
            //
            KxUnexpectedInterrupt.DispatchCode[i] = KiInterruptTemplate[i];
        }
        
        //
        // Set DMA coherency
        //
        KiDmaIoCoherency = 0;
        
        //
        // Sweep D-Cache
        //
        HalSweepDcache();
    }
    
    //
    // Set all interrupt routines to unexpected interrupts as well
    //
    for (i = 0; i < MAXIMUM_VECTOR; i++)
    {
        //
        // Point to the same template
        //
        Pcr->InterruptRoutine[i] = (PVOID)&KxUnexpectedInterrupt.DispatchCode;
    }
    
    //
    // Setup profiling
    //
    Pcr->ProfileCount = 0;
    Pcr->ProfileInterval = 0x200000;
    Pcr->StallScaleFactor = 50;
    
    //
    // Setup software interrupts
    //
    Pcr->InterruptRoutine[PASSIVE_LEVEL] = KiPassiveRelease;
    Pcr->InterruptRoutine[APC_LEVEL] = KiApcInterrupt;
    Pcr->InterruptRoutine[DISPATCH_LEVEL] = KiDispatchInterrupt;
    Pcr->ReservedVectors = (1 << PASSIVE_LEVEL) |
                           (1 << APC_LEVEL) |
                           (1 << DISPATCH_LEVEL) |
                           (1 << IPI_LEVEL);

    //
    // Set IRQL and prcessor member/number
    //
    Pcr->CurrentIrql = APC_LEVEL;
    Pcr->SetMember = 1 << Number;
    Pcr->NotMember = -Pcr->SetMember;
    Pcr->Number = Number;

    //
    // Remember our parent
    //
    InitThread->ApcState.Process = InitProcess;

    //
    // Setup the active processor numbers
    //
    KeActiveProcessors |= 1 << Number;
    KeNumberProcessors = Number + 1;

    //
    // Check if this is the boot CPU
    //
    if (!Number)
    {
        //
        // Setup KD
        //
        KdInitSystem(0, LoaderBlock);

        //
        // Cleanup the rest of the processor block array
        //
        for (i = 1; i < MAXIMUM_PROCESSORS; i++) KiProcessorBlock[i] = NULL;

        //
        // Initialize portable parts of the OS
        //
        KiInitSystem();

        //
        // Initialize the Idle Process and the Process Listhead
        //
        InitializeListHead(&KiProcessListHead);
        PageDirectory.QuadPart = 0;
        KeInitializeProcess(InitProcess,
                            0,
                            0xFFFFFFFF,
                            &PageDirectory,
                            FALSE);
        InitProcess->QuantumReset = MAXCHAR;
    }
    else
    {
        //
        // FIXME-V6: See if we want to support MP
        //
        DPRINT1("ARM MPCore not supported\n");
    }
    
    //
    // Setup the Idle Thread
    //
    KeInitializeThread(InitProcess,
                       InitThread,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       IdleStack);
    InitThread->NextProcessor = Number;
    InitThread->Priority = HIGH_PRIORITY;
    InitThread->State = Running;
    InitThread->Affinity = 1 << Number;
    InitThread->WaitIrql = DISPATCH_LEVEL;
    InitProcess->ActiveProcessors = 1 << Number;

    //
    // HACK for MmUpdatePageDir
    //
    ((PETHREAD)InitThread)->ThreadsProcess = (PEPROCESS)InitProcess;

    //
    // Initialize the Kernel Executive
    //
    ExpInitializeExecutive(Number, LoaderBlock);
    
    //
    // Only do this on the boot CPU
    //
    if (!Number)
    {
        //
        // Calculate the time reciprocal
        //
        KiTimeIncrementReciprocal =
        KiComputeReciprocal(KeMaximumIncrement,
                            &KiTimeIncrementShiftCount);
        
        //
        // Update DPC Values in case they got updated by the executive
        //
        Prcb->MaximumDpcQueueDepth = KiMaximumDpcQueueDepth;
        Prcb->MinimumDpcRate = KiMinimumDpcRate;
        Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;
    }
    
    //
    // Raise to Dispatch
    //
    KfRaiseIrql(DISPATCH_LEVEL);
    
    //
    // Set the Idle Priority to 0. This will jump into Phase 1
    //
    KeSetPriorityThread(InitThread, 0);
    
    //
    // If there's no thread scheduled, put this CPU in the Idle summary
    //
    KiAcquirePrcbLock(Prcb);
    if (!Prcb->NextThread) KiIdleSummary |= 1 << Number;
    KiReleasePrcbLock(Prcb);
    
    //
    // Raise back to HIGH_LEVEL
    //
    KfRaiseIrql(HIGH_LEVEL);
}

VOID
KiInitializeSystem(IN ULONG Magic,
                   IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ARM_PTE Pte;
    PKPCR Pcr;
    ARM_CONTROL_REGISTER ControlRegister;

    //
    // Detect ARM version (Architecture 6 is the ARMv5TE-J, go figure!)
    //
    KeIsArmV6 = KeArmIdCodeRegisterGet().Architecture == 7;
    
    //
    // Set the number of TLB entries and ASIDs
    //
    KeNumberTbEntries = 64;
    if (__ARMV6__)
    {
        //
        // 256 ASIDs on v6/v7
        //
        KeNumberProcessIds = 256;
    }
    else
    {
        //
        // The TLB is VIVT on v4/v5
        //
        KeNumberProcessIds = 0;
    }
    
    //
    // Flush the TLB
    //
    KeFlushTb();
    
    //
    // Build the KIPCR pte
    //
    Pte.L1.Section.Type = SectionPte;
    Pte.L1.Section.Buffered = FALSE;
    Pte.L1.Section.Cached = FALSE;
    Pte.L1.Section.Reserved = 1; // ARM926EJ-S manual recommends setting to 1
    Pte.L1.Section.Domain = Domain0;
    Pte.L1.Section.Access = SupervisorAccess;
    Pte.L1.Section.BaseAddress = LoaderBlock->u.Arm.PcrPage;
    Pte.L1.Section.Ignored = Pte.L1.Section.Ignored1 = 0;
    
    //
    // Map it into kernel address space by locking it into the TLB
    //
    KeFillFixedEntryTb(Pte, (PVOID)KIPCR, PCR_ENTRY);

    //
    // Now map the PCR into user address space as well (read-only)
    //
    Pte.L1.Section.Access = SharedAccess;
    KeFillFixedEntryTb(Pte, (PVOID)USPCR, PCR_ENTRY + 1);
    
    //
    // Now we should be able to use the PCR...
    //
    Pcr = (PKPCR)KeGetPcr();
    
    //
    // Set the cache policy (HACK)
    //
    Pcr->CachePolicy = 0;
    Pcr->AlignedCachePolicy = 0;
    
    //
    // Copy cache information from the loader block
    //
    Pcr->FirstLevelDcacheSize = LoaderBlock->u.Arm.FirstLevelDcacheSize;
    Pcr->SecondLevelDcacheSize = LoaderBlock->u.Arm.SecondLevelDcacheSize;
    Pcr->FirstLevelIcacheSize = LoaderBlock->u.Arm.FirstLevelIcacheSize;
    Pcr->SecondLevelIcacheSize = LoaderBlock->u.Arm.SecondLevelIcacheSize;
    Pcr->FirstLevelDcacheFillSize = LoaderBlock->u.Arm.FirstLevelDcacheFillSize;
    Pcr->SecondLevelDcacheFillSize = LoaderBlock->u.Arm.SecondLevelDcacheFillSize;
    Pcr->FirstLevelIcacheFillSize = LoaderBlock->u.Arm.FirstLevelIcacheFillSize;
    Pcr->SecondLevelIcacheFillSize = LoaderBlock->u.Arm.SecondLevelIcacheFillSize;

    //
    // Set global d-cache fill and alignment values
    //
    if (Pcr->SecondLevelDcacheSize)
    {
        //
        // Use the first level
        //
        Pcr->DcacheFillSize = Pcr->SecondLevelDcacheSize;
    }
    else
    {
        //
        // Use the second level
        //
        Pcr->DcacheFillSize = Pcr->SecondLevelDcacheSize;
    }
    
    //
    // Set the alignment
    //
    Pcr->DcacheAlignment = Pcr->DcacheFillSize - 1;
    
    //
    // Set global i-cache fill and alignment values
    //
    if (Pcr->SecondLevelIcacheSize)
    {
        //
        // Use the first level
        //
        Pcr->IcacheFillSize = Pcr->SecondLevelIcacheSize;
    }
    else
    {
        //
        // Use the second level
        //
        Pcr->IcacheFillSize = Pcr->SecondLevelIcacheSize;
    }
    
    //
    // Set the alignment
    //
    Pcr->IcacheAlignment = Pcr->IcacheFillSize - 1;
    
    //
    // Now sweep caches
    //
    HalSweepIcache();
    HalSweepDcache();
    
    //
    // Set PCR version
    //
    Pcr->MinorVersion = PCR_MINOR_VERSION;
    Pcr->MajorVersion = PCR_MAJOR_VERSION;
    
    //
    // Set boot PRCB
    //
    Pcr->Prcb = (PKPRCB)LoaderBlock->Prcb;
    
    //
    // Set the different stacks
    //
    Pcr->InitialStack = (PVOID)LoaderBlock->KernelStack;
    Pcr->PanicStack = (PVOID)LoaderBlock->u.Arm.PanicStack;
    Pcr->InterruptStack = (PVOID)LoaderBlock->u.Arm.InterruptStack;
    
    //
    // Set the current thread
    //
    Pcr->CurrentThread = (PKTHREAD)LoaderBlock->Thread;
    
    //
    // Set the current IRQL to high
    //
    Pcr->CurrentIrql = HIGH_LEVEL;
    
    //
    // Set processor information
    //
    Pcr->ProcessorId = KeArmIdCodeRegisterGet().AsUlong;
    Pcr->SystemReserved[0] = KeArmControlRegisterGet().AsUlong;
    
    //
    // Set the exception address to high
    //
    ControlRegister = KeArmControlRegisterGet();
    ControlRegister.HighVectors = TRUE;
    KeArmControlRegisterSet(ControlRegister);
    
    //
    // Setup the exception vector table
    //
    RtlCopyMemory((PVOID)0xFFFF0000, &KiArmVectorTable, 14 * sizeof(PVOID));

    //
    // Initialize the rest of the kernel now
    //
    KiInitializeKernel((PKPROCESS)LoaderBlock->Process,
                       (PKTHREAD)LoaderBlock->Thread,
                       (PVOID)LoaderBlock->KernelStack,
                       (PKPRCB)LoaderBlock->Prcb,
                       Pcr->Prcb->Number,
                       LoaderBlock);
    
    
    //
    // Jump to idle loop
    //
    KiIdleLoop();
}
