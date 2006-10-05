/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/kiinit.c
 * PURPOSE:         Kernel Initialization for x86 CPUs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NTDDI_VERSION NTDDI_WS03SP1
#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* Spinlocks used only on X86 */
KSPIN_LOCK KiFreezeExecutionLock;
KSPIN_LOCK Ki486CompatibilityLock;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KiInitMachineDependent(VOID)
{
    ULONG Protect;
    ULONG CpuCount;
    BOOLEAN FbCaching = FALSE;
    NTSTATUS Status;
    //ULONG ReturnLength;
    ULONG i, Affinity;
    PFX_SAVE_AREA FxSaveArea;
    ULONG MXCsrMask = 0xFFBF, NewMask;

    /* Check for large page support */
    if (KeFeatureBits & KF_LARGE_PAGE)
    {
        /* FIXME: Support this */
        DPRINT1("Your machine supports PGE but ReactOS doesn't yet.\n");
    }

    /* Check for global page support */
    if (KeFeatureBits & KF_GLOBAL_PAGE)
    {
        /* Do an IPI to enable it on all CPUs */
        CpuCount = KeNumberProcessors;
        KeIpiGenericCall(Ki386EnableGlobalPage, (ULONG_PTR)&CpuCount);
    }

    /* Check for PAT and/or MTRR support */
    if (KeFeatureBits & (KF_PAT | KF_MTRR))
    {
        /* FIXME: ROS HAL Doesn't initialize this! */
#if 1
        Status = STATUS_UNSUCCESSFUL;
#else
        /* Query the HAL to make sure we can use it */
        Status = HalQuerySystemInformation(HalFrameBufferCachingInformation,
                                           sizeof(BOOLEAN),
                                           &FbCaching,
                                           &ReturnLength);
#endif
        if ((NT_SUCCESS(Status)) && (FbCaching))
        {
            /* We can't, disable it */
            KeFeatureBits &= ~(KF_PAT | KF_MTRR);
        }
    }

    /* Check for PAT support and enable it */
    if (KeFeatureBits & KF_PAT) KiInitializePAT();

    /* Assume no errata for now */
    SharedUserData->ProcessorFeatures[PF_FLOATING_POINT_PRECISION_ERRATA] = 0;

    /* If there's no NPX, then we're emulating the FPU */
    SharedUserData->ProcessorFeatures[PF_FLOATING_POINT_EMULATED] =
        !KeI386NpxPresent;

    /* Check if there's no NPX, so that we can disable associated features */
    if (!KeI386NpxPresent)
    {
        /* Remove NPX-related bits */
        KeFeatureBits &= ~(KF_XMMI64 | KF_XMMI | KF_FXSR | KF_MMX);

        /* Disable kernel flags */
        KeI386FxsrPresent = KeI386XMMIPresent = FALSE;

        /* Disable processor features that might've been set until now */
        SharedUserData->ProcessorFeatures[PF_FLOATING_POINT_PRECISION_ERRATA] =
        SharedUserData->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE]   =
        SharedUserData->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE]     =
        SharedUserData->ProcessorFeatures[PF_3DNOW_INSTRUCTIONS_AVAILABLE]    =
        SharedUserData->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] = 0;
    }

    /* Check for CR4 support */
    if (KeFeatureBits & KF_CR4)
    {
        /* Do an IPI call to enable the Debug Exceptions */
        CpuCount = KeNumberProcessors;
        KeIpiGenericCall(Ki386EnableDE, (ULONG_PTR)&CpuCount);
    }

    /* Check if FXSR was found */
    if (KeFeatureBits & KF_FXSR)
    {
        /* Do an IPI call to enable the FXSR */
        CpuCount = KeNumberProcessors;
        KeIpiGenericCall(Ki386EnableFxsr, (ULONG_PTR)&CpuCount);

        /* Check if XMM was found too */
        if (KeFeatureBits & KF_XMMI)
        {
            /* Do an IPI call to enable XMMI exceptions */
            CpuCount = KeNumberProcessors;
            KeIpiGenericCall(Ki386EnableXMMIExceptions, (ULONG_PTR)&CpuCount);

            /* FIXME: Implement and enable XMM Page Zeroing for Mm */

            /* Patch the RtlPrefetchMemoryNonTemporal routine to enable it */
            Protect = MmGetPageProtect(NULL, RtlPrefetchMemoryNonTemporal);
            MmSetPageProtect(NULL,
                             RtlPrefetchMemoryNonTemporal,
                             Protect | PAGE_IS_WRITABLE);
            *(PCHAR)RtlPrefetchMemoryNonTemporal = 0x90;
            MmSetPageProtect(NULL, RtlPrefetchMemoryNonTemporal, Protect);
        }
    }

    /* Check for, and enable SYSENTER support */
    KiRestoreFastSyscallReturnState();

    /* Loop every CPU */
    i = KeActiveProcessors;
    for (Affinity = 1; i; Affinity <<= 1)
    {
        /* Check if this is part of the set */
        if (i & Affinity)
        {
            /* Run on this CPU */
            i &= ~Affinity;
            KeSetSystemAffinityThread(Affinity);

            /* Reset MHz to 0 for this CPU */
            KeGetCurrentPrcb()->MHz = 0;

            /* Check if we can use RDTSC */
            if (KeFeatureBits & KF_RDTSC)
            {
                /* Start sampling loop */
                for (;;)
                {
                    //
                    // FIXME: TODO
                    //
                    break;
                }
            }

            /* Check if we have MTRR without PAT */
            if (!(KeFeatureBits & KF_PAT) && (KeFeatureBits & KF_MTRR))
            {
                /* Then manually initialize MTRR for the CPU */
                KiInitializeMTRR((BOOLEAN)i);
            }

            /* Check if we have AMD MTRR and initialize it for the CPU */
            if (KeFeatureBits & KF_AMDK6MTRR) KiAmdK6InitializeMTRR();

            /* Check if this is a buggy Pentium and apply the fixup if so */
            if (KiI386PentiumLockErrataPresent) KiI386PentiumLockErrataFixup();

            /* Get the current thread NPX state */
            FxSaveArea = (PVOID)
                         ((ULONG_PTR)KeGetCurrentThread()->InitialStack -
                         NPX_FRAME_LENGTH);

            /* Clear initial MXCsr mask */
            FxSaveArea->U.FxArea.MXCsrMask = 0;

            /* Save the current NPX State */
#ifdef __GNUC__
            asm volatile("fxsave %0\n\t" : "=m" (*FxSaveArea));
#else
            __asm fxsave [FxSaveArea]
#endif
            /* Check if the current mask doesn't match the reserved bits */
            if (FxSaveArea->U.FxArea.MXCsrMask != MXCsrMask)
            {
                /* Then use whatever it's holding */
                MXCsrMask = FxSaveArea->U.FxArea.MXCsrMask;
            }

            /* Check if nobody set the kernel-wide mask */
            if (!KiMXCsrMask)
            {
                /* Then use the one we calculated above */
                NewMask = MXCsrMask;
            }
            else
            {
                /* Use the existing mask */
                NewMask = KiMXCsrMask;

                /* Was it set to the same value we found now? */
                if (NewMask != MXCsrMask)
                {
                    /* No, something is definitely wrong */
                    KEBUGCHECKEX(MULTIPROCESSOR_CONFIGURATION_NOT_SUPPORTED,
                                 KF_FXSR,
                                 NewMask,
                                 MXCsrMask,
                                 0);
                }
            }

            /* Now set the kernel mask */
            KiMXCsrMask = NewMask & MXCsrMask;
        }
    }

    /* Return affinity back to where it was */
    KeRevertToUserAffinityThread();

    /* NT allows limiting the duration of an ISR with a registry key */
    if (KiTimeLimitIsrMicroseconds)
    {
        /* FIXME: TODO */
        DPRINT1("ISR Time Limit not yet supported\n");
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
    Pcr->PrcbData.CurrentThread = IdleThread;

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
                   IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    BOOLEAN NpxPresent;
    ULONG FeatureBits;
    LARGE_INTEGER PageDirectory;
    PVOID DpcStack;
    ULONG NXSupportPolicy;

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

    /* Set the default NX policy (opt-in) */
    NXSupportPolicy = 2;

    /* Check if NPX is always on */
    if (strstr(KeLoaderBlock->LoadOptions, "NOEXECUTE=ALWAYSON"))
    {
        /* Set it always on */
        NXSupportPolicy = 1;
        FeatureBits |= KF_NX_ENABLED;
    }
    else if (strstr(KeLoaderBlock->LoadOptions, "NOEXECUTE=OPTOUT"))
    {
        /* Set it in opt-out mode */
        NXSupportPolicy = 3;
        FeatureBits |= KF_NX_ENABLED;
    }
    else if ((strstr(KeLoaderBlock->LoadOptions, "NOEXECUTE=OPTIN")) ||
             (strstr(KeLoaderBlock->LoadOptions, "NOEXECUTE")))
    {
        /* Set the feature bits */
        FeatureBits |= KF_NX_ENABLED;
    }
    else if ((strstr(KeLoaderBlock->LoadOptions, "NOEXECUTE=ALWAYSOFF")) ||
             (strstr(KeLoaderBlock->LoadOptions, "EXECUTE")))
    {
        /* Set disabled mode */
        NXSupportPolicy = 0;
        FeatureBits |= KF_NX_DISABLED;
    }

    /* Save feature bits */
    Prcb->FeatureBits = FeatureBits;

    /* Save CPU state */
    KiSaveProcessorControlState(&Prcb->ProcessorState);

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

        /* Lower to APC_LEVEL */
        KfLowerIrql(APC_LEVEL);

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
                            &PageDirectory,
                            FALSE);
        InitProcess->QuantumReset = MAXCHAR;
    }
    else
    {
        /* FIXME */
        DPRINT1("SMP Boot support not yet present\n");
    }

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

    /* HACK for MmUpdatePageDir */
    ((PETHREAD)InitThread)->ThreadsProcess = (PEPROCESS)InitProcess;

    /* Initialize Kernel Memory Address Space */
    MmInit1(FirstKrnlPhysAddr,
            LastKrnlPhysAddr,
            LastKernelAddress,
            (PADDRESS_RANGE)&KeMemoryMap,
            KeMemoryMapRangeCount,
            4096);

    /* Sets up the Text Sections of the Kernel and HAL for debugging */
    LdrInit1();

    /* Set the NX Support policy */
    SharedUserData->NXSupportPolicy = NXSupportPolicy;

    /* Set basic CPU Features that user mode can read */
    SharedUserData->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] =
        (KeFeatureBits & KF_MMX);
    SharedUserData->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE] =
        (KeFeatureBits & KF_CMPXCHG8B);
    SharedUserData->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE] =
        ((KeFeatureBits & KF_FXSR) && (KeFeatureBits & KF_XMMI));
    SharedUserData->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE] =
        ((KeFeatureBits & KF_FXSR) && (KeFeatureBits & KF_XMMI64));
    SharedUserData->ProcessorFeatures[PF_3DNOW_INSTRUCTIONS_AVAILABLE] =
        (KeFeatureBits & KF_3DNOW);
    SharedUserData->ProcessorFeatures[PF_RDTSC_INSTRUCTION_AVAILABLE] =
        (KeFeatureBits & KF_RDTSC);

    /* Set up the thread-related fields in the PRCB */
    Prcb->CurrentThread = InitThread;
    Prcb->NextThread = NULL;
    Prcb->IdleThread = InitThread;

    /* Initialize the Kernel Executive */
    ExpInitializeExecutive(Number, LoaderBlock);

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

    /* Raise to Dispatch */
    KfRaiseIrql(DISPATCH_LEVEL);

    /* Set the Idle Priority to 0. This will jump into Phase 1 */
    KeSetPriorityThread(InitThread, 0);

    /* If there's no thread scheduled, put this CPU in the Idle summary */
    if (!Prcb->NextThread) KiIdleSummary |= 1 << Number;

    /* Raise back to HIGH_LEVEL and clear the PRCB for the loader block */
    KfRaiseIrql(HIGH_LEVEL);
    LoaderBlock->Prcb = 0;
}

