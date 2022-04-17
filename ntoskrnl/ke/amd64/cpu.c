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

/* GLOBALS *******************************************************************/

/* The Boot TSS */
KTSS64 KiBootTss;

/* CPU Features and Flags */
ULONG KeI386CpuType;
ULONG KeI386CpuStep;
ULONG KeI386MachineType;
ULONG KeI386NpxPresent = 1;
ULONG KeLargestCacheLine = 0x40;
ULONG KiDmaIoCoherency = 0;
BOOLEAN KiSMTProcessorsPresent;

/* Flush data */
volatile LONG KiTbFlushTimeStamp;

/* CPU Signatures */
static const CHAR CmpIntelID[]       = "GenuineIntel";
static const CHAR CmpAmdID[]         = "AuthenticAMD";
static const CHAR CmpCentaurID[]     = "CentaurHauls";

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KiSetProcessorType(VOID)
{
    CPU_INFO CpuInfo;
    ULONG Stepping, Type;

    /* Do CPUID 1 now */
    KiCpuId(&CpuInfo, 1);

    /*
     * Get the Stepping and Type. The stepping contains both the
     * Model and the Step, while the Type contains the returned Type.
     * We ignore the family.
     *
     * For the stepping, we convert this: zzzzzzxy into this: x0y
     */
    Stepping = CpuInfo.Eax & 0xF0;
    Stepping <<= 4;
    Stepping += (CpuInfo.Eax & 0xFF);
    Stepping &= 0xF0F;
    Type = CpuInfo.Eax & 0xF00;
    Type >>= 8;

    /* Save them in the PRCB */
    KeGetCurrentPrcb()->CpuID = TRUE;
    KeGetCurrentPrcb()->CpuType = (UCHAR)Type;
    KeGetCurrentPrcb()->CpuStep = (USHORT)Stepping;
}

ULONG
NTAPI
KiGetCpuVendor(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    CPU_INFO CpuInfo;

    /* Get the Vendor ID and null-terminate it */
    KiCpuId(&CpuInfo, 0);

    /* Copy it to the PRCB and null-terminate it */
    *(ULONG*)&Prcb->VendorString[0] = CpuInfo.Ebx;
    *(ULONG*)&Prcb->VendorString[4] = CpuInfo.Edx;
    *(ULONG*)&Prcb->VendorString[8] = CpuInfo.Ecx;
    Prcb->VendorString[12] = 0;

    /* Now check the CPU Type */
    if (!strcmp((PCHAR)Prcb->VendorString, CmpIntelID))
    {
        Prcb->CpuVendor = CPU_INTEL;
    }
    else if (!strcmp((PCHAR)Prcb->VendorString, CmpAmdID))
    {
        Prcb->CpuVendor = CPU_AMD;
    }
    else if (!strcmp((PCHAR)Prcb->VendorString, CmpCentaurID))
    {
        DPRINT1("VIA CPUs not fully supported\n");
        Prcb->CpuVendor = CPU_VIA;
    }
    else
    {
        /* Invalid CPU */
        DPRINT1("%s CPU support not fully tested!\n", Prcb->VendorString);
        Prcb->CpuVendor = CPU_UNKNOWN;
    }

    return Prcb->CpuVendor;
}

