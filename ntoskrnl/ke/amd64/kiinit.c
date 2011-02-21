/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/kiinit.c
 * PURPOSE:         Kernel Initialization for x86 CPUs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define REQUIRED_FEATURE_BITS (KF_RDTSC|KF_CR4|KF_CMPXCHG8B|KF_XMMI|KF_XMMI64| \
                               KF_NX_BIT)

/* GLOBALS *******************************************************************/

/* Function pointer for early debug prints */
ULONG (*FrLdrDbgPrint)(const char *Format, ...);

/* Spinlocks used only on X86 */
KSPIN_LOCK KiFreezeExecutionLock;

/* BIOS Memory Map. Not NTLDR-compliant yet */
extern ULONG KeMemoryMapRangeCount;
extern ADDRESS_RANGE KeMemoryMap[64];

KIPCR KiInitialPcr;

/* Boot and double-fault/NMI/DPC stack */
UCHAR DECLSPEC_ALIGN(16) P0BootStackData[KERNEL_STACK_SIZE] = {0};
UCHAR DECLSPEC_ALIGN(16) KiDoubleFaultStackData[KERNEL_STACK_SIZE] = {0};
ULONG_PTR P0BootStack = (ULONG_PTR)&P0BootStackData[KERNEL_STACK_SIZE];
ULONG_PTR KiDoubleFaultStack = (ULONG_PTR)&KiDoubleFaultStackData[KERNEL_STACK_SIZE];

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KiInitMachineDependent(VOID)
{
    /* Check for large page support */
    if (KeFeatureBits & KF_LARGE_PAGE)
    {
        /* FIXME: Support this */
        DPRINT1("Large Page support detected but not yet taken advantage of!\n");
    }

    /* Check for global page support */
    if (KeFeatureBits & KF_GLOBAL_PAGE)
    {
        /* FIXME: Support this */
        DPRINT1("Global Page support detected but not yet taken advantage of!\n");
    }

    /* Check if we have MTRR */
    if (KeFeatureBits & KF_MTRR)
    {
        /* FIXME: Support this */
        DPRINT1("MTRR support detected but not yet taken advantage of!\n");
    }

    /* Check for PAT and/or MTRR support */
    if (KeFeatureBits & KF_PAT)
    {
        /* FIXME: Support this */
        DPRINT1("PAT support detected but not yet taken advantage of!\n");
    }


}

VOID
NTAPI
KiInitializePcr(IN PKIPCR Pcr,
                IN ULONG ProcessorNumber,
                IN PKTHREAD IdleThread,
                IN PVOID DpcStack)
{
    KDESCRIPTOR GdtDescriptor = {{0},0,0}, IdtDescriptor = {{0},0,0};
    PKGDTENTRY64 TssEntry;
    USHORT Tr = 0;

    /* Zero out the PCR */
    RtlZeroMemory(Pcr, PAGE_SIZE);

    /* Set pointers to ourselves */
    Pcr->Self = (PKPCR)Pcr;
    Pcr->CurrentPrcb = &Pcr->Prcb;

    /* Set the PCR Version */
    Pcr->MajorVersion = PCR_MAJOR_VERSION;
    Pcr->MinorVersion = PCR_MINOR_VERSION;

    /* Set the PRCB Version */
    Pcr->Prcb.MajorVersion = 1;
    Pcr->Prcb.MinorVersion = 1;

    /* Set the Build Type */
    Pcr->Prcb.BuildType = 0;
#ifndef CONFIG_SMP
    Pcr->Prcb.BuildType |= PRCB_BUILD_UNIPROCESSOR;
#endif
#ifdef DBG
    Pcr->Prcb.BuildType |= PRCB_BUILD_DEBUG;
#endif

    /* Set the Processor Number and current Processor Mask */
    Pcr->Prcb.Number = (UCHAR)ProcessorNumber;
    Pcr->Prcb.SetMember = 1 << ProcessorNumber;

    /* Get GDT and IDT descriptors */
    __sgdt(&GdtDescriptor.Limit);
    __sidt(&IdtDescriptor.Limit);
    Pcr->GdtBase = (PVOID)GdtDescriptor.Base;
    Pcr->IdtBase = (PKIDTENTRY)IdtDescriptor.Base;

    /* Get TSS Selector */
    __str(&Tr);
    ASSERT(Tr == KGDT64_SYS_TSS);

    /* Get TSS Entry */
    TssEntry = KiGetGdtEntry(Pcr->GdtBase, Tr);

    /* Get the KTSS itself */
    Pcr->TssBase = KiGetGdtDescriptorBase(TssEntry);

    Pcr->Prcb.RspBase = Pcr->TssBase->Rsp0; // FIXME

    /* Set DPC Stack */
    Pcr->Prcb.DpcStack = DpcStack;

    /* Setup the processor set */
    Pcr->Prcb.MultiThreadProcessorSet = Pcr->Prcb.SetMember;

    /* Clear DR6/7 to cleanup bootloader debugging */
    Pcr->Prcb.ProcessorState.SpecialRegisters.KernelDr6 = 0;
    Pcr->Prcb.ProcessorState.SpecialRegisters.KernelDr7 = 0;

    /* Set the Current Thread */
    Pcr->Prcb.CurrentThread = IdleThread;

    /* Start us out at PASSIVE_LEVEL */
    Pcr->Irql = PASSIVE_LEVEL;
    KeSetCurrentIrql(PASSIVE_LEVEL);

}

