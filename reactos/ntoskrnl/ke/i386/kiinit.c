/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/kiinit.c
 * PURPOSE:         Kernel Initialization for x86 CPUs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include <intrin.h>

/* GLOBALS *******************************************************************/

/* Spinlocks used only on X86 */
KSPIN_LOCK KiFreezeExecutionLock;
KSPIN_LOCK Ki486CompatibilityLock;

/* FUNCTIONS *****************************************************************/

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

    /* Set up the thread-related fields in the PRCB */
    Prcb->CurrentThread = InitThread;
    Prcb->NextThread = NULL;
    Prcb->IdleThread = InitThread;

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
    PVOID InitialStack;
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
        KeGetPcr()->Number = Cpu;

        /* Set the initial stack and idle thread as well */
        LoaderBlock->KernelStack = (ULONG_PTR)P0BootStack;
        LoaderBlock->Thread = (ULONG_PTR)&KiInitialThread;
    }

    /* Save the initial thread and stack */
    InitialStack = (PVOID)LoaderBlock->KernelStack;
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
    Ki386InitializeTss(Tss, Idt);

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
        while (KiFreezeExecutionLock == 1);
    } while(InterlockedBitTestAndSet(&KiFreezeExecutionLock, 0));

    /* Setup CPU-related fields */
    __writefsdword(KPCR_NUMBER, Cpu);
    __writefsdword(KPCR_SET_MEMBER, 1 << Cpu);
    __writefsdword(KPCR_SET_MEMBER_COPY, 1 << Cpu);
    __writefsdword(KPCR_PRCB_SET_MEMBER, 1 << Cpu);

    /* Initialize the Processor with HAL */
    HalInitializeProcessor(Cpu, KeLoaderBlock);

    /* Set active processors */
    KeActiveProcessors |= Pcr->SetMember;
    KeNumberProcessors++;

    /* Check if this is the boot CPU */
    if (!Cpu)
    {
        /* Initialize debugging system */
        KdInitSystem (0, KeLoaderBlock);

        /* Check for break-in */
        if (KdPollBreakIn()) DbgBreakPointWithStatus(1);
    }

    /* Raise to HIGH_LEVEL */
    KfRaiseIrql(HIGH_LEVEL);

    /* Call main kernel initialization */
    KiInitializeKernel(&KiInitialProcess.Pcb,
                       InitialThread,
                       InitialStack,
                       (PKPRCB)__readfsdword(KPCR_PRCB),
                       Cpu,
                       LoaderBlock);

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