VOID
FASTCALL
KiGetMachineBootPointers(IN PKGDTENTRY *Gdt,
                         IN PKIDTENTRY *Idt,
                         IN PKIPCR *Pcr,
                         IN PKTSS *Tss)
{
    KDESCRIPTOR GdtDescriptor, IdtDescriptor;
    KGDTENTRY TssSelector, PcrSelector;
    ULONG Tr, Fs;

    /* Get GDT and IDT descriptors */
    Ke386GetGlobalDescriptorTable(GdtDescriptor);
    Ke386GetInterruptDescriptorTable(IdtDescriptor);

    /* Save IDT and GDT */
    *Gdt = (PKGDTENTRY)GdtDescriptor.Base;
    *Idt = (PKIDTENTRY)IdtDescriptor.Base;

    /* Get TSS and FS Selectors */
    Ke386GetTr(&Tr);
    if (Tr != KGDT_TSS) Tr = KGDT_TSS; // FIXME: HACKHACK
    Fs = Ke386GetFs();

    /* Get PCR Selector, mask it and get its GDT Entry */
    PcrSelector = *(PKGDTENTRY)((ULONG_PTR)*Gdt + (Fs & ~RPL_MASK));

    /* Get the KPCR itself */
    *Pcr = (PKIPCR)(ULONG_PTR)(PcrSelector.BaseLow |
                               PcrSelector.HighWord.Bytes.BaseMid << 16 |
                               PcrSelector.HighWord.Bytes.BaseHi << 24);

    /* Get TSS Selector, mask it and get its GDT Entry */
    TssSelector = *(PKGDTENTRY)((ULONG_PTR)*Gdt + (Tr & ~RPL_MASK));

    /* Get the KTSS itself */
    *Tss = (PKTSS)(ULONG_PTR)(TssSelector.BaseLow |
                              TssSelector.HighWord.Bytes.BaseMid << 16 |
                              TssSelector.HighWord.Bytes.BaseHi << 24);
}

