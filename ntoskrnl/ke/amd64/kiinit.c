/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/amd64/kiinit.c
 * PURPOSE:         Kernel Initialization for x86 CPUs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define REQUIRED_FEATURE_BITS (KF_RDTSC|KF_CR4|KF_CMPXCHG8B|KF_XMMI|KF_XMMI64| \
                               KF_LARGE_PAGE|KF_FAST_SYSCALL|KF_GLOBAL_PAGE| \
                               KF_CMOV|KF_PAT|KF_MMX|KF_FXSR|KF_NX_BIT|KF_MTRR)

/* GLOBALS *******************************************************************/

/* Function pointer for early debug prints */
ULONG (*FrLdrDbgPrint)(const char *Format, ...);

/* Spinlocks used only on X86 */
KSPIN_LOCK KiFreezeExecutionLock;


KIPCR KiInitialPcr;

/* Boot and double-fault/NMI/DPC stack */
UCHAR DECLSPEC_ALIGN(16) P0BootStackData[KERNEL_STACK_SIZE] = {0};
UCHAR DECLSPEC_ALIGN(16) KiDoubleFaultStackData[KERNEL_STACK_SIZE] = {0};
ULONG_PTR P0BootStack = (ULONG_PTR)&P0BootStackData[KERNEL_STACK_SIZE];
ULONG_PTR KiDoubleFaultStack = (ULONG_PTR)&KiDoubleFaultStackData[KERNEL_STACK_SIZE];