ULONG
NTAPI
KiGetFeatureBits(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    ULONG Vendor;
    ULONG FeatureBits = KF_WORKING_PTE;
    CPU_INFO CpuInfo;

    /* Get the Vendor ID */
    Vendor = KiGetCpuVendor();

    /* Make sure we got a valid vendor ID at least. */
    if (!Vendor) return FeatureBits;

    /* Get the CPUID Info. */
    KiCpuId(&CpuInfo, 1);

    /* Set the initial APIC ID */
    Prcb->InitialApicId = (UCHAR)(CpuInfo.Ebx >> 24);

    /* Convert all CPUID Feature bits into our format */
    if (CpuInfo.Edx & X86_FEATURE_VME) FeatureBits |= KF_V86_VIS | KF_CR4;
    if (CpuInfo.Edx & X86_FEATURE_PSE) FeatureBits |= KF_LARGE_PAGE | KF_CR4;
    if (CpuInfo.Edx & X86_FEATURE_TSC) FeatureBits |= KF_RDTSC;
    if (CpuInfo.Edx & X86_FEATURE_CX8) FeatureBits |= KF_CMPXCHG8B;
    if (CpuInfo.Edx & X86_FEATURE_SYSCALL) FeatureBits |= KF_FAST_SYSCALL;
    if (CpuInfo.Edx & X86_FEATURE_MTTR) FeatureBits |= KF_MTRR;
    if (CpuInfo.Edx & X86_FEATURE_PGE) FeatureBits |= KF_GLOBAL_PAGE | KF_CR4;
    if (CpuInfo.Edx & X86_FEATURE_CMOV) FeatureBits |= KF_CMOV;
    if (CpuInfo.Edx & X86_FEATURE_PAT) FeatureBits |= KF_PAT;
    if (CpuInfo.Edx & X86_FEATURE_DS) FeatureBits |= KF_DTS;
    if (CpuInfo.Edx & X86_FEATURE_MMX) FeatureBits |= KF_MMX;
    if (CpuInfo.Edx & X86_FEATURE_FXSR) FeatureBits |= KF_FXSR;
    if (CpuInfo.Edx & X86_FEATURE_SSE) FeatureBits |= KF_XMMI;
    if (CpuInfo.Edx & X86_FEATURE_SSE2) FeatureBits |= KF_XMMI64;

    if (CpuInfo.Ecx & X86_FEATURE_SSE3) FeatureBits |= KF_SSE3;
    //if (CpuInfo.Ecx & X86_FEATURE_MONITOR) FeatureBits |= KF_MONITOR;
    //if (CpuInfo.Ecx & X86_FEATURE_SSSE3) FeatureBits |= KF_SSE3SUP;
    if (CpuInfo.Ecx & X86_FEATURE_CX16) FeatureBits |= KF_CMPXCHG16B;
    //if (CpuInfo.Ecx & X86_FEATURE_SSE41) FeatureBits |= KF_SSE41;
    //if (CpuInfo.Ecx & X86_FEATURE_POPCNT) FeatureBits |= KF_POPCNT;
    if (CpuInfo.Ecx & X86_FEATURE_XSAVE) FeatureBits |= KF_XSTATE;

    /* Check if the CPU has hyper-threading */
    if (CpuInfo.Edx & X86_FEATURE_HT)
    {
        /* Set the number of logical CPUs */
        Prcb->LogicalProcessorsPerPhysicalProcessor = (UCHAR)(CpuInfo.Ebx >> 16);
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
    KiCpuId(&CpuInfo, 0x80000000);
    if ((CpuInfo.Eax & 0xffffff00) == 0x80000000)
    {
        /* Check if CPUID 0x80000001 is supported */
        if (CpuInfo.Eax >= 0x80000001)
        {
            /* Check which extended features are available. */
            KiCpuId(&CpuInfo, 0x80000001);

            /* Check if NX-bit is supported */
            if (CpuInfo.Edx & X86_FEATURE_NX) FeatureBits |= KF_NX_BIT;

            /* Now handle each features for each CPU Vendor */
            switch (Vendor)
            {
                case CPU_AMD:
                    if (CpuInfo.Edx & 0x80000000) FeatureBits |= KF_3DNOW;
                    break;
            }
        }
    }

    /* Return the Feature Bits */
    return FeatureBits;
}

VOID
NTAPI
KiGetCacheInformation(VOID)
{
    PKIPCR Pcr = (PKIPCR)KeGetPcr();
    ULONG Vendor;
    ULONG CacheRequests = 0, i;
    ULONG CurrentRegister;
    UCHAR RegisterByte;
    BOOLEAN FirstPass = TRUE;
    CPU_INFO CpuInfo;

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
            KiCpuId(&CpuInfo, 0);
            if (CpuInfo.Eax >= 2)
            {
                /* We need to loop for the number of times CPUID will tell us to */
                do
                {
                    /* Do the CPUID call */
                    KiCpuId(&CpuInfo, 2);

                    /* Check if it was the first call */
                    if (FirstPass)
                    {
                        /*
                         * The number of times to loop is the first byte. Read
                         * it and then destroy it so we don't get confused.
                         */
                        CacheRequests = CpuInfo.Eax & 0xFF;
                        CpuInfo.Eax &= 0xFFFFFF00;

                        /* Don't go over this again */
                        FirstPass = FALSE;
                    }

                    /* Loop all 4 registers */
                    for (i = 0; i < 4; i++)
                    {
                        /* Get the current register */
                        CurrentRegister = CpuInfo.AsUINT32[i];

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
            KiCpuId(&CpuInfo, 0x80000000);
            if (CpuInfo.Eax >= 6)
            {
                /* Get 2nd level cache and tlb size */
                KiCpuId(&CpuInfo, 0x80000006);

                /* Set the L2 Cache Size */
                Pcr->SecondLevelCacheSize = (CpuInfo.Ecx & 0xFFFF0000) >> 6;
            }
            break;
    }
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
//    __lldt(&ProcessorState->SpecialRegisters.Ldtr);
//    __ltr(&ProcessorState->SpecialRegisters.Tr);
    __lidt(&ProcessorState->SpecialRegisters.Idtr.Limit);

//    __ldmxcsr(&ProcessorState->SpecialRegisters.MxCsr); // FIXME
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
KeFlushEntireTb(IN BOOLEAN Invalid,
                IN BOOLEAN AllProcessors)
{
    KIRQL OldIrql;

    // FIXME: halfplemented
    /* Raise the IRQL for the TB Flush */
    OldIrql = KeRaiseIrqlToSynchLevel();

    /* Flush the TB for the Current CPU, and update the flush stamp */
    KeFlushCurrentTb();

    /* Update the flush stamp and return to original IRQL */
    InterlockedExchangeAdd(&KiTbFlushTimeStamp, 1);
    KeLowerIrql(OldIrql);

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
KxSaveFloatingPointState(OUT PKFLOATING_SAVE FloatingState)
{
    UNREFERENCED_PARAMETER(FloatingState);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KxRestoreFloatingPointState(IN PKFLOATING_SAVE FloatingState)
{
    UNREFERENCED_PARAMETER(FloatingState);
    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
KeInvalidateAllCaches(VOID)
{
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