VOID
NTAPI
KiSystemStartup(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG Cpu;
    PKTHREAD InitialThread;
    ULONG InitialStack;
    PKGDTENTRY Gdt;
    PKIDTENTRY Idt;
    PKTSS Tss;
    PKIPCR Pcr;

    /* Save the loader block and get the current CPU */
    KeLoaderBlock = LoaderBlock;
    Cpu = KeNumberProcessors;
    if (!Cpu)
    {
        /* If this is the boot CPU, set FS and the CPU Number*/
        Ke386SetFs(KGDT_R0_PCR);
        __writefsdword(KPCR_PROCESSOR_NUMBER, Cpu);

        /* Set the initial stack and idle thread as well */
        LoaderBlock->KernelStack = (ULONG_PTR)P0BootStack;
        LoaderBlock->Thread = (ULONG_PTR)&KiInitialThread;
    }

    /* Save the initial thread and stack */
    InitialStack = LoaderBlock->KernelStack;
    InitialThread = (PKTHREAD)LoaderBlock->Thread;

    /* Clean the APC List Head */
    InitializeListHead(&InitialThread->ApcState.ApcListHead[KernelMode]);

    /* Initialize the machine type */
    KiInitializeMachineType();

    /* Skip initial setup if this isn't the Boot CPU */
    if (Cpu) goto AppCpuInit;

    /* Get GDT, IDT, PCR and TSS pointers */
    KiGetMachineBootPointers(&Gdt, &Idt, &Pcr, &Tss);

    /* Setup the TSS descriptors and entries */
    Ki386InitializeTss(Tss, Idt, Gdt);

    /* Initialize the PCR */
    RtlZeroMemory(Pcr, PAGE_SIZE);
    KiInitializePcr(Cpu,
                    Pcr,
                    Idt,
                    Gdt,
                    Tss,
                    InitialThread,
                    KiDoubleFaultStack);

    /* Set us as the current process */
    InitialThread->ApcState.Process = &KiInitialProcess.Pcb;

    /* Clear DR6/7 to cleanup bootloader debugging */
    __writefsdword(KPCR_TEB, 0);
    __writefsdword(KPCR_DR6, 0);
    __writefsdword(KPCR_DR7, 0);

    /* Load Ring 3 selectors for DS/ES */
    Ke386SetDs(KGDT_R3_DATA | RPL_MASK);
    Ke386SetEs(KGDT_R3_DATA | RPL_MASK);

AppCpuInit:
    /* Loop until we can release the freeze lock */
    do
    {
        /* Loop until execution can continue */
        while ((volatile KSPIN_LOCK)KiFreezeExecutionLock == 1);
    } while(InterlockedBitTestAndSet((PLONG)&KiFreezeExecutionLock, 0));

    /* Setup CPU-related fields */
    __writefsdword(KPCR_NUMBER, Cpu);
    __writefsdword(KPCR_SET_MEMBER, 1 << Cpu);
    __writefsdword(KPCR_SET_MEMBER_COPY, 1 << Cpu);
    __writefsdword(KPCR_PRCB_SET_MEMBER, 1 << Cpu);

    /* Initialize the Processor with HAL */
    HalInitializeProcessor(Cpu, KeLoaderBlock);

    /* Set active processors */
    KeActiveProcessors |= __readfsdword(KPCR_SET_MEMBER);
    KeNumberProcessors++;

    /* Check if this is the boot CPU */
    if (!Cpu)
    {
        /* Initialize debugging system */
        KdInitSystem(0, KeLoaderBlock);

        /* Check for break-in */
        if (KdPollBreakIn()) DbgBreakPointWithStatus(1);
    }

    /* Raise to HIGH_LEVEL */
    KfRaiseIrql(HIGH_LEVEL);

    /* Align stack and make space for the trap frame and NPX frame */
    InitialStack &= ~KTRAP_FRAME_ALIGN;
#ifdef __GNUC__
    __asm__ __volatile__("movl %0,%%esp" : :"r" (InitialStack));
    __asm__ __volatile__("subl %0,%%esp" : :"r" (NPX_FRAME_LENGTH +
                                                 KTRAP_FRAME_LENGTH +
                                                 KTRAP_FRAME_ALIGN));
    __asm__ __volatile__("push %0" : :"r" (CR0_EM + CR0_TS + CR0_MP));
#else
    __asm mov esp, InitialStack;
    __asm sub esp, NPX_FRAME_LENGTH + KTRAP_FRAME_ALIGN + KTRAP_FRAME_LENGTH;
    __asm push CR0_EM + CR0_TS + CR0_MP
#endif

    /* Call main kernel initialization */
    KiInitializeKernel(&KiInitialProcess.Pcb,
                       InitialThread,
                       (PVOID)InitialStack,
                       (PKPRCB)__readfsdword(KPCR_PRCB),
                       Cpu,
                       KeLoaderBlock);

    /* Set the priority of this thread to 0 */
    KeGetCurrentThread()->Priority = 0;

    /* Force interrupts enabled and lower IRQL back to DISPATCH_LEVEL */
    _enable();
    KfLowerIrql(DISPATCH_LEVEL);

    /* Set the right wait IRQL */
    KeGetCurrentThread()->WaitIrql = DISPATCH_LEVEL;

    /* Set idle thread as running on UP builds */
#ifndef CONFIG_SMP
    KeGetCurrentThread()->State = Running;
#endif

    /* Jump into the idle loop */
    KiIdleLoop();
}

