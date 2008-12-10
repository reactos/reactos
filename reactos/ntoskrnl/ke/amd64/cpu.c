/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/amd64/cpu.c
 * PURPOSE:         Routines for CPU-level support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FIXME: Local EFLAGS defines not used anywhere else */
#define EFLAGS_IOPL     0x3000
#define EFLAGS_NF       0x4000
#define EFLAGS_RF       0x10000
#define EFLAGS_ID       0x200000

/* GLOBALS *******************************************************************/

/* The Boot TSS */
KTSS64 KiBootTss;

/* CPU Features and Flags */
ULONG KeI386CpuType;
ULONG KeI386CpuStep;
ULONG KeProcessorArchitecture;
ULONG KeProcessorLevel;
ULONG KeProcessorRevision;
ULONG KeFeatureBits;
ULONG KeI386MachineType;
ULONG KeI386NpxPresent = 1;
ULONG KeI386XMMIPresent = 0;
ULONG KeI386FxsrPresent = 0;
ULONG KeLargestCacheLine = 0x40;
ULONG KiDmaIoCoherency = 0;
CHAR KeNumberProcessors = 0;
KAFFINITY KeActiveProcessors = 1;
BOOLEAN KiI386PentiumLockErrataPresent;
BOOLEAN KiSMTProcessorsPresent;

/* Freeze data */
KIRQL KiOldIrql;
ULONG KiFreezeFlag;

/* CPU Signatures */
static const CHAR CmpIntelID[]       = "GenuineIntel";
static const CHAR CmpAmdID[]         = "AuthenticAMD";
static const CHAR CmpCyrixID[]       = "CyrixInstead";
static const CHAR CmpTransmetaID[]   = "GenuineTMx86";
static const CHAR CmpCentaurID[]     = "CentaurHauls";
static const CHAR CmpRiseID[]        = "RiseRiseRise";

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KiSetProcessorType(VOID)
{
    ULONG64 EFlags;
    INT Reg[4];
    ULONG Stepping, Type;

    /* Start by assuming no CPUID data */
    KeGetCurrentPrcb()->CpuID = 0;

    /* Save EFlags */
    Ke386SaveFlags(EFlags);

    /* Do CPUID 1 now */
    __cpuid(Reg, 1);

    /*
     * Get the Stepping and Type. The stepping contains both the
     * Model and the Step, while the Type contains the returned Type.
     * We ignore the family.
     *
     * For the stepping, we convert this: zzzzzzxy into this: x0y
     */
    Stepping = Reg[0] & 0xF0;
    Stepping <<= 4;
    Stepping += (Reg[0] & 0xFF);
    Stepping &= 0xF0F;
    Type = Reg[0] & 0xF00;
    Type >>= 8;

    /* Save them in the PRCB */
    KeGetCurrentPrcb()->CpuID = TRUE;
    KeGetCurrentPrcb()->CpuType = (UCHAR)Type;
    KeGetCurrentPrcb()->CpuStep = (USHORT)Stepping;

    /* Restore EFLAGS */
    Ke386RestoreFlags(EFlags);
}

ULONG
NTAPI
KiGetCpuVendor(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    INT Vendor[5];
    ULONG Temp;

    /* Assume no Vendor ID and fail if no CPUID Support. */
    Prcb->VendorString[0] = 0;
    if (!Prcb->CpuID) return 0;

    /* Get the Vendor ID and null-terminate it */
    __cpuid(Vendor, 0);
    Vendor[4] = 0;

    /* Re-arrange vendor string */
    Temp = Vendor[2];
    Vendor[2] = Vendor[3];
    Vendor[3] = Temp;

    /* Copy it to the PRCB and null-terminate it again */
    RtlCopyMemory(Prcb->VendorString,
                  &Vendor[1],
                  sizeof(Prcb->VendorString) - sizeof(CHAR));
    Prcb->VendorString[sizeof(Prcb->VendorString) - sizeof(CHAR)] = ANSI_NULL;

    /* Now check the CPU Type */
    if (!strcmp((PCHAR)Prcb->VendorString, CmpIntelID))
    {
        return CPU_INTEL;
    }
    else if (!strcmp((PCHAR)Prcb->VendorString, CmpAmdID))
    {
        return CPU_AMD;
    }
    else if (!strcmp((PCHAR)Prcb->VendorString, CmpCyrixID))
    {
        DPRINT1("Cyrix CPUs not fully supported\n");
        return 0;
    }
    else if (!strcmp((PCHAR)Prcb->VendorString, CmpTransmetaID))
    {
        DPRINT1("Transmeta CPUs not fully supported\n");
        return 0;
    }
    else if (!strcmp((PCHAR)Prcb->VendorString, CmpCentaurID))
    {
        DPRINT1("VIA CPUs not fully supported\n");
        return 0;
    }
    else if (!strcmp((PCHAR)Prcb->VendorString, CmpRiseID))
    {
        DPRINT1("Rise CPUs not fully supported\n");
        return 0;
    }

    /* Invalid CPU */
    return 0;
}