void KiInitializeSegments();
void KiSystemCallEntry64();
void KiSystemCallEntry32();

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
KiInitMachineDependent(VOID)
{
    /* Check for large page support */
    if (KeFeatureBits & KF_LARGE_PAGE)
    {
        /* FIXME: Support this */
        DPRINT("Large Page support detected but not yet taken advantage of!\n");
    }

    /* Check for global page support */
    if (KeFeatureBits & KF_GLOBAL_PAGE)
    {
        /* FIXME: Support this */
        DPRINT("Global Page support detected but not yet taken advantage of!\n");
    }

    /* Check if we have MTRR */
    if (KeFeatureBits & KF_MTRR)
    {
        /* FIXME: Support this */
        DPRINT("MTRR support detected but not yet taken advantage of!\n");
    }

    /* Check for PAT and/or MTRR support */
    if (KeFeatureBits & KF_PAT)
    {
        /* FIXME: Support this */
        DPRINT("PAT support detected but not yet taken advantage of!\n");
    }

//        /* Allocate the IOPM save area */
//        Ki386IopmSaveArea = ExAllocatePoolWithTag(PagedPool,
//                                                  IOPM_SIZE,
//                                                  '  eK');
//        if (!Ki386IopmSaveArea)
//        {
//            /* Bugcheck. We need this for V86/VDM support. */
//            KeBugCheckEx(NO_PAGES_AVAILABLE, 2, IOPM_SIZE, 0, 0);
//        }

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
    RtlZeroMemory(Pcr, sizeof(KIPCR));

    /* Set pointers to ourselves */
    Pcr->Self = (PKPCR)Pcr;
    Pcr->CurrentPrcb = &Pcr->Prcb;

    /* Set the PCR Version */
    Pcr->MajorVersion = PCR_MAJOR_VERSION;
    Pcr->MinorVersion = PCR_MINOR_VERSION;

    /* Set the PRCB Version */
    Pcr->Prcb.MajorVersion = PRCB_MAJOR_VERSION;
    Pcr->Prcb.MinorVersion = PRCB_MINOR_VERSION;

    /* Set the Build Type */
    Pcr->Prcb.BuildType = 0;
#ifndef CONFIG_SMP
    Pcr->Prcb.BuildType |= PRCB_BUILD_UNIPROCESSOR;
#endif
#if DBG
    Pcr->Prcb.BuildType |= PRCB_BUILD_DEBUG;
#endif

    /* Set the Processor Number and current Processor Mask */
    Pcr->Prcb.Number = (UCHAR)ProcessorNumber;
    Pcr->Prcb.SetMember = 1ULL << ProcessorNumber;

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
KiInitializeCpu(PKIPCR Pcr)
{
    ULONG64 Pat;
    ULONG FeatureBits;

    /* Initialize gs */
    KiInitializeSegments();

    /* Set GS base */
    __writemsr(MSR_GS_BASE, (ULONG64)Pcr);
    __writemsr(MSR_GS_SWAP, (ULONG64)Pcr);

    /* Detect and set the CPU Type */
    KiSetProcessorType();

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
    Pcr->Prcb.FeatureBits = FeatureBits;

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

    /* LDT is unused */
    __lldt(0);

    /* Set the systemcall entry points */
    __writemsr(MSR_LSTAR, (ULONG64)KiSystemCallEntry64);
    __writemsr(MSR_CSTAR, (ULONG64)KiSystemCallEntry32);

    __writemsr(MSR_STAR, ((ULONG64)KGDT64_R0_CODE << 32) |
                         ((ULONG64)(KGDT64_R3_CMCODE|RPL_MASK) << 48));

    /* Set the flags to be cleared when doing a syscall */
    __writemsr(MSR_SYSCALL_MASK, EFLAGS_IF_MASK | EFLAGS_TF | EFLAGS_DF);

    /* Enable syscall instruction and no-execute support */
    __writemsr(MSR_EFER, __readmsr(MSR_EFER) | MSR_SCE | MSR_NXE);

    /* Initialize the PAT */
    Pat = (PAT_WB << 0)  | (PAT_WC << 8) | (PAT_UCM << 16) | (PAT_UC << 24) |
          (PAT_WB << 32) | (PAT_WC << 40) | (PAT_UCM << 48) | (PAT_UC << 56);
    __writemsr(MSR_PAT, Pat);
}

VOID
FASTCALL
KiInitializeTss(IN PKTSS64 Tss,
                IN UINT64 Stack)
{
    PKGDTENTRY64 TssEntry;

    /* Get pointer to the GDT entry */
    TssEntry = KiGetGdtEntry(KeGetPcr()->GdtBase, KGDT64_SYS_TSS);

    /* Initialize the GDT entry */
    KiInitGdtEntry(TssEntry, (ULONG64)Tss, sizeof(KTSS64), AMD64_TSS, 0);

    /* Zero out the TSS */
    RtlZeroMemory(Tss, sizeof(KTSS64));

    /* FIXME: I/O Map? */
    Tss->IoMapBase = 0x68;

    /* Setup ring 0 stack pointer */
    Tss->Rsp0 = Stack;

    /* Setup a stack for Double Fault Traps */
    Tss->Ist[1] = (ULONG64)KiDoubleFaultStack;

    /* Setup a stack for CheckAbort Traps */
    Tss->Ist[2] = (ULONG64)KiDoubleFaultStack;

    /* Setup a stack for NMI Traps */
    Tss->Ist[3] = (ULONG64)KiDoubleFaultStack;

    /* Load the task register */
    __ltr(KGDT64_SYS_TSS);
}

CODE_SEG("INIT")
VOID
NTAPI
KiInitializeKernelMachineDependent(
    IN PKPRCB Prcb,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Set boot-level flags */
    KeI386CpuType = Prcb->CpuType;
    KeI386CpuStep = Prcb->CpuStep;
    KeProcessorArchitecture = PROCESSOR_ARCHITECTURE_AMD64;
    KeProcessorLevel = (USHORT)Prcb->CpuType;
    if (Prcb->CpuID)
        KeProcessorRevision = Prcb->CpuStep;

    /* Set basic CPU Features that user mode can read */
    SharedUserData->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE] = TRUE;
    SharedUserData->ProcessorFeatures[PF_RDTSC_INSTRUCTION_AVAILABLE] = TRUE;
    SharedUserData->ProcessorFeatures[PF_PPC_MOVEMEM_64BIT_OK] = TRUE;
    SharedUserData->ProcessorFeatures[PF_PAE_ENABLED] = TRUE; // ???
    SharedUserData->ProcessorFeatures[PF_NX_ENABLED] = TRUE;
    SharedUserData->ProcessorFeatures[PF_FASTFAIL_AVAILABLE] = TRUE;
    SharedUserData->ProcessorFeatures[PF_XSAVE_ENABLED] = TRUE;
    SharedUserData->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] =
        (Prcb->FeatureBits & KF_MMX) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE] =
        ((Prcb->FeatureBits & KF_FXSR) && (Prcb->FeatureBits & KF_XMMI)) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE] =
        ((Prcb->FeatureBits & KF_FXSR) && (Prcb->FeatureBits & KF_XMMI64)) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_3DNOW_INSTRUCTIONS_AVAILABLE] =
        (Prcb->FeatureBits & KF_3DNOW) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_SSE3_INSTRUCTIONS_AVAILABLE] =
        (Prcb->FeatureBits & KF_SSE3) ? TRUE: FALSE;
    SharedUserData->ProcessorFeatures[PF_COMPARE_EXCHANGE128] =
        (Prcb->FeatureBits & KF_CMPXCHG16B) ? TRUE: FALSE;

    /* Set the default NX policy (opt-in) */
    SharedUserData->NXSupportPolicy = NX_SUPPORT_POLICY_OPTIN;

    /* Check if NPX is always on */
    if (strstr(KeLoaderBlock->LoadOptions, "NOEXECUTE=ALWAYSON"))
    {
        /* Set it always on */
        SharedUserData->NXSupportPolicy = NX_SUPPORT_POLICY_ALWAYSON;
        Prcb->FeatureBits |= KF_NX_ENABLED;
    }
    else if (strstr(KeLoaderBlock->LoadOptions, "NOEXECUTE=OPTOUT"))
    {
        /* Set it in opt-out mode */
        SharedUserData->NXSupportPolicy = NX_SUPPORT_POLICY_OPTOUT;
        Prcb->FeatureBits |= KF_NX_ENABLED;
    }
    else if ((strstr(KeLoaderBlock->LoadOptions, "NOEXECUTE=OPTIN")) ||
             (strstr(KeLoaderBlock->LoadOptions, "NOEXECUTE")))
    {
        /* Set the feature bits */
        Prcb->FeatureBits |= KF_NX_ENABLED;
    }
    else if ((strstr(KeLoaderBlock->LoadOptions, "NOEXECUTE=ALWAYSOFF")) ||
             (strstr(KeLoaderBlock->LoadOptions, "EXECUTE")))
    {
        /* Set disabled mode */
        SharedUserData->NXSupportPolicy = NX_SUPPORT_POLICY_ALWAYSOFF;
        Prcb->FeatureBits |= KF_NX_DISABLED;
    }
}