VOID
NTAPI
KiInitializeCpuFeatures(ULONG Cpu)
{
    ULONG FeatureBits;

    /* Get the processor features for this CPU */
    FeatureBits = KiGetFeatureBits();

    /* Check if we support all needed features */
    if ((FeatureBits & REQUIRED_FEATURE_BITS) != REQUIRED_FEATURE_BITS)
    {
        /* If not, bugcheck system */
        FrLdrDbgPrint("CPU doesn't have needed features! Has: 0x%x, required: 0x%x\n",
                FeatureBits, REQUIRED_FEATURE_BITS);
        KeBugCheck(0);
    }

    /* Set DEP to always on */
    SharedUserData->NXSupportPolicy = NX_SUPPORT_POLICY_ALWAYSON;
    FeatureBits |= KF_NX_ENABLED;

    /* Save feature bits */
    KeGetCurrentPrcb()->FeatureBits = FeatureBits;

    /* Enable fx save restore support */
    __writecr4(__readcr4() | CR4_FXSR);

    /* Enable XMMI exceptions */
    __writecr4(__readcr4() | CR4_XMMEXCPT);

    /* Enable Write-Protection */
    __writecr0(__readcr0() | CR0_WP);

    /* Disable fpu monitoring */
    __writecr0(__readcr0() & ~CR0_MP);

    /* Disable x87 fpu exceptions */
    __writecr0(__readcr0() & ~CR0_NE);
    
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
    ULONG PageDirectory[2];
    PVOID DpcStack;

    /* Detect and set the CPU Type */
    KiSetProcessorType();

    /* Initialize the Power Management Support for this PRCB */
//    PoInitializePrcb(Prcb);

    /* Save CPU state */
    KiSaveProcessorControlState(&Prcb->ProcessorState);

    /* Get cache line information for this CPU */
    KiGetCacheInformation();

    /* Initialize spinlocks and DPC data */
    KiInitSpinLocks(Prcb, Number);

    /* Check if this is the Boot CPU */
    if (Number == 0)
    {
        /* Set Node Data */
        KeNodeBlock[0] = &KiNode0;
        Prcb->ParentNode = KeNodeBlock[0];
        KeNodeBlock[0]->ProcessorMask = Prcb->SetMember;

        /* Set boot-level flags */
        KeProcessorArchitecture = PROCESSOR_ARCHITECTURE_AMD64;
        KeProcessorLevel = (USHORT)Prcb->CpuType;
        if (Prcb->CpuID) KeProcessorRevision = Prcb->CpuStep;

        /* Set the current MP Master KPRCB to the Boot PRCB */
        Prcb->MultiThreadSetMaster = Prcb;

        /* Lower to APC_LEVEL */
        KeLowerIrql(APC_LEVEL);

        /* Initialize some spinlocks */
        KeInitializeSpinLock(&KiFreezeExecutionLock);

        /* Initialize portable parts of the OS */
        KiInitSystem();

        /* Initialize the Idle Process and the Process Listhead */
        InitializeListHead(&KiProcessListHead);
        PageDirectory[0] = 0;
        PageDirectory[1] = 0;
        KeInitializeProcess(InitProcess,
                            0,
                            0xFFFFFFFF,
                            PageDirectory,
                            FALSE);
        InitProcess->QuantumReset = MAXCHAR;
    }
    else
    {
        /* FIXME */
        DPRINT1("SMP Boot support not yet present\n");
    }

    /* HACK for MmUpdatePageDir */
    ((PETHREAD)InitThread)->ThreadsProcess = (PEPROCESS)InitProcess;

    /* Setup the Idle Thread */
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

    /* Set basic CPU Features that user mode can read */
    SharedUserData->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] =
        (KeFeatureBits & KF_MMX) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE] =
        (KeFeatureBits & KF_CMPXCHG8B) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE] =
        ((KeFeatureBits & KF_FXSR) && (KeFeatureBits & KF_XMMI)) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE] =
        ((KeFeatureBits & KF_FXSR) && (KeFeatureBits & KF_XMMI64)) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_3DNOW_INSTRUCTIONS_AVAILABLE] =
        (KeFeatureBits & KF_3DNOW) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_RDTSC_INSTRUCTION_AVAILABLE] =
        (KeFeatureBits & KF_RDTSC) ? TRUE: FALSE;

    /* Set up the thread-related fields in the PRCB */
    Prcb->CurrentThread = InitThread;
    Prcb->NextThread = NULL;
    Prcb->IdleThread = InitThread;

    /* Initialize the Kernel Executive */
    ExpInitializeExecutive(Number, LoaderBlock);

    /* Only do this on the boot CPU */
    if (Number == 0)
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
        DpcStack = MmCreateKernelStack(FALSE, 0);
        if (!DpcStack) KeBugCheckEx(NO_PAGES_AVAILABLE, 1, 0, 0, 0);
        Prcb->DpcStack = DpcStack;

        /* Allocate the IOPM save area. */
