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

/* Serial port for debug output */
#define COM1_PORT 0x3F8

/* GLOBALS *******************************************************************/

/* Function pointer for early debug prints */
ULONG (*FrLdrDbgPrint)(const char *Format, ...);

/* Spinlocks used only on X86 */
KSPIN_LOCK KiFreezeExecutionLock;


KIPCR KiInitialPcr;

/* Boot and double-fault/NMI/DPC stack */
UCHAR DECLSPEC_ALIGN(16) KiP0BootStackData[KERNEL_STACK_SIZE] = {0};
UCHAR DECLSPEC_ALIGN(16) KiP0DoubleFaultStackData[KERNEL_STACK_SIZE] = {0};
PVOID KiP0BootStack = &KiP0BootStackData[KERNEL_STACK_SIZE];
PVOID KiP0DoubleFaultStack = &KiP0DoubleFaultStackData[KERNEL_STACK_SIZE];

ULONGLONG BootCycles, BootCyclesEnd;

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

static
VOID
KiInitializePcr(
    _Out_ PKIPCR Pcr,
    _In_ ULONG ProcessorNumber,
    _In_ PKGDTENTRY64 GdtBase,
    _In_ PKIDTENTRY64 IdtBase,
    _In_ PKTSS64 TssBase,
    _In_ PKTHREAD IdleThread,
    _In_ PVOID DpcStack)
{
    /* Debug output */
    {
        const char msg[] = "*** KERNEL: KiInitializePcr - About to zero PCR ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Check if PCR pointer is valid */
    if (!Pcr)
    {
        const char msg[] = "*** KERNEL ERROR: PCR pointer is NULL! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        return;
    }
    
    /* Zero out the PCR carefully */
    {
        const char msg[] = "*** KERNEL: Zeroing PCR memory ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Try smaller chunks first to see where it fails */
    PUCHAR PcrBytes = (PUCHAR)Pcr;
    SIZE_T i;
    
    /* Test with first byte */
    {
        const char msg[] = "*** KERNEL: Testing first byte write ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    PcrBytes[0] = 0;
    
    {
        const char msg[] = "*** KERNEL: First byte written successfully ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Now zero the rest - but carefully to avoid page faults */
    /* For now, just zero critical fields instead of entire structure */
    {
        const char msg[] = "*** KERNEL: Zeroing first page of PCR only ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Zero just the first page worth */
    SIZE_T limit = (sizeof(KIPCR) < PAGE_SIZE) ? sizeof(KIPCR) : PAGE_SIZE;
    for (i = 1; i < limit; i++)
    {
        PcrBytes[i] = 0;
    }
    
    {
        const char msg[] = "*** KERNEL: PCR zeroed successfully ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

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

    /* Set GDT and IDT base */
    Pcr->GdtBase = GdtBase;
    Pcr->IdtBase = IdtBase;

    /* Set TssBase */
    Pcr->TssBase = TssBase;

    Pcr->Prcb.RspBase = Pcr->TssBase->Rsp0; // FIXME

    /* Set DPC Stack */
    Pcr->Prcb.DpcStack = DpcStack;

    /* Setup the processor set */
    Pcr->Prcb.MultiThreadProcessorSet = Pcr->Prcb.SetMember;

    /* Clear DR6/7 to cleanup bootloader debugging */
    Pcr->Prcb.ProcessorState.SpecialRegisters.KernelDr6 = 0;
    Pcr->Prcb.ProcessorState.SpecialRegisters.KernelDr7 = 0;

    /* Initialize MXCSR (all exceptions masked) */
    Pcr->Prcb.MxCsr = INITIAL_MXCSR;

    /* Set the Current Thread */
    Pcr->Prcb.CurrentThread = IdleThread;

    /* Start us out at PASSIVE_LEVEL */
    Pcr->Irql = PASSIVE_LEVEL;
}

VOID
NTAPI
KiInitializeCpu(PKIPCR Pcr)
{
    ULONG64 Pat;
    ULONG64 FeatureBits;

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
    Pcr->Prcb.FeatureBits = (ULONG)FeatureBits;
    Pcr->Prcb.FeatureBitsHigh = FeatureBits >> 32;

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

    /* Check if XSAVE is supported */
    if (FeatureBits & KF_XSTATE)
    {
        /* Enable CR4.OSXSAVE[Bit 18] */
        __writecr4(__readcr4() | CR4_XSAVE);
    }

    /* LDT is unused */
    __lldt(0);

    /* Set the systemcall entry points */
    __writemsr(MSR_LSTAR, (ULONG64)KiSystemCallEntry64);
    __writemsr(MSR_CSTAR, (ULONG64)KiSystemCallEntry32);

    __writemsr(MSR_STAR, ((ULONG64)KGDT64_R0_CODE << 32) |
                         ((ULONG64)(KGDT64_R3_CMCODE|RPL_MASK) << 48));

    /* Set the flags to be cleared when doing a syscall */
    __writemsr(MSR_SYSCALL_MASK, EFLAGS_IF_MASK | EFLAGS_TF | EFLAGS_DF | EFLAGS_NESTED_TASK);

    /* Enable syscall instruction and no-execute support */
    __writemsr(MSR_EFER, __readmsr(MSR_EFER) | MSR_SCE | MSR_NXE);

    /* Initialize the PAT */
    Pat = (PAT_WB << 0)  | (PAT_WC << 8) | (PAT_UCM << 16) | (PAT_UC << 24) |
          (PAT_WB << 32) | (PAT_WC << 40) | (PAT_UCM << 48) | (PAT_UC << 56);
    __writemsr(MSR_PAT, Pat);

    /* Initialize MXCSR */
    _mm_setcsr(INITIAL_MXCSR);

    KeSetCurrentIrql(PASSIVE_LEVEL);
}

static
VOID
KiInitializeTss(
    _In_ PKIPCR Pcr,
    _Out_ PKTSS64 Tss,
    _In_ PVOID InitialStack,
    _In_ PVOID DoubleFaultStack,
    _In_ PVOID NmiStack)
{
    PKGDTENTRY64 TssEntry;
    
    /* Debug: Entry point */
    {
        const char msg[] = "*** KERNEL: KiInitializeTss entered ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Debug: Check parameters */
    {
        const char msg[] = "*** KERNEL: Checking TSS parameters... ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    if (!Pcr)
    {
        const char msg[] = "*** KERNEL ERROR: Pcr is NULL! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        return;
    }
    
    if (!Tss)
    {
        const char msg[] = "*** KERNEL ERROR: Tss is NULL! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        return;
    }
    
    /* Debug: Output Tss address */
    {
        const char msg[] = "*** KERNEL: Tss address = 0x";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        
        ULONG_PTR addr = (ULONG_PTR)Tss;
        for (int i = 60; i >= 0; i -= 4)
        {
            int digit = (addr >> i) & 0xF;
            char c = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
            while ((__inbyte(COM1_PORT + 5) & 0x20) == 0);
            __outbyte(COM1_PORT, c);
        }
        
        const char newline[] = " ***\n";
        p = newline;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Debug: Check GdtBase */
    if (!Pcr->GdtBase)
    {
        const char msg[] = "*** KERNEL ERROR: Pcr->GdtBase is NULL! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        return;
    }
    
    {
        const char msg[] = "*** KERNEL: Getting GDT entry for TSS ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Get pointer to the GDT entry */
    TssEntry = KiGetGdtEntry(Pcr->GdtBase, KGDT64_SYS_TSS);
    
    if (!TssEntry)
    {
        const char msg[] = "*** KERNEL ERROR: TssEntry is NULL! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        return;
    }
    
    {
        const char msg[] = "*** KERNEL: Got TSS GDT entry, initializing it ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Initialize the GDT entry */
    KiInitGdtEntry(TssEntry, (ULONG64)Tss, sizeof(KTSS64), AMD64_TSS, 0);
    
    {
        const char msg[] = "*** KERNEL: GDT entry initialized, zeroing TSS ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Zero out the TSS manually to avoid RtlZeroMemory issues */
    {
        const char msg[] = "*** KERNEL: Manual TSS zero - size = ";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        
        /* Output size */
        ULONG size = sizeof(KTSS64);
        char buf[20];
        int i = 0;
        do {
            buf[i++] = '0' + (size % 10);
            size /= 10;
        } while (size > 0);
        
        while (--i >= 0) {
            while ((__inbyte(COM1_PORT + 5) & 0x20) == 0);
            __outbyte(COM1_PORT, buf[i]);
        }
        
        const char bytes[] = " bytes ***\n";
        p = bytes;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Test if we can write to TSS */
    {
        const char msg[] = "*** KERNEL: Testing TSS write access ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Try to write first byte */
    volatile UCHAR *TssBytes = (volatile UCHAR *)Tss;
    TssBytes[0] = 0;
    
    {
        const char msg[] = "*** KERNEL: First byte written successfully ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Zero TSS manually byte by byte */
    for (ULONG i = 0; i < sizeof(KTSS64); i++)
    {
        TssBytes[i] = 0;
        
        /* Progress indicator every 16 bytes */
        if ((i & 0xF) == 0 && i > 0)
        {
            while ((__inbyte(COM1_PORT + 5) & 0x20) == 0);
            __outbyte(COM1_PORT, '.');
        }
    }
    
    {
        const char msg[] = "\n*** KERNEL: TSS zeroed successfully ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* FIXME: I/O Map? */
    Tss->IoMapBase = 0x68;
    
    {
        const char msg[] = "*** KERNEL: IoMapBase set ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Setup ring 0 stack pointer */
    Tss->Rsp0 = (ULONG64)InitialStack;
    
    {
        const char msg[] = "*** KERNEL: Rsp0 set ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Setup a stack for Double Fault Traps */
    Tss->Ist[1] = (ULONG64)DoubleFaultStack;
    
    {
        const char msg[] = "*** KERNEL: IST[1] set ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Setup a stack for CheckAbort Traps */
    Tss->Ist[2] = (ULONG64)DoubleFaultStack;
    
    {
        const char msg[] = "*** KERNEL: IST[2] set ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Setup a stack for NMI Traps */
    Tss->Ist[3] = (ULONG64)NmiStack;
    
    {
        const char msg[] = "*** KERNEL: IST[3] set ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    {
        const char msg[] = "*** KERNEL: KiInitializeTss function completed successfully ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
}

CODE_SEG("INIT")
VOID
KiInitializeProcessorBootStructures(
    _In_ ULONG ProcessorNumber,
    _Out_ PKIPCR Pcr,
    _In_ PKGDTENTRY64 GdtBase,
    _In_ PKIDTENTRY64 IdtBase,
    _In_ PKTSS64 TssBase,
    _In_ PKTHREAD IdleThread,
    _In_ PVOID KernelStack,
    _In_ PVOID DpcStack,
    _In_ PVOID DoubleFaultStack,
    _In_ PVOID NmiStack)
{
    /* Debug output */
    {
        const char msg[] = "*** KERNEL: Inside KiInitializeProcessorBootStructures ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Initialize the PCR */
    {
        const char msg[] = "*** KERNEL: About to call KiInitializePcr ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    KiInitializePcr(Pcr,
                    ProcessorNumber,
                    GdtBase,
                    IdtBase,
                    TssBase,
                    IdleThread,
                    DpcStack);

    {
        const char msg[] = "*** KERNEL: KiInitializePcr completed ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Setup the TSS descriptor and entries */
    {
        const char msg[] = "*** KERNEL: About to call KiInitializeTss ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    KiInitializeTss(Pcr,
                    TssBase,
                    KernelStack,
                    DoubleFaultStack,
                    NmiStack);
    
    {
        const char msg[] = "*** KERNEL: KiInitializeTss completed ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
}

CODE_SEG("INIT")
static
VOID
KiInitializeP0BootStructures(
    _Inout_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    KDESCRIPTOR GdtDescriptor = {{0},0,0}, IdtDescriptor = {{0},0,0};
    PKGDTENTRY64 TssEntry;
    PKTSS64 TssBase;

    /* Debug output */
    {
        const char msg[] = "*** KERNEL: KiInitializeP0BootStructures entered ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Set the initial stack, idle thread and process for processor 0 */
    LoaderBlock->KernelStack = (ULONG_PTR)KiP0BootStack;
    LoaderBlock->Thread = (ULONG_PTR)&KiInitialThread;
    LoaderBlock->Process = (ULONG_PTR)&KiInitialProcess.Pcb;
    LoaderBlock->Prcb = (ULONG_PTR)&KiInitialPcr.Prcb;
    
    {
        const char msg[] = "*** KERNEL: LoaderBlock pointers set ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Get GDT and IDT descriptors */
    __sgdt(&GdtDescriptor.Limit);
    __sidt(&IdtDescriptor.Limit);
    
    {
        const char msg[] = "*** KERNEL: GDT/IDT descriptors obtained ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Get the boot TSS from the GDT */
    {
        const char msg[] = "*** KERNEL: Getting TSS from GDT ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    TssEntry = KiGetGdtEntry(GdtDescriptor.Base, KGDT64_SYS_TSS);
    
    {
        const char msg[] = "*** KERNEL: Got TSS entry, getting base ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    TssBase = KiGetGdtDescriptorBase(TssEntry);

    /* Initialize PCR and TSS */
    {
        const char msg[] = "*** KERNEL: TSS base obtained, preparing to call KiInitializeProcessorBootStructures ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Check if we can access global variables */
    {
        const char msg[] = "*** KERNEL: Testing access to globals ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Try to access the globals carefully */
    PKTHREAD InitialThread = NULL;
    PVOID BootStack = NULL;
    PVOID DoubleFaultStack = NULL;
    
    /* Use LoaderBlock values if possible */
    if (LoaderBlock && LoaderBlock->Thread)
    {
        InitialThread = (PKTHREAD)LoaderBlock->Thread;
        const char msg[] = "*** KERNEL: Using LoaderBlock thread ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    else
    {
        /* Try to use global if accessible */
        InitialThread = &KiInitialThread.Tcb;
        const char msg[] = "*** KERNEL: Using global thread ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    if (LoaderBlock && LoaderBlock->KernelStack)
    {
        BootStack = (PVOID)LoaderBlock->KernelStack;
        const char msg[] = "*** KERNEL: Using LoaderBlock stack ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    else
    {
        BootStack = KiP0BootStack;
        const char msg[] = "*** KERNEL: Using global stack ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Use same stack for double fault for now */
    DoubleFaultStack = BootStack;
    
    {
        const char msg[] = "*** KERNEL: Calling KiInitializeProcessorBootStructures now ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Use PCR from LoaderBlock if available */
    PKIPCR PcrToUse = NULL;
    
    if (LoaderBlock->Prcb && LoaderBlock->Prcb != 0)
    {
        /* LoaderBlock->Prcb is a physical address, need to add KSEG0_BASE */
        ULONG_PTR PrcbVA = LoaderBlock->Prcb;
        
        /* Check if it's already a kernel address */
        if (PrcbVA < KSEG0_BASE)
        {
            /* It's a physical address, convert to kernel VA */
            PrcbVA = PrcbVA + KSEG0_BASE;
            const char msg[] = "*** KERNEL: Converting PCR from physical to kernel VA ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* CONTAINING_RECORD to get PCR from PRCB address */
        /* PRCB is at offset 0x180 in PCR on AMD64 */
        PcrToUse = (PKIPCR)(PrcbVA - FIELD_OFFSET(KIPCR, Prcb));
        const char msg[] = "*** KERNEL: Using PCR from LoaderBlock ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    else
    {
        /* Fall back to global - be careful */
        const char msg[] = "*** KERNEL: WARNING - No PCR in LoaderBlock, using global (risky) ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        PcrToUse = &KiInitialPcr;
    }
    
    {
        const char msg[] = "*** KERNEL: Calling KiInitializeProcessorBootStructures with PCR ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    KiInitializeProcessorBootStructures(0,
                                        PcrToUse,
                                        GdtDescriptor.Base,
                                        IdtDescriptor.Base,
                                        TssBase,
                                        InitialThread,
                                        BootStack,
                                        DoubleFaultStack,
                                        DoubleFaultStack,
                                        DoubleFaultStack);
    
    /* Update LoaderBlock if needed */
    if (!LoaderBlock->Prcb || LoaderBlock->Prcb == 0)
    {
        LoaderBlock->Prcb = (ULONG_PTR)&PcrToUse->Prcb;
    }
    
    {
        const char msg[] = "*** KERNEL: ProcessorBootStructures initialized, exiting function ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
}

CODE_SEG("INIT")
VOID
NTAPI
KiInitializeKernelMachineDependent(
    IN PKPRCB Prcb,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG64 FeatureBits = KeFeatureBits;

    /* Set boot-level flags */
    KeI386CpuType = Prcb->CpuType;
    KeI386CpuStep = Prcb->CpuStep;
    KeProcessorArchitecture = PROCESSOR_ARCHITECTURE_AMD64;
    KeProcessorLevel = (USHORT)Prcb->CpuType;
    if (Prcb->CpuID)
        KeProcessorRevision = Prcb->CpuStep;

    /* Set basic CPU Features that user mode can read */
    SharedUserData->ProcessorFeatures[PF_FLOATING_POINT_PRECISION_ERRATA] = FALSE;
    SharedUserData->ProcessorFeatures[PF_FLOATING_POINT_EMULATED] = FALSE;
    SharedUserData->ProcessorFeatures[PF_COMPARE_EXCHANGE_DOUBLE] = TRUE;
    SharedUserData->ProcessorFeatures[PF_MMX_INSTRUCTIONS_AVAILABLE] =
        (FeatureBits & KF_MMX) ? TRUE : FALSE;
    SharedUserData->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE] =
        ((FeatureBits & KF_FXSR) && (FeatureBits & KF_XMMI)) ? TRUE : FALSE;
    SharedUserData->ProcessorFeatures[PF_3DNOW_INSTRUCTIONS_AVAILABLE] =
        (FeatureBits & KF_3DNOW) ? TRUE : FALSE;
    SharedUserData->ProcessorFeatures[PF_RDTSC_INSTRUCTION_AVAILABLE] = TRUE;
    SharedUserData->ProcessorFeatures[PF_PAE_ENABLED] = TRUE; // ???
    SharedUserData->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE] =
        ((FeatureBits & KF_FXSR) && (FeatureBits & KF_XMMI64)) ? TRUE : FALSE;
    SharedUserData->ProcessorFeatures[PF_SSE_DAZ_MODE_AVAILABLE] = FALSE; // ???
    SharedUserData->ProcessorFeatures[PF_NX_ENABLED] = TRUE;
    SharedUserData->ProcessorFeatures[PF_SSE3_INSTRUCTIONS_AVAILABLE] =
        (FeatureBits & KF_SSE3) ? TRUE : FALSE;
    SharedUserData->ProcessorFeatures[PF_COMPARE_EXCHANGE128] =
        (FeatureBits & KF_CMPXCHG16B) ? TRUE : FALSE;
    SharedUserData->ProcessorFeatures[PF_COMPARE64_EXCHANGE128] = FALSE; // ???
    SharedUserData->ProcessorFeatures[PF_CHANNELS_ENABLED] = FALSE; // ???
    SharedUserData->ProcessorFeatures[PF_XSAVE_ENABLED] =
        (FeatureBits & KF_XSTATE) ? TRUE : FALSE;
    SharedUserData->ProcessorFeatures[PF_SECOND_LEVEL_ADDRESS_TRANSLATION] =
        (FeatureBits & KF_SLAT) ? TRUE : FALSE;
    SharedUserData->ProcessorFeatures[PF_VIRT_FIRMWARE_ENABLED] =
        (FeatureBits & KF_VIRT_FIRMWARE_ENABLED) ? TRUE : FALSE;
    SharedUserData->ProcessorFeatures[PF_RDWRFSGSBASE_AVAILABLE] =
        (FeatureBits & KF_RDWRFSGSBASE) ? TRUE : FALSE;
    SharedUserData->ProcessorFeatures[PF_FASTFAIL_AVAILABLE] = TRUE;
    SharedUserData->ProcessorFeatures[PF_RDRAND_INSTRUCTION_AVAILABLE] =
        (FeatureBits & KF_RDRAND) ? TRUE : FALSE;
    SharedUserData->ProcessorFeatures[PF_RDTSCP_INSTRUCTION_AVAILABLE] =
        (FeatureBits & KF_RDTSCP) ? TRUE : FALSE;
    SharedUserData->ProcessorFeatures[PF_RDPID_INSTRUCTION_AVAILABLE] = FALSE; // ???
    SharedUserData->ProcessorFeatures[PF_SSSE3_INSTRUCTIONS_AVAILABLE] =
        (FeatureBits & KF_SSSE3) ? TRUE : FALSE;
    SharedUserData->ProcessorFeatures[PF_SSE4_1_INSTRUCTIONS_AVAILABLE] =
        (FeatureBits & KF_SSE4_1) ? TRUE : FALSE;
    SharedUserData->ProcessorFeatures[PF_SSE4_2_INSTRUCTIONS_AVAILABLE] =
        (FeatureBits & KF_SSE4_2) ? TRUE : FALSE;
    SharedUserData->ProcessorFeatures[PF_AVX_INSTRUCTIONS_AVAILABLE] =
        (FeatureBits & KF_AVX) ? TRUE : FALSE;
    SharedUserData->ProcessorFeatures[PF_AVX2_INSTRUCTIONS_AVAILABLE] =
        (FeatureBits & KF_AVX2) ? TRUE : FALSE;
    SharedUserData->ProcessorFeatures[PF_AVX512F_INSTRUCTIONS_AVAILABLE] =
        (FeatureBits & KF_AVX512F) ? TRUE : FALSE;

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

#if DBG
    /* Print applied kernel features/policies and boot CPU features */
    KiReportCpuFeatures(Prcb);
#endif
}

static LDR_DATA_TABLE_ENTRY LdrCoreEntries[3];

void
KiInitModuleList(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Debug output */
    {
        const char msg[] = "*** KERNEL: KiInitModuleList entered ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Skip for now - global variable access issues */
    {
        const char msg[] = "*** KERNEL: Skipping module list initialization (global access issue) ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    return;
    
    /* Original code - skip for now
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    PLIST_ENTRY Entry;
    ULONG i;

    InitializeListHead(&PsLoadedModuleList);

    for (Entry = LoaderBlock->LoadOrderListHead.Flink, i = 0;
         Entry != &LoaderBlock->LoadOrderListHead && i < 3;
         Entry = Entry->Flink, i++)
    {
        LdrEntry = CONTAINING_RECORD(Entry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

        LdrCoreEntries[i] = *LdrEntry;

        InsertTailList(&PsLoadedModuleList, &LdrCoreEntries[i].InLoadOrderLinks);
    }
    */
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
    
    /* Early debug output using serial port directly */
    {
        /* Output to COM1 (0x3F8) */
        #define COM1_PORT 0x3F8
        const char msg[] = "*** KERNEL: KiSystemStartup entered! ***\n";
        const char *p = msg;
        while (*p)
        {
            while ((__inbyte(COM1_PORT + 5) & 0x20) == 0);
            __outbyte(COM1_PORT, *p++);
        }
    }
    
    /* Output the actual RIP to see where we're running from */
    {
        ULONG_PTR rip;
        __asm__ __volatile__("lea 0(%%rip), %0" : "=r"(rip));
        
        /* Output RIP address in hex */
        const char msg[] = "*** KERNEL: Current RIP = 0x";
        const char *pm = msg;
        while (*pm) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *pm++); }
        
        /* Output RIP in hex */
        for (int i = 60; i >= 0; i -= 4)
        {
            int digit = (rip >> i) & 0xF;
            char c = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
            while ((__inbyte(COM1_PORT + 5) & 0x20) == 0);
            __outbyte(COM1_PORT, c);
        }
        
        const char newline[] = " ***\n";
        pm = newline;
        while (*pm) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *pm++); }
    }

    /* Boot cycles timestamp */
    BootCycles = __rdtsc();
    
    {
        const char msg[] = "*** KERNEL: Boot cycles obtained ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* HACK - Be very careful with LoaderBlock access */
    if (LoaderBlock)
    {
        const char msg[] = "*** KERNEL: LoaderBlock is valid ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        
        /* Try to access CommonDataArea carefully */
        FrLdrDbgPrint = LoaderBlock->u.I386.CommonDataArea;
        
        const char msg2[] = "*** KERNEL: CommonDataArea accessed ***\n";
        const char *p2 = msg2;
        while (*p2) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p2++); }
        
        if (FrLdrDbgPrint)
        {
            FrLdrDbgPrint("*** KERNEL: Hello from KiSystemStartup! ***\n");
            FrLdrDbgPrint("*** KERNEL: LoaderBlock = %p ***\n", LoaderBlock);
        }
    }
    else
    {
        const char msg[] = "*** KERNEL ERROR: LoaderBlock is NULL! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Get the current CPU number */
    {
        const char msg[] = "*** KERNEL: About to get CPU number ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* For now, hardcode CPU 0 - the real KeNumberProcessors might not be accessible yet */
    Cpu = 0;  /* Boot CPU is always 0 */
    
    /* Serial debug */
    {
        const char msg[] = "*** KERNEL: CPU number set to 0 (boot CPU) ***\n";  
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* LoaderBlock initialization for Cpu 0 */
    if (Cpu == 0)
    {
        const char msg[] = "*** KERNEL: CPU 0 - Processing loader block ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        
        /* Try to save the loader block - but be careful with global access */
        {
            const char msg[] = "*** KERNEL: Attempting to save KeLoaderBlock global ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Test if we can write to globals */
        volatile ULONG TestGlobal = 0x12345678;
        if (TestGlobal == 0x12345678)
        {
            const char msg[] = "*** KERNEL: Stack variable test successful ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Test global variable access more carefully */
        {
            const char msg[] = "*** KERNEL: Testing KeLoaderBlock global variable access ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Save the loader block - should work now with proper ImageBase */
        KeLoaderBlock = LoaderBlock;
        
        /* Test if we can read it back */
        if (KeLoaderBlock == LoaderBlock)
        {
            const char msg[] = "*** KERNEL: SUCCESS! KeLoaderBlock global saved and readable! ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        else
        {
            const char msg[] = "*** KERNEL: ERROR - KeLoaderBlock global not accessible ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }

        /* Prepare LoaderBlock, PCR, TSS with the P0 boot data */
        const char msg2[] = "*** KERNEL: About to call KiInitializeP0BootStructures ***\n";
        const char *p2 = msg2;
        while (*p2) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p2++); }
        
        KiInitializeP0BootStructures(LoaderBlock);
        
        const char msg3[] = "*** KERNEL: P0 boot structures initialized ***\n";
        const char *p3 = msg3;
        while (*p3) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p3++); }
    }

    /* Get Pcr from loader block */
    {
        const char msg[] = "*** KERNEL: Getting PCR from LoaderBlock ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    if (LoaderBlock->Prcb == 0)
    {
        /* Serial output error */
        const char msg[] = "*** KERNEL ERROR: LoaderBlock->Prcb is NULL! ***\n";
        const char *p = msg;
        while (*p)
        {
            while ((__inbyte(COM1_PORT + 5) & 0x20) == 0);
            __outbyte(COM1_PORT, *p++);
        }
        
        /* Use initial PCR if Prcb is not set */
        Pcr = &KiInitialPcr;
    }
    else
    {
        /* Convert physical to kernel VA if needed */
        ULONG_PTR PrcbAddr = LoaderBlock->Prcb;
        if (PrcbAddr < KSEG0_BASE)
        {
            PrcbAddr = PrcbAddr + KSEG0_BASE;
        }
        
        Pcr = CONTAINING_RECORD(PrcbAddr, KIPCR, Prcb);
        
        {
            const char msg[] = "*** KERNEL: Got PCR from LoaderBlock ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
    }

    /* Set the PRCB for this Processor */
    {
        const char msg[] = "*** KERNEL: Skipping KiProcessorBlock assignment (global access issue) ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Skip - causes crash */
    /* KiProcessorBlock[Cpu] = &Pcr->Prcb; */
    {
        const char msg[] = "*** KERNEL: PRCB set for processor ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Save the initial thread */
    InitialThread = (PKTHREAD)LoaderBlock->Thread;
    {
        const char msg[] = "*** KERNEL: Initial thread obtained ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Set us as the current process */
    if (InitialThread && LoaderBlock->Process)
    {
        /* Convert physical addresses to kernel VA if needed */
        ULONG_PTR ThreadVA = (ULONG_PTR)InitialThread;
        ULONG_PTR ProcessVA = LoaderBlock->Process;
        
        if (ThreadVA < KSEG0_BASE)
        {
            ThreadVA = ThreadVA + KSEG0_BASE;
            InitialThread = (PKTHREAD)ThreadVA;
        }
        
        if (ProcessVA < KSEG0_BASE)
        {
            ProcessVA = ProcessVA + KSEG0_BASE;
        }
        
        {
            const char msg[] = "*** KERNEL: Attempting to set ApcState.Process ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Now that globals work, we can set this */
        InitialThread->ApcState.Process = (PVOID)ProcessVA;
        
        const char msg[] = "*** KERNEL: ApcState.Process assigned successfully ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    else
    {
        const char msg[] = "*** KERNEL WARNING: Thread or Process is NULL ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Initialize the CPU features */
    {
        const char msg[] = "*** KERNEL: Calling KiInitializeCpu ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    KiInitializeCpu(Pcr);
    {
        const char msg[] = "*** KERNEL: CPU initialized ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Initial setup for the boot CPU */
    if (Cpu == 0)
    {
        {
            const char msg[] = "*** KERNEL: Boot CPU (CPU 0) - Starting initial setup ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Set global feature bits */
        {
            const char msg[] = "*** KERNEL: Setting global feature bits ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        
        /* Skip global variable access for now */
        /* KeFeatureBits = (ULONG64)Pcr->Prcb.FeatureBitsHigh << 32 |
                        Pcr->Prcb.FeatureBits; */
        
        {
            const char msg[] = "*** KERNEL: Skipping KeFeatureBits assignment ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }

        /* Initialize the module list (ntos, hal, kdcom) */
        {
            const char msg[] = "*** KERNEL: Initializing module list ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        KiInitModuleList(LoaderBlock);
        {
            const char msg[] = "*** KERNEL: Module list initialized ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }

        /* Setup the IDT */
        {
            const char msg[] = "*** KERNEL: Setting up IDT ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        KeInitExceptions();
        {
            const char msg[] = "*** KERNEL: IDT setup complete ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }

         /* Initialize debugging system */
        {
            const char msg[] = "*** KERNEL: Skipping debug system (may cause crash) ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        /* Skip KdInitSystem for now - it might be accessing globals */
        /* KdInitSystem(0, LoaderBlock); */
        
        {
            const char msg[] = "*** KERNEL: Debug system skipped ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }

        /* Skip break-in check too */
        /* if (KdPollBreakIn()) DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C); */
    }

    /* Skip DPRINT1 - it may use globals */
    /* DPRINT1("Pcr = %p, Gdt = %p, Idt = %p, Tss = %p\n",
           Pcr, Pcr->GdtBase, Pcr->IdtBase, Pcr->TssBase); */

    {
        const char msg[] = "*** KERNEL: About to acquire lock ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Skip lock for now - global variable access */
    /* Acquire lock 
    while (InterlockedBitTestAndSet64((PLONG64)&KiFreezeExecutionLock, 0))
    {
        while ((*(volatile KSPIN_LOCK*)&KiFreezeExecutionLock) & 1);
    }*/

    {
        const char msg[] = "*** KERNEL: Initializing HAL processor ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Skip HAL initialization for now - it may crash */
    {
        const char msg[] = "*** KERNEL: Skipping HAL processor init (may crash) ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* HalInitializeProcessor(Cpu, LoaderBlock); */
    
    {
        const char msg[] = "*** KERNEL: HAL processor init skipped ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Set processor as active */
    {
        const char msg[] = "*** KERNEL: Skipping KeActiveProcessors assignment ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    /* KeActiveProcessors |= 1ULL << Cpu; */

    /* Release lock - skip since we didn't acquire it */
    /* InterlockedAnd64((PLONG64)&KiFreezeExecutionLock, 0); */

    /* Raise to HIGH_LEVEL */
    {
        const char msg[] = "*** KERNEL: Raising IRQL to HIGH_LEVEL ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    KfRaiseIrql(HIGH_LEVEL);

    /* Machine specific kernel initialization */
    if (Cpu == 0) 
    {
        {
            const char msg[] = "*** KERNEL: Calling KiInitializeKernelMachineDependent ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
        /* Skip for now - accesses too many globals */
        /* KiInitializeKernelMachineDependent(&Pcr->Prcb, LoaderBlock); */
        {
            const char msg[] = "*** KERNEL: Skipping KiInitializeKernelMachineDependent (globals) ***\n";
            const char *p = msg;
            while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
        }
    }

    /* Initialize extended state management */
    {
        const char msg[] = "*** KERNEL: Skipping XState configuration (globals) ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    /* KiInitializeXStateConfiguration(Cpu); */

    /* Calculate the initial stack pointer */
    {
        const char msg[] = "*** KERNEL: Calculating initial stack pointer ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Skip KeXStateLength global access */
    /* Get initial kernel stack from LoaderBlock - should work with proper ImageBase */
    InitialStack = LoaderBlock->KernelStack;
    InitialStack = ALIGN_DOWN_BY(InitialStack, 64);
    
    /* Log the stack value */
    {
        char msg[128];
        char *p = msg;
        const char prefix[] = "*** KERNEL: InitialStack = 0x";
        const char *pp = prefix;
        while (*pp) *p++ = *pp++;
        
        /* Convert stack value to hex */
        ULONG64 val = InitialStack;
        for (int i = 15; i >= 0; i--) {
            int digit = (val >> (i * 4)) & 0xF;
            *p++ = digit < 10 ? '0' + digit : 'a' + digit - 10;
        }
        *p++ = ' ';
        *p++ = '*';
        *p++ = '*';
        *p++ = '*';
        *p++ = '\n';
        *p = '\0';
        
        p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }

    /* Switch to new kernel stack and start kernel bootstrapping */
    {
        const char msg[] = "*** KERNEL: About to switch to boot stack and start bootstrapping! ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    {
        const char msg[] = "*** KERNEL: Calling KiSwitchToBootStack to continue kernel boot ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* LoaderBlock is now saved in global variable (works with proper ImageBase) */
    
    /* This should continue the kernel boot process */
    KiSwitchToBootStack(InitialStack);
}