ULONG
NTAPI
KiGetFeatureBits(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    ULONG Vendor;
    ULONG FeatureBits = KF_WORKING_PTE;
    INT Reg[4];
    ULONG CpuFeatures = 0;

    /* Get the Vendor ID */
    Vendor = KiGetCpuVendor();

    /* Make sure we got a valid vendor ID at least. */
    if (!Vendor) return FeatureBits;

    /* Get the CPUID Info. Features are in Reg[3]. */
    __cpuid(Reg, 1);

    /* Set the initial APIC ID */
    Prcb->InitialApicId = (UCHAR)(Reg[1] >> 24);

    /* Set the current features */
    CpuFeatures = Reg[3];

    /* Convert all CPUID Feature bits into our format */
    if (CpuFeatures & 0x00000002) FeatureBits |= KF_V86_VIS | KF_CR4;
    if (CpuFeatures & 0x00000008) FeatureBits |= KF_LARGE_PAGE | KF_CR4;
    if (CpuFeatures & 0x00000010) FeatureBits |= KF_RDTSC;
    if (CpuFeatures & 0x00000100) FeatureBits |= KF_CMPXCHG8B;
    if (CpuFeatures & 0x00000800) FeatureBits |= KF_FAST_SYSCALL;
    if (CpuFeatures & 0x00001000) FeatureBits |= KF_MTRR;
    if (CpuFeatures & 0x00002000) FeatureBits |= KF_GLOBAL_PAGE | KF_CR4;
    if (CpuFeatures & 0x00008000) FeatureBits |= KF_CMOV;
    if (CpuFeatures & 0x00010000) FeatureBits |= KF_PAT;
    if (CpuFeatures & 0x00200000) FeatureBits |= KF_DTS;
    if (CpuFeatures & 0x00800000) FeatureBits |= KF_MMX;
    if (CpuFeatures & 0x01000000) FeatureBits |= KF_FXSR;
    if (CpuFeatures & 0x02000000) FeatureBits |= KF_XMMI;
    if (CpuFeatures & 0x04000000) FeatureBits |= KF_XMMI64;

    /* Check if the CPU has hyper-threading */
    if (CpuFeatures & 0x10000000)
    {
        /* Set the number of logical CPUs */
        Prcb->LogicalProcessorsPerPhysicalProcessor = (UCHAR)(Reg[1] >> 16);
        if (Prcb->LogicalProcessorsPerPhysicalProcessor > 1)
        {
            /* We're on dual-core */
            KiSMTProcessorsPresent = TRUE;
        }
    }
    else
    {
        /* We only have a single CPU */
        Prcb->LogicalProcessorsPerPhysicalProcessor = 1;
    }

    /* Check extended cpuid features */
    __cpuid(Reg, 0x80000000);
    if ((Reg[0] & 0xffffff00) == 0x80000000)
    {
        /* Check if CPUID 0x80000001 is supported */
        if (Reg[0] >= 0x80000001)
        {
            /* Check which extended features are available. */
            __cpuid(Reg, 0x80000001);

            /* Check if NX-bit is supported */
            if (Reg[3] & 0x00100000) FeatureBits |= KF_NX_BIT;

            /* Now handle each features for each CPU Vendor */
            switch (Vendor)
            {
                case CPU_AMD:
                    if (Reg[3] & 0x80000000) FeatureBits |= KF_3DNOW;
                    break;
            }
        }
    }

    /* Return the Feature Bits */
    return FeatureBits;
}

VOID
NTAPI
KiInitializeCpuFeatures()
{
    /* Enable Write-Protection */
    __writecr0(__readcr0() | CR0_WP);

    /* Disable fpu monitoring */
    __writecr0(__readcr0() & ~CR0_MP);

    /* Enable fx save restore support */
    __writecr4(__readcr4() | CR4_FXSR);

}

