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
#include <x86x64/Cpuid.h>
#include <x86x64/Msr.h>
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

typedef union _CPU_SIGNATURE
{
    struct
    {
        ULONG Step : 4;
        ULONG Model : 4;
        ULONG Family : 4;
        ULONG Unused : 4;
        ULONG ExtendedModel : 4;
        ULONG ExtendedFamily : 8;
        ULONG Unused2 : 4;
    };
    ULONG AsULONG;
} CPU_SIGNATURE;

/* FUNCTIONS *****************************************************************/

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

VOID
NTAPI
KiSetProcessorType(VOID)
{
    CPU_INFO CpuInfo;
    CPU_SIGNATURE CpuSignature;
    BOOLEAN ExtendModel;
    ULONG Stepping, Type, Vendor;

    /* This initializes Prcb->CpuVendor */
    Vendor = KiGetCpuVendor();

    /* Do CPUID 1 now */
    KiCpuId(&CpuInfo, 1);

    /*
     * Get the Stepping and Type. The stepping contains both the
     * Model and the Step, while the Type contains the returned Family.
     *
     * For the stepping, we convert this: zzzzzzxy into this: x0y
     */
    CpuSignature.AsULONG = CpuInfo.Eax;
    Stepping = CpuSignature.Model;
    ExtendModel = (CpuSignature.Family == 15);
#if ( (NTDDI_VERSION >= NTDDI_WINXPSP2) && (NTDDI_VERSION < NTDDI_WS03) ) || (NTDDI_VERSION >= NTDDI_WS03SP1)
    if (CpuSignature.Family == 6)
    {
        ExtendModel |= (Vendor == CPU_INTEL);
#if (NTDDI_VERSION >= NTDDI_WIN8)
        ExtendModel |= (Vendor == CPU_CENTAUR);
#endif
    }
#endif
    if (ExtendModel)
    {
        /* Add ExtendedModel to distinguish from non-extended values. */
        Stepping |= (CpuSignature.ExtendedModel << 4);
    }
    Stepping = (Stepping << 8) | CpuSignature.Step;
    Type = CpuSignature.Family;
    if (CpuSignature.Family == 15)
    {
        /* Add ExtendedFamily to distinguish from non-extended values.
         * It must not be larger than 0xF0 to avoid overflow. */
        Type += min(CpuSignature.ExtendedFamily, 0xF0);
    }

    /* Save them in the PRCB */
    KeGetCurrentPrcb()->CpuID = TRUE;
    KeGetCurrentPrcb()->CpuType = (UCHAR)Type;
    KeGetCurrentPrcb()->CpuStep = (USHORT)Stepping;
}