//        Ki386IopmSaveArea = ExAllocatePoolWithTag(PagedPool,
//                                                  PAGE_SIZE * 2,
//                                                  TAG('K', 'e', ' ', ' '));
//        if (!Ki386IopmSaveArea)
//        {
//            /* Bugcheck. We need this for V86/VDM support. */
//            KeBugCheckEx(NO_PAGES_AVAILABLE, 2, PAGE_SIZE * 2, 0, 0);
//        }
    }

    /* Raise to Dispatch */
    KfRaiseIrql(DISPATCH_LEVEL);

    /* Set the Idle Priority to 0. This will jump into Phase 1 */
    KeSetPriorityThread(InitThread, 0);

    /* If there's no thread scheduled, put this CPU in the Idle summary */
    KiAcquirePrcbLock(Prcb);
    if (!Prcb->NextThread) KiIdleSummary |= 1 << Number;
    KiReleasePrcbLock(Prcb);

    /* Raise back to HIGH_LEVEL and clear the PRCB for the loader block */
    KfRaiseIrql(HIGH_LEVEL);
    LoaderBlock->Prcb = 0;
}

VOID
NTAPI
KiSystemStartup(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG Cpu;
    PKTHREAD InitialThread;
    ULONG64 InitialStack;
    PKIPCR Pcr;

    /* HACK */
    FrLdrDbgPrint = LoaderBlock->u.I386.CommonDataArea;
    FrLdrDbgPrint("Hello from KiSystemStartup!!!\n");

    /* HACK, because freeldr maps page 0 */
    MiAddressToPte((PVOID)0)->u.Hard.Valid = 0;

    /* Save the loader block */
    KeLoaderBlock = LoaderBlock;

    /* Get the current CPU number */
    Cpu = KeNumberProcessors++; // FIXME

    /* LoaderBlock initialization for Cpu 0 */
    if (Cpu == 0)
    {
        /* Set the initial stack, idle thread and process */
        LoaderBlock->KernelStack = (ULONG_PTR)P0BootStack;
        LoaderBlock->Thread = (ULONG_PTR)&KiInitialThread;
        LoaderBlock->Process = (ULONG_PTR)&KiInitialProcess.Pcb;
        LoaderBlock->Prcb = (ULONG_PTR)&KiInitialPcr.Prcb;
    }

    /* Get Pcr from loader block */
    Pcr = CONTAINING_RECORD(LoaderBlock->Prcb, KIPCR, Prcb);

    /* Set the PRCB for this Processor */
    KiProcessorBlock[Cpu] = &Pcr->Prcb;

    /* Set GS base */
    __writemsr(X86_MSR_GSBASE, (ULONG64)Pcr);
    __writemsr(X86_MSR_KERNEL_GSBASE, (ULONG64)Pcr);

    /* LDT is unused */
    __lldt(0);

    /* Align stack to 16 bytes */
    LoaderBlock->KernelStack &= ~(16 - 1);

    /* Save the initial thread and stack */
    InitialStack = LoaderBlock->KernelStack; // Checkme
    InitialThread = (PKTHREAD)LoaderBlock->Thread;

    /* Clean the APC List Head */
    InitializeListHead(&InitialThread->ApcState.ApcListHead[KernelMode]);

    /* Set us as the current process */
    InitialThread->ApcState.Process = (PVOID)LoaderBlock->Process;

    /* Initialize the PCR */
    KiInitializePcr(Pcr, Cpu, InitialThread, (PVOID)KiDoubleFaultStack);

    /* Initialize the CPU features */
    KiInitializeCpuFeatures(Cpu);

    /* Initial setup for the boot CPU */
    if (Cpu == 0)
    {
        /* Setup the TSS descriptors and entries */
        KiInitializeTss(Pcr->TssBase, InitialStack);

        /* Setup the IDT */
        KeInitExceptions();

        /* HACK: misuse this function to pass a function pointer to kdcom */
        KdDebuggerInitialize1((PVOID)FrLdrDbgPrint);

        /* Initialize debugging system */
        KdInitSystem(0, KeLoaderBlock);

        /* Check for break-in */
        if (KdPollBreakIn()) DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);

        /* Hack! Wait for the debugger! */
#ifdef _WINKD_
        while (!KdPollBreakIn());
        DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);