VOID
NTAPI
KiGetCacheInformation(VOID)
{
    PKIPCR Pcr = (PKIPCR)KeGetPcr();
    ULONG Vendor;
    INT Data[4];
    ULONG CacheRequests = 0, i;
    ULONG CurrentRegister;
    UCHAR RegisterByte;
    BOOLEAN FirstPass = TRUE;

    /* Set default L2 size */
    Pcr->SecondLevelCacheSize = 0;

    /* Get the Vendor ID and make sure we support CPUID */
    Vendor = KiGetCpuVendor();
    if (!Vendor) return;

    /* Check the Vendor ID */
    switch (Vendor)
    {
        /* Handle Intel case */
        case CPU_INTEL:

            /*Check if we support CPUID 2 */
            __cpuid(Data, 0);
            if (Data[0] >= 2)
            {
                /* We need to loop for the number of times CPUID will tell us to */
                do
                {
                    /* Do the CPUID call */
                    __cpuid(Data, 2);

                    /* Check if it was the first call */
                    if (FirstPass)
                    {
                        /*
                         * The number of times to loop is the first byte. Read
                         * it and then destroy it so we don't get confused.
                         */
                        CacheRequests = Data[0] & 0xFF;
                        Data[0] &= 0xFFFFFF00;

                        /* Don't go over this again */
                        FirstPass = FALSE;
                    }

                    /* Loop all 4 registers */
                    for (i = 0; i < 4; i++)
                    {
                        /* Get the current register */
                        CurrentRegister = Data[i];

                        /*
                         * If the upper bit is set, then this register should
                         * be skipped.
                         */
                        if (CurrentRegister & 0x80000000) continue;

                        /* Keep looping for every byte inside this register */
                        while (CurrentRegister)
                        {
                            /* Read a byte, skip a byte. */
                            RegisterByte = (UCHAR)(CurrentRegister & 0xFF);
                            CurrentRegister >>= 8;
                            if (!RegisterByte) continue;

                            /*
                             * Valid values are from 0x40 (0 bytes) to 0x49
                             * (32MB), or from 0x80 to 0x89 (same size but
                             * 8-way associative.
                             */
                            if (((RegisterByte > 0x40) &&
                                 (RegisterByte <= 0x49)) ||
                                ((RegisterByte > 0x80) &&
                                (RegisterByte <= 0x89)))
                            {
                                /* Mask out only the first nibble */
                                RegisterByte &= 0x0F;

                                /* Set the L2 Cache Size */
                                Pcr->SecondLevelCacheSize = 0x10000 <<
                                                            RegisterByte;
                            }
                        }
                    }
                } while (--CacheRequests);
            }
            break;

        case CPU_AMD:

            /* Check if we support CPUID 0x80000006 */
            __cpuid(Data, 0x80000000);
            if (Data[0] >= 6)
            {
                /* Get 2nd level cache and tlb size */
                __cpuid(Data, 0x80000006);

                /* Set the L2 Cache Size */
                Pcr->SecondLevelCacheSize = (Data[2] & 0xFFFF0000) >> 6;
            }
            break;
    }
}


VOID
FASTCALL
Ki386InitializeTss(IN PKTSS64 Tss,
                   IN PKIDTENTRY Idt,
                   IN PKGDTENTRY Gdt,
                   IN UINT64 Stack)
{
    PKGDTENTRY64 TssEntry;

    /* Initialize the TSS descriptor entry */
    TssEntry = (PVOID)((ULONG64)Gdt + KGDT_TSS);
    TssEntry->Bits.Type = 9;//AMD64_TSS;
    TssEntry->Bits.Dpl = 0;
    TssEntry->Bits.Present = 1;
    TssEntry->Bits.System = 0;
    TssEntry->Bits.LongMode = 0;
    TssEntry->Bits.DefaultBig = 0;
    TssEntry->Bits.Granularity = 0;

    /* Descriptor base is the TSS address */
    TssEntry->BaseLow = (ULONG64)Tss & 0xffff;
    TssEntry->Bits.BaseMiddle = ((ULONG64)Tss >> 16) & 0xff;
    TssEntry->Bits.BaseHigh = ((ULONG64)Tss >> 24) & 0xff;
    TssEntry->BaseUpper = (ULONG64)Tss >> 32;

    /* Set the limit */
    TssEntry->LimitLow = sizeof(KTSS64) -1;
    TssEntry->Bits.LimitHigh = 0;

    /* FIXME: I/O Map? */
    Tss->IoMapBase = 0;

    /* Setup ring 0 stack pointer */
    Tss->Rsp0 = Stack;

    /* Setup a stack for Double Fault Traps */
    Tss->Ist[1] = (ULONG64)KiDoubleFaultStack;

    /* Setup a stack for CheckAbort Traps */
    Tss->Ist[2] = (ULONG64)KiDoubleFaultStack;

    /* Setup a stack for NMI Traps */
    Tss->Ist[3] = (ULONG64)KiDoubleFaultStack;

    /* Load the task register */
    Ke386SetTr(KGDT_TSS);

}