static LDR_DATA_TABLE_ENTRY LdrCoreEntries[3];

void
KiInitModuleList(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PLIST_ENTRY Entry;
    ULONG i;

    /* Initialize the list head */
    InitializeListHead(&PsLoadedModuleList);

    /* Loop the first 3 entries */
    for (Entry = LoaderBlock->LoadOrderListHead.Flink, i = 0;
         Entry != &LoaderBlock->LoadOrderListHead && i < 3;
         Entry = Entry->Flink, i++)
    {
        /* Get the data table entry */
        LdrEntry = CONTAINING_RECORD(Entry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

        /* Copy the entry */
        LdrCoreEntries[i] = *LdrEntry;

        /* Insert the copy into the list */
        InsertTailList(&PsLoadedModuleList, &LdrCoreEntries[i].InLoadOrderLinks);
    }
}

CODE_SEG("INIT")
DECLSPEC_NORETURN
VOID
NTAPI
KiSystemStartup(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    CCHAR Cpu;
    PKTHREAD InitialThread;
    ULONG64 InitialStack;
    PKIPCR Pcr;

    /* HACK */
    FrLdrDbgPrint = LoaderBlock->u.I386.CommonDataArea;
    //FrLdrDbgPrint("Hello from KiSystemStartup!!!\n");

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

    /* Align stack to 16 bytes */
    LoaderBlock->KernelStack &= ~(16 - 1);

    /* Save the initial thread and stack */
    InitialStack = LoaderBlock->KernelStack; // Checkme
    InitialThread = (PKTHREAD)LoaderBlock->Thread;

    /* Set us as the current process */
    InitialThread->ApcState.Process = (PVOID)LoaderBlock->Process;

    /* Initialize the PCR */
    KiInitializePcr(Pcr, Cpu, InitialThread, (PVOID)KiDoubleFaultStack);

    /* Initialize the CPU features */
    KiInitializeCpu(Pcr);

    /* Initial setup for the boot CPU */
    if (Cpu == 0)
    {
        /* Initialize the module list (ntos, hal, kdcom) */
        KiInitModuleList(LoaderBlock);

        /* Setup the TSS descriptors and entries */
        KiInitializeTss(Pcr->TssBase, InitialStack);

        /* Setup the IDT */
        KeInitExceptions();

         /* Initialize debugging system */
        KdInitSystem(0, KeLoaderBlock);

        /* Check for break-in */
        if (KdPollBreakIn()) DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);
    }

    DPRINT1("Pcr = %p, Gdt = %p, Idt = %p, Tss = %p\n",
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
    KeActiveProcessors |= 1ULL << Cpu;

    /* Release lock */
    InterlockedAnd64((PLONG64)&KiFreezeExecutionLock, 0);

    /* Raise to HIGH_LEVEL */
    KfRaiseIrql(HIGH_LEVEL);

    /* Machine specific kernel initialization */
    if (Cpu == 0) KiInitializeKernelMachineDependent(&Pcr->Prcb, LoaderBlock);

    /* Switch to new kernel stack and start kernel bootstrapping */
    KiSwitchToBootStack(InitialStack & ~3);
}