#endif
    }

    DPRINT("Pcr = %p, Gdt = %p, Idt = %p, Tss = %p\n",
           Pcr, Pcr->GdtBase, Pcr->IdtBase, Pcr->TssBase);

    /* Acquire lock */
    while (InterlockedBitTestAndSet64((PLONG64)&KiFreezeExecutionLock, 0))
    {
        /* Loop until lock is free */
        while ((*(volatile KSPIN_LOCK*)&KiFreezeExecutionLock) & 1);
    } 

    /* Initialize the Processor with HAL */
    HalInitializeProcessor(Cpu, KeLoaderBlock);

    /* Set processor as active */
    KeActiveProcessors |= 1 << Cpu;

    /* Release lock */
    InterlockedAnd64((PLONG64)&KiFreezeExecutionLock, 0);

    /* Raise to HIGH_LEVEL */
    KfRaiseIrql(HIGH_LEVEL);

    /* Switch to new kernel stack and start kernel bootstrapping */
    KiSetupStackAndInitializeKernel(&KiInitialProcess.Pcb,
                                    InitialThread,
                                    (PVOID)InitialStack,
                                    &Pcr->Prcb,
                                    (CCHAR)Cpu,
                                    KeLoaderBlock);
}


VOID
NTAPI
KiInitializeKernelAndGotoIdleLoop(IN PKPROCESS InitProcess,
                                  IN PKTHREAD InitThread,
                                  IN PVOID IdleStack,
                                  IN PKPRCB Prcb,
                                  IN CCHAR Number,
                                  IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
//    DbgBreakPointWithStatus(0);

    /* Initialize kernel */
    KiInitializeKernel(InitProcess,
                       InitThread,
                       IdleStack,
                       Prcb,
                       Number,
                       KeLoaderBlock);

    /* Set the priority of this thread to 0 */
    InitThread->Priority = 0;

    /* Force interrupts enabled and lower IRQL back to DISPATCH_LEVEL */
    _enable();
    KeLowerIrql(DISPATCH_LEVEL);

    /* Set the right wait IRQL */
    InitThread->WaitIrql = DISPATCH_LEVEL;

    /* Jump into the idle loop */
    KiIdleLoop();
}