VOID
NTAPI
KeFlushCurrentTb(VOID)
{
    /* Flush the TLB by resetting CR3 */
    __writecr3(__readcr3());
}

VOID
NTAPI
KiRestoreProcessorControlState(PKPROCESSOR_STATE ProcessorState)
{
    /* Restore the CR registers */
    __writecr0(ProcessorState->SpecialRegisters.Cr0);
//    __writecr2(ProcessorState->SpecialRegisters.Cr2);
    __writecr3(ProcessorState->SpecialRegisters.Cr3);
    __writecr4(ProcessorState->SpecialRegisters.Cr4);
    __writecr8(ProcessorState->SpecialRegisters.Cr8);

    /* Restore the DR registers */
    __writedr(0, ProcessorState->SpecialRegisters.KernelDr0);
    __writedr(1, ProcessorState->SpecialRegisters.KernelDr1);
    __writedr(2, ProcessorState->SpecialRegisters.KernelDr2);
    __writedr(3, ProcessorState->SpecialRegisters.KernelDr3);
    __writedr(6, ProcessorState->SpecialRegisters.KernelDr6);
    __writedr(7, ProcessorState->SpecialRegisters.KernelDr7);

    /* Restore GDT, IDT, LDT and TSS */
    __lgdt(&ProcessorState->SpecialRegisters.Gdtr.Limit);
    __lldt(&ProcessorState->SpecialRegisters.Ldtr);
    __ltr(&ProcessorState->SpecialRegisters.Tr);
    __lidt(&ProcessorState->SpecialRegisters.Idtr.Limit);

    __ldmxcsr(&ProcessorState->SpecialRegisters.MxCsr);
//    ProcessorState->SpecialRegisters.DebugControl
//    ProcessorState->SpecialRegisters.LastBranchToRip
//    ProcessorState->SpecialRegisters.LastBranchFromRip
//    ProcessorState->SpecialRegisters.LastExceptionToRip
//    ProcessorState->SpecialRegisters.LastExceptionFromRip

    /* Restore MSRs */
    __writemsr(X86_MSR_GSBASE, ProcessorState->SpecialRegisters.MsrGsBase);
    __writemsr(X86_MSR_KERNEL_GSBASE, ProcessorState->SpecialRegisters.MsrGsSwap);
    __writemsr(X86_MSR_STAR, ProcessorState->SpecialRegisters.MsrStar);
    __writemsr(X86_MSR_LSTAR, ProcessorState->SpecialRegisters.MsrLStar);
    __writemsr(X86_MSR_CSTAR, ProcessorState->SpecialRegisters.MsrCStar);
    __writemsr(X86_MSR_SFMASK, ProcessorState->SpecialRegisters.MsrSyscallMask);

}