/*!
    \brief Evaluates the KeFeatureFlag bits for the current CPU.

    \return The feature flags for this CPU.

    \see https://www.geoffchappell.com/studies/windows/km/ntoskrnl/structs/kprcb/featurebits.htm

    \todo
     - KF_VIRT_FIRMWARE_ENABLED 0x08000000 (see notes from Geoff Chappell)
     - KF_FPU_LEAKAGE 0x0000020000000000ULL
     - KF_CAT 0x0000100000000000ULL
     - KF_CET_SS 0x0000400000000000ULL
*/
ULONG64
NTAPI
KiGetFeatureBits(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    ULONG Vendor;
    ULONG64 FeatureBits = 0;
    CPUID_SIGNATURE_REGS signature;
    CPUID_VERSION_INFO_REGS VersionInfo;
    CPUID_EXTENDED_FUNCTION_REGS extendedFunction;

    /* Get the Vendor ID */
    Vendor = Prcb->CpuVendor;

    /* Make sure we got a valid vendor ID at least. */
    if (Vendor == CPU_UNKNOWN) return FeatureBits;

    /* Get signature CPUID for the maximum function */
    __cpuid(signature.AsInt32, CPUID_SIGNATURE);

    /* Get the CPUID Info. */
    __cpuid(VersionInfo.AsInt32, CPUID_VERSION_INFO);

    /* Set the initial APIC ID */
    Prcb->InitialApicId = (UCHAR)VersionInfo.Ebx.Bits.InitialLocalApicId;

    /* Convert all CPUID Feature bits into our format */
    if (VersionInfo.Edx.Bits.VME) FeatureBits |= KF_CR4;
    if (VersionInfo.Edx.Bits.PSE) FeatureBits |= KF_LARGE_PAGE | KF_CR4;
    if (VersionInfo.Edx.Bits.TSC) FeatureBits |= KF_RDTSC;
    if (VersionInfo.Edx.Bits.CX8) FeatureBits |= KF_CMPXCHG8B;
    if (VersionInfo.Edx.Bits.SEP) FeatureBits |= KF_FAST_SYSCALL;
    if (VersionInfo.Edx.Bits.MTRR) FeatureBits |= KF_MTRR;
    if (VersionInfo.Edx.Bits.PGE) FeatureBits |= KF_GLOBAL_PAGE | KF_CR4;
    if (VersionInfo.Edx.Bits.CMOV) FeatureBits |= KF_CMOV;
    if (VersionInfo.Edx.Bits.PAT) FeatureBits |= KF_PAT;
    if (VersionInfo.Edx.Bits.DS) FeatureBits |= KF_DTS;
    if (VersionInfo.Edx.Bits.MMX) FeatureBits |= KF_MMX;
    if (VersionInfo.Edx.Bits.FXSR) FeatureBits |= KF_FXSR;
    if (VersionInfo.Edx.Bits.SSE) FeatureBits |= KF_XMMI;
    if (VersionInfo.Edx.Bits.SSE2) FeatureBits |= KF_XMMI64;

    if (VersionInfo.Ecx.Bits.SSE3) FeatureBits |= KF_SSE3;
    if (VersionInfo.Ecx.Bits.SSSE3) FeatureBits |= KF_SSSE3;
    if (VersionInfo.Ecx.Bits.CMPXCHG16B) FeatureBits |= KF_CMPXCHG16B;
    if (VersionInfo.Ecx.Bits.SSE4_1) FeatureBits |= KF_SSE4_1;
    if (VersionInfo.Ecx.Bits.XSAVE) FeatureBits |= KF_XSTATE;
    if (VersionInfo.Ecx.Bits.RDRAND) FeatureBits |= KF_RDRAND;

    /* Check if the CPU has hyper-threading */
    if (VersionInfo.Edx.Bits.HTT)
    {
        /* Set the number of logical CPUs */
        Prcb->LogicalProcessorsPerPhysicalProcessor =
            VersionInfo.Ebx.Bits.MaximumAddressableIdsForLogicalProcessors;
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

    /* Check if CPUID_THERMAL_POWER_MANAGEMENT (0x06) is supported */
    if (signature.MaxLeaf >= CPUID_THERMAL_POWER_MANAGEMENT)
    {
        /* Read CPUID_THERMAL_POWER_MANAGEMENT */
        CPUID_THERMAL_POWER_MANAGEMENT_REGS PowerInfo;
        __cpuid(PowerInfo.AsInt32, CPUID_THERMAL_POWER_MANAGEMENT);

        if (PowerInfo.Undoc.Ecx.ACNT2) FeatureBits |= KF_ACNT2;
    }

    /* Check if CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS (0x07) is supported */
    if (signature.MaxLeaf >= CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS)
    {
        /* Read CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS */
        CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_REGS ExtFlags;
        __cpuidex(ExtFlags.AsInt32,
            CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS,
            CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_SUB_LEAF_INFO);

        if (ExtFlags.Ebx.Bits.SMEP) FeatureBits |= KF_SMEP;
        if (ExtFlags.Ebx.Bits.FSGSBASE) FeatureBits |= KF_RDWRFSGSBASE;
        if (ExtFlags.Ebx.Bits.SMAP) FeatureBits |= KF_SMAP;
    }

    /* Check if CPUID_EXTENDED_STATE (0x0D) is supported */
    if (signature.MaxLeaf >= CPUID_EXTENDED_STATE)
    {
        /* Read CPUID_EXTENDED_STATE */
        CPUID_EXTENDED_STATE_SUB_LEAF_REGS ExtStateSub;
        __cpuidex(ExtStateSub.AsInt32,
            CPUID_EXTENDED_STATE,
            CPUID_EXTENDED_STATE_SUB_LEAF);

        if (ExtStateSub.Eax.Bits.XSAVEOPT) FeatureBits |= KF_XSAVEOPT;
        if (ExtStateSub.Eax.Bits.XSAVES)  FeatureBits |= KF_XSAVES;
    }

    /* Check extended cpuid features */
    __cpuid(extendedFunction.AsInt32, CPUID_EXTENDED_FUNCTION);
    if ((extendedFunction.MaxLeaf & 0xffffff00) == 0x80000000)
    {
        /* Check if CPUID_EXTENDED_CPU_SIG (0x80000001) is supported */
        if (extendedFunction.MaxLeaf >= CPUID_EXTENDED_CPU_SIG)
        {
            /* Read CPUID_EXTENDED_CPU_SIG */
            CPUID_EXTENDED_CPU_SIG_REGS ExtSig;
            __cpuid(ExtSig.AsInt32, CPUID_EXTENDED_CPU_SIG);

            /* Check if NX-bit is supported */
            if (ExtSig.Intel.Edx.Bits.NX) FeatureBits |= KF_NX_BIT;
            if (ExtSig.Intel.Edx.Bits.Page1GB) FeatureBits |= KF_HUGEPAGE;
            if (ExtSig.Intel.Edx.Bits.RDTSCP) FeatureBits |= KF_RDTSCP;

            /* AMD specific */
            if (Vendor == CPU_AMD)
            {
                if (ExtSig.Amd.Edx.Bits.ThreeDNow) FeatureBits |= KF_3DNOW;
            }
        }
    }

    /* Vendor specific */
    if (Vendor == CPU_INTEL)
    {
        FeatureBits |= KF_GENUINE_INTEL;

        /* Check for models that support LBR */
        if (VersionInfo.Eax.Bits.FamilyId == 6)
        {
            if ((VersionInfo.Eax.Bits.Model == 15) ||
                (VersionInfo.Eax.Bits.Model == 22) ||
                (VersionInfo.Eax.Bits.Model == 23) ||
                (VersionInfo.Eax.Bits.Model == 26))
            {
                FeatureBits |= KF_BRANCH;
            }
        }

        /* Check if VMX is available */
        if (VersionInfo.Ecx.Bits.VMX)
        {
            /* Read PROCBASED ctls and check if secondary are allowed */
            MSR_IA32_VMX_PROCBASED_CTLS_REGISTER ProcBasedCtls;
            ProcBasedCtls.Uint64 = __readmsr(MSR_IA32_VMX_PROCBASED_CTLS);
            if (ProcBasedCtls.Bits.Allowed1.ActivateSecondaryControls)
            {
                /* Read secondary controls and check if EPT is allowed */
                MSR_IA32_VMX_PROCBASED_CTLS2_REGISTER ProcBasedCtls2;
                ProcBasedCtls2.Uint64 = __readmsr(MSR_IA32_VMX_PROCBASED_CTLS2);
                if (ProcBasedCtls2.Bits.Allowed1.EPT)
                    FeatureBits |= KF_SLAT;
            }
        }
    }
    else if (Vendor == CPU_AMD)
    {
        FeatureBits |= KF_AUTHENTICAMD;
        FeatureBits |= KF_BRANCH;

        /* Check extended cpuid features */
        if ((extendedFunction.MaxLeaf & 0xffffff00) == 0x80000000)
        {
            /* Check if CPUID_AMD_SVM_FEATURES (0x8000000A) is supported */
            if (extendedFunction.MaxLeaf >= CPUID_AMD_SVM_FEATURES)
            {
                /* Read CPUID_AMD_SVM_FEATURES and check if Nested Paging is available */
                CPUID_AMD_SVM_FEATURES_REGS SvmFeatures;
                __cpuid(SvmFeatures.AsInt32, CPUID_AMD_SVM_FEATURES);
                if (SvmFeatures.Edx.Bits.NP) FeatureBits |= KF_SLAT;
            }
        }
    }

    /* Return the Feature Bits */
    return FeatureBits;
}

#if DBG
VOID
KiReportCpuFeatures(IN PKPRCB Prcb)
{
    ULONG CpuFeatures = 0;
    CPU_INFO CpuInfo;

    if (Prcb->CpuVendor)
    {
        KiCpuId(&CpuInfo, 1);
        CpuFeatures = CpuInfo.Edx;
    }

    DPRINT1("Supported CPU features: ");

#define print_kf_bit(kf_value) if (Prcb->FeatureBits & kf_value) DbgPrint(#kf_value " ")
    print_kf_bit(KF_SMEP);
    print_kf_bit(KF_RDTSC);
    print_kf_bit(KF_CR4);
    print_kf_bit(KF_CMOV);
    print_kf_bit(KF_GLOBAL_PAGE);
    print_kf_bit(KF_LARGE_PAGE);
    print_kf_bit(KF_MTRR);
    print_kf_bit(KF_CMPXCHG8B);
    print_kf_bit(KF_MMX);
    print_kf_bit(KF_DTS);
    print_kf_bit(KF_PAT);
    print_kf_bit(KF_FXSR);
    print_kf_bit(KF_FAST_SYSCALL);
    print_kf_bit(KF_XMMI);
    print_kf_bit(KF_3DNOW);
    print_kf_bit(KF_XSAVEOPT);
    print_kf_bit(KF_XMMI64);
    print_kf_bit(KF_BRANCH);
    print_kf_bit(KF_00040000);
    print_kf_bit(KF_SSE3);
    print_kf_bit(KF_CMPXCHG16B);
    print_kf_bit(KF_AUTHENTICAMD);
    print_kf_bit(KF_ACNT2);
    print_kf_bit(KF_XSTATE);
    print_kf_bit(KF_GENUINE_INTEL);
    print_kf_bit(KF_SLAT);
    print_kf_bit(KF_VIRT_FIRMWARE_ENABLED);
    print_kf_bit(KF_RDWRFSGSBASE);
    print_kf_bit(KF_NX_BIT);
    print_kf_bit(KF_NX_DISABLED);
    print_kf_bit(KF_NX_ENABLED);
    print_kf_bit(KF_RDRAND);
    print_kf_bit(KF_SMAP);
    print_kf_bit(KF_RDTSCP);
    print_kf_bit(KF_HUGEPAGE);
    print_kf_bit(KF_XSAVES);
    print_kf_bit(KF_FPU_LEAKAGE);
    print_kf_bit(KF_CAT);
    print_kf_bit(KF_CET_SS);
    print_kf_bit(KF_SSSE3);
    print_kf_bit(KF_SSE4_1);
    print_kf_bit(KF_SSE4_2);
#undef print_kf_bit

#define print_cf(cpu_flag) if (CpuFeatures & cpu_flag) DbgPrint(#cpu_flag " ")
    print_cf(X86_FEATURE_PAE);
    print_cf(X86_FEATURE_HT);
#undef print_cf

    DbgPrint("\n");
}
#endif // DBG

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

    _mm_setcsr(ProcessorState->SpecialRegisters.MxCsr);
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

    ProcessorState->SpecialRegisters.MxCsr = _mm_getcsr();
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
KiSaveProcessorState(
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ PKEXCEPTION_FRAME ExceptionFrame)
{
    PKPRCB Prcb = KeGetCurrentPrcb();

    /* Save all context */
    Prcb->ProcessorState.ContextFrame.ContextFlags = CONTEXT_ALL;
    KeTrapFrameToContext(TrapFrame, ExceptionFrame, &Prcb->ProcessorState.ContextFrame);

    /* Save control registers */
    KiSaveProcessorControlState(&Prcb->ProcessorState);
}

VOID
NTAPI
KiRestoreProcessorState(
    _Out_ PKTRAP_FRAME TrapFrame,
    _Out_ PKEXCEPTION_FRAME ExceptionFrame)
{
    PKPRCB Prcb = KeGetCurrentPrcb();

    /* Restore all context */
    KeContextToTrapFrame(&Prcb->ProcessorState.ContextFrame,
                         ExceptionFrame,
                         TrapFrame,
                         CONTEXT_ALL,
                         TrapFrame->PreviousMode);

    /* Restore control registers */
    KiRestoreProcessorControlState(&Prcb->ProcessorState);
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