VOID
NTAPI
KiSaveProcessorControlState(OUT PKPROCESSOR_STATE ProcessorState)
{
    /* Save the CR registers */
    ProcessorState->SpecialRegisters.Cr0 = __readcr0();
    ProcessorState->SpecialRegisters.Cr2 = __readcr2();
    ProcessorState->SpecialRegisters.Cr3 = __readcr3();
    ProcessorState->SpecialRegisters.Cr4 = __readcr4();
    ProcessorState->SpecialRegisters.Cr8 = __readcr8();

    /* Save the DR registers */
    ProcessorState->SpecialRegisters.KernelDr0 = __readdr(0);
    ProcessorState->SpecialRegisters.KernelDr1 = __readdr(1);
    ProcessorState->SpecialRegisters.KernelDr2 = __readdr(2);
    ProcessorState->SpecialRegisters.KernelDr3 = __readdr(3);
    ProcessorState->SpecialRegisters.KernelDr6 = __readdr(6);
    ProcessorState->SpecialRegisters.KernelDr7 = __readdr(7);

    /* Save GDT, IDT, LDT and TSS */
    __sgdt(&ProcessorState->SpecialRegisters.Gdtr.Limit);
    __sldt(&ProcessorState->SpecialRegisters.Ldtr);
    __str(&ProcessorState->SpecialRegisters.Tr);
    __sidt(&ProcessorState->SpecialRegisters.Idtr.Limit);

//    __stmxcsr(&ProcessorState->SpecialRegisters.MxCsr);
//    ProcessorState->SpecialRegisters.DebugControl = 
//    ProcessorState->SpecialRegisters.LastBranchToRip = 
//    ProcessorState->SpecialRegisters.LastBranchFromRip = 
//    ProcessorState->SpecialRegisters.LastExceptionToRip = 
//    ProcessorState->SpecialRegisters.LastExceptionFromRip = 

    /* Save MSRs */
    ProcessorState->SpecialRegisters.MsrGsBase = __readmsr(X86_MSR_GSBASE);
    ProcessorState->SpecialRegisters.MsrGsSwap = __readmsr(X86_MSR_KERNEL_GSBASE);
    ProcessorState->SpecialRegisters.MsrStar = __readmsr(X86_MSR_STAR);
    ProcessorState->SpecialRegisters.MsrLStar = __readmsr(X86_MSR_LSTAR);
    ProcessorState->SpecialRegisters.MsrCStar = __readmsr(X86_MSR_CSTAR);
    ProcessorState->SpecialRegisters.MsrSyscallMask = __readmsr(X86_MSR_SFMASK);
}


VOID
NTAPI
KiInitializeMachineType(VOID)
{
    /* Set the Machine Type we got from NTLDR */
    KeI386MachineType = KeLoaderBlock->u.I386.MachineType & 0x000FF;
}

VOID
NTAPI
KeFlushEntireTb(IN BOOLEAN Invalid,
                IN BOOLEAN AllProcessors)
{
    UNIMPLEMENTED;
}

KAFFINITY
NTAPI
KeQueryActiveProcessors(VOID)
{
    PAGED_CODE();

    /* Simply return the number of active processors */
    return KeActiveProcessors;
}

NTSTATUS
NTAPI
KeSaveFloatingPointState(OUT PKFLOATING_SAVE Save)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
KeRestoreFloatingPointState(IN PKFLOATING_SAVE Save)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

BOOLEAN
NTAPI
KeFreezeExecution(IN PKTRAP_FRAME TrapFrame,
                  IN PKEXCEPTION_FRAME ExceptionFrame)
{
    ULONG64 Flags = 0;

    /* Disable interrupts and get previous state */
    Flags = __readeflags();
    //Flags = __getcallerseflags();
    _disable();

    /* Save freeze flag */
    KiFreezeFlag = 4;

    /* Save the old IRQL */
    KiOldIrql = KeGetCurrentIrql();

    /* Return whether interrupts were enabled */
    return (Flags & EFLAGS_INTERRUPT_MASK) ? TRUE: FALSE;
}

VOID
NTAPI
KeThawExecution(IN BOOLEAN Enable)
{
    /* Cleanup CPU caches */
    KeFlushCurrentTb();

    /* Re-enable interrupts */
    if (Enable) _enable();
}

BOOLEAN
NTAPI
KeInvalidateAllCaches(VOID)
{
    /* Only supported on Pentium Pro and higher */
    if (KeI386CpuType < 6) return FALSE;

    /* Invalidate all caches */
    __wbinvd();
    return TRUE;
}

/*
 * @implemented
 */
ULONG
NTAPI
KeGetRecommendedSharedDataAlignment(VOID)
{
    /* Return the global variable */
    return KeLargestCacheLine;
}

/*
 * @implemented
 */
VOID
__cdecl
KeSaveStateForHibernate(IN PKPROCESSOR_STATE State)
{
    /* Capture the context */
    RtlCaptureContext(&State->ContextFrame);

    /* Capture the control state */
    KiSaveProcessorControlState(State);
}

/*
 * @implemented
 */
VOID
NTAPI
KeSetDmaIoCoherency(IN ULONG Coherency)
{
    /* Save the coherency globally */
    KiDmaIoCoherency = Coherency;
}
