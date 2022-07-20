/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/cpu.c
 * PURPOSE:         Routines for CPU-level support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#include <xmmintrin.h>

/* GLOBALS *******************************************************************/

/* The TSS to use for Double Fault Traps (INT 0x9) */
UCHAR KiDoubleFaultTSS[KTSS_IO_MAPS];

/* The TSS to use for NMI Fault Traps (INT 0x2) */
UCHAR KiNMITSS[KTSS_IO_MAPS];

/* CPU Features and Flags */
ULONG KeI386CpuType;
ULONG KeI386CpuStep;
ULONG KiFastSystemCallDisable = 0;
ULONG KeI386NpxPresent = TRUE;
ULONG KiMXCsrMask = 0;
ULONG MxcsrFeatureMask = 0;
ULONG KeI386XMMIPresent = 0;
ULONG KeI386FxsrPresent = 0;
ULONG KeI386MachineType;
ULONG Ke386Pae = FALSE;
ULONG Ke386NoExecute = FALSE;
ULONG KeLargestCacheLine = 0x40;
ULONG KeDcacheFlushCount = 0;
ULONG KeIcacheFlushCount = 0;
ULONG KiDmaIoCoherency = 0;
ULONG KePrefetchNTAGranularity = 32;
BOOLEAN KiI386PentiumLockErrataPresent;
BOOLEAN KiSMTProcessorsPresent;

/* The distance between SYSEXIT and IRETD return modes */
UCHAR KiSystemCallExitAdjust;

/* The offset that was applied -- either 0 or the value above */
UCHAR KiSystemCallExitAdjusted;

/* Whether the adjustment was already done once */
BOOLEAN KiFastCallCopyDoneOnce;

/* Flush data */
volatile LONG KiTbFlushTimeStamp;

/* CPU Signatures */
static const CHAR CmpIntelID[]       = "GenuineIntel";
static const CHAR CmpAmdID[]         = "AuthenticAMD";
static const CHAR CmpCyrixID[]       = "CyrixInstead";
static const CHAR CmpTransmetaID[]   = "GenuineTMx86";
static const CHAR CmpCentaurID[]     = "CentaurHauls";
static const CHAR CmpRiseID[]        = "RiseRiseRise";

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

/* FX area alignment size */
#define FXSAVE_ALIGN 15

/* SUPPORT ROUTINES FOR MSVC COMPATIBILITY ***********************************/

/* NSC/Cyrix CPU configuration register index */
#define CX86_CCR1 0xc1

/* NSC/Cyrix CPU indexed register access macros */
static __inline
UCHAR
getCx86(UCHAR reg)
{
    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)0x22, reg);
    return READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)0x23);
}

static __inline
void
setCx86(UCHAR reg, UCHAR data)
{
    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)0x22, reg);
    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)0x23, data);
}

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
ULONG
NTAPI
KiGetCpuVendor(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    CPU_INFO CpuInfo;

    /* Get the Vendor ID */
    KiCpuId(&CpuInfo, 0);

    /* Copy it to the PRCB and null-terminate it */
    *(ULONG*)&Prcb->VendorString[0] = CpuInfo.Ebx;
    *(ULONG*)&Prcb->VendorString[4] = CpuInfo.Edx;
    *(ULONG*)&Prcb->VendorString[8] = CpuInfo.Ecx;
    Prcb->VendorString[12] = 0;

    /* Now check the CPU Type */
    if (!strcmp(Prcb->VendorString, CmpIntelID))
    {
        return CPU_INTEL;
    }
    else if (!strcmp(Prcb->VendorString, CmpAmdID))
    {
        return CPU_AMD;
    }
    else if (!strcmp(Prcb->VendorString, CmpCyrixID))
    {
        DPRINT1("Cyrix CPU support not fully tested!\n");
        return CPU_CYRIX;
    }
    else if (!strcmp(Prcb->VendorString, CmpTransmetaID))
    {
        DPRINT1("Transmeta CPU support not fully tested!\n");
        return CPU_TRANSMETA;
    }
    else if (!strcmp(Prcb->VendorString, CmpCentaurID))
    {
        DPRINT1("Centaur CPU support not fully tested!\n");
        return CPU_CENTAUR;
    }
    else if (!strcmp(Prcb->VendorString, CmpRiseID))
    {
        DPRINT1("Rise CPU support not fully tested!\n");
        return CPU_RISE;
    }

    /* Unknown CPU */
    DPRINT1("%s CPU support not fully tested!\n", Prcb->VendorString);
    return CPU_UNKNOWN;
}

CODE_SEG("INIT")
VOID
NTAPI
KiSetProcessorType(VOID)
{
    CPU_INFO CpuInfo;
    CPU_SIGNATURE CpuSignature;
    BOOLEAN ExtendModel;
    ULONG Stepping, Type;

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
        ULONG Vendor = KiGetCpuVendor();
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

CODE_SEG("INIT")
ULONG
NTAPI
KiGetFeatureBits(VOID)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    ULONG Vendor;
    ULONG FeatureBits = KF_WORKING_PTE;
    CPU_INFO CpuInfo, DummyCpuInfo;
    UCHAR Ccr1;
    BOOLEAN ExtendedCPUID = TRUE;
    ULONG CpuFeatures = 0;

    /* Get the Vendor ID */
    Vendor = KiGetCpuVendor();

    /* Make sure we got a valid vendor ID at least. */
    if (!Vendor) return FeatureBits;

    /* Get the CPUID Info. Features are in Reg[3]. */
    KiCpuId(&CpuInfo, 1);

    /* Set the initial APIC ID */
    Prcb->InitialApicId = (UCHAR)(CpuInfo.Ebx >> 24);

    switch (Vendor)
    {
        /* Intel CPUs */
        case CPU_INTEL:

            /* Check if it's a P6 */
            if (Prcb->CpuType == 6)
            {
                /* Perform the special sequence to get the MicroCode Signature */
                __writemsr(0x8B, 0);
                KiCpuId(&DummyCpuInfo, 1);
                Prcb->UpdateSignature.QuadPart = __readmsr(0x8B);
            }
            else if (Prcb->CpuType == 5)
            {
                /* On P5, enable workaround for the LOCK errata. */
                KiI386PentiumLockErrataPresent = TRUE;
            }

            /* Check for broken P6 with bad SMP PTE implementation */
            if (((CpuInfo.Eax & 0x0FF0) == 0x0610 && (CpuInfo.Eax & 0x000F) <= 0x9) ||
                ((CpuInfo.Eax & 0x0FF0) == 0x0630 && (CpuInfo.Eax & 0x000F) <= 0x4))
            {
                /* Remove support for correct PTE support. */
                FeatureBits &= ~KF_WORKING_PTE;
            }

            /* Check if the CPU is too old to support SYSENTER */
            if ((Prcb->CpuType < 6) ||
                ((Prcb->CpuType == 6) && (Prcb->CpuStep < 0x0303)))
            {
                /* Disable it */
                CpuInfo.Edx &= ~0x800;
            }

            break;

        /* AMD CPUs */
        case CPU_AMD:

            /* Check if this is a K5 or K6. (family 5) */
            if ((CpuInfo.Eax & 0x0F00) == 0x0500)
            {
                /* Get the Model Number */
                switch (CpuInfo.Eax & 0x00F0)
                {
                    /* Model 1: K5 - 5k86 (initial models) */
                    case 0x0010:

                        /* Check if this is Step 0 or 1. They don't support PGE */
                        if ((CpuInfo.Eax & 0x000F) > 0x03) break;

                    /* Model 0: K5 - SSA5 */
                    case 0x0000:

                        /* Model 0 doesn't support PGE at all. */
                        CpuInfo.Edx &= ~0x2000;
                        break;

                    /* Model 8: K6-2 */
                    case 0x0080:

                        /* K6-2, Step 8 and over have support for MTRR. */
                        if ((CpuInfo.Eax & 0x000F) >= 0x8) FeatureBits |= KF_AMDK6MTRR;
                        break;

                    /* Model 9: K6-III
                       Model D: K6-2+, K6-III+ */
                    case 0x0090:
                    case 0x00D0:

                        FeatureBits |= KF_AMDK6MTRR;
                        break;
                }
            }
            else if((CpuInfo.Eax & 0x0F00) < 0x0500)
            {
                /* Families below 5 don't support PGE, PSE or CMOV at all */
                CpuInfo.Edx &= ~(0x08 | 0x2000 | 0x8000);

                /* They also don't support advanced CPUID functions. */
                ExtendedCPUID = FALSE;
            }

            break;

        /* Cyrix CPUs */
        case CPU_CYRIX:

            /* Workaround the "COMA" bug on 6x family of Cyrix CPUs */
            if (Prcb->CpuType == 6 &&
                Prcb->CpuStep <= 1)
            {
                /* Get CCR1 value */
                Ccr1 = getCx86(CX86_CCR1);

                /* Enable the NO_LOCK bit */
                Ccr1 |= 0x10;

                /* Set the new CCR1 value */
                setCx86(CX86_CCR1, Ccr1);
            }

            break;

        /* Transmeta CPUs */
        case CPU_TRANSMETA:

            /* Enable CMPXCHG8B if the family (>= 5), model and stepping (>= 4.2) support it */
            if ((CpuInfo.Eax & 0x0FFF) >= 0x0542)
            {
                __writemsr(0x80860004, __readmsr(0x80860004) | 0x0100);
                FeatureBits |= KF_CMPXCHG8B;
            }

            break;

        /* Centaur, IDT, Rise and VIA CPUs */
        case CPU_CENTAUR:
        case CPU_RISE:

            /* These CPUs don't report the presence of CMPXCHG8B through CPUID.
               However, this feature exists and operates properly without any additional steps. */
            FeatureBits |= KF_CMPXCHG8B;

            break;
    }

    /* Set the current features */
    CpuFeatures = CpuInfo.Edx;

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

    if (CpuFeatures & 0x00000040)
    {
        DPRINT1("Support PAE\n");
    }

    /* Check if the CPU has hyper-threading */
    if (CpuFeatures & 0x10000000)
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

    /* Check if CPUID 0x80000000 is supported */
    if (ExtendedCPUID)
    {
        /* Do the call */
        KiCpuId(&CpuInfo, 0x80000000);
        if ((CpuInfo.Eax & 0xffffff00) == 0x80000000)
        {
            /* Check if CPUID 0x80000001 is supported */
            if (CpuInfo.Eax >= 0x80000001)
            {
                /* Check which extended features are available. */
                KiCpuId(&CpuInfo, 0x80000001);

                /* Check if NX-bit is supported */
                if (CpuInfo.Edx & 0x00100000) FeatureBits |= KF_NX_BIT;

                /* Now handle each features for each CPU Vendor */
                switch (Vendor)
                {
                    case CPU_AMD:
                    case CPU_CENTAUR:
                        if (CpuInfo.Edx & 0x80000000) FeatureBits |= KF_3DNOW;
                        break;
                }
            }
        }
    }

#define print_supported(kf_value) ((FeatureBits & kf_value) ? #kf_value : "")
    DPRINT1("Supported CPU features : %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n",
    print_supported(KF_V86_VIS),
    print_supported(KF_RDTSC),
    print_supported(KF_CR4),
    print_supported(KF_CMOV),
    print_supported(KF_GLOBAL_PAGE),
    print_supported(KF_LARGE_PAGE),
    print_supported(KF_MTRR),
    print_supported(KF_CMPXCHG8B),
    print_supported(KF_MMX),
    print_supported(KF_WORKING_PTE),
    print_supported(KF_PAT),
    print_supported(KF_FXSR),
    print_supported(KF_FAST_SYSCALL),
    print_supported(KF_XMMI),
    print_supported(KF_3DNOW),
    print_supported(KF_AMDK6MTRR),
    print_supported(KF_XMMI64),
    print_supported(KF_DTS),
    print_supported(KF_NX_BIT),
    print_supported(KF_NX_DISABLED),
    print_supported(KF_NX_ENABLED));
#undef print_supported

    /* Return the Feature Bits */
    return FeatureBits;
}

CODE_SEG("INIT")
VOID
NTAPI
KiGetCacheInformation(VOID)
{
    PKIPCR Pcr = (PKIPCR)KeGetPcr();
    CPU_INFO CpuInfo;
    ULONG CacheRequests = 0, i;
    ULONG CurrentRegister;
    UCHAR RegisterByte, Associativity = 0;
    ULONG Size, CacheLine = 64, CurrentSize = 0;
    BOOLEAN FirstPass = TRUE;

    /* Set default L2 size */
    Pcr->SecondLevelCacheSize = 0;

    /* Check the Vendor ID */
    switch (KiGetCpuVendor())
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

                            Size = 0;
                            switch (RegisterByte)
                            {
                                case 0x06:
                                case 0x08:
                                    KePrefetchNTAGranularity = 32;
                                    break;
                                case 0x09:
                                    KePrefetchNTAGranularity = 64;
                                    break;
                                case 0x0a:
                                case 0x0c:
                                    KePrefetchNTAGranularity = 32;
                                    break;
                                case 0x0d:
                                case 0x0e:
                                    KePrefetchNTAGranularity = 64;
                                    break;
                                case 0x1d:
                                    Size = 128 * 1024;
                                    Associativity = 2;
                                    break;
                                case 0x21:
                                    Size = 256 * 1024;
                                    Associativity = 8;
                                    break;
                                case 0x24:
                                    Size = 1024 * 1024;
                                    Associativity = 16;
                                    break;
                                case 0x2c:
                                case 0x30:
                                    KePrefetchNTAGranularity = 64;
                                    break;
                                case 0x41:
                                case 0x42:
                                case 0x43:
                                case 0x44:
                                case 0x45:
                                    Size = (1 << (RegisterByte - 0x41)) * 128 * 1024;
                                    Associativity = 4;
                                    break;
                                case 0x48:
                                    Size = 3 * 1024 * 1024;
                                    Associativity = 12;
                                    break;
                                case 0x49:
                                    Size = 4 * 1024 * 1024;
                                    Associativity = 16;
                                    break;
                                case 0x4e:
                                    Size = 6 * 1024 * 1024;
                                    Associativity = 24;
                                    break;
                                case 0x60:
                                case 0x66:
                                case 0x67:
                                case 0x68:
                                    KePrefetchNTAGranularity = 64;
                                    break;
                                case 0x78:
                                    Size = 1024 * 1024;
                                    Associativity = 4;
                                    break;
                                case 0x79:
                                case 0x7a:
                                case 0x7b:
                                case 0x7c:
                                case 0x7d:
                                    Size = (1 << (RegisterByte - 0x79)) * 128 * 1024;
                                    Associativity = 8;
                                    break;
                                case 0x7f:
                                    Size = 512 * 1024;
                                    Associativity = 2;
                                    break;
                                case 0x80:
                                    Size = 512 * 1024;
                                    Associativity = 8;
                                    break;
                                case 0x82:
                                case 0x83:
                                case 0x84:
                                case 0x85:
                                    Size = (1 << (RegisterByte - 0x82)) * 256 * 1024;
                                    Associativity = 8;
                                    break;
                                case 0x86:
                                    Size = 512 * 1024;
                                    Associativity = 4;
                                    break;
                                case 0x87:
                                    Size = 1024 * 1024;
                                    Associativity = 8;
                                    break;
                                case 0xf0:
                                    KePrefetchNTAGranularity = 64;
                                    break;
                                case 0xf1:
                                    KePrefetchNTAGranularity = 128;
                                    break;
                            }
                            if (Size && (Size / Associativity) > CurrentSize)
                            {
                                /* Set the L2 Cache Size and Associativity */
                                CurrentSize = Size / Associativity;
                                Pcr->SecondLevelCacheSize = Size;
                                Pcr->SecondLevelCacheAssociativity = Associativity;
                            }
                        }
                    }
                } while (--CacheRequests);
            }
            break;

        case CPU_AMD:

            /* Check if we support CPUID 0x80000005 */
            KiCpuId(&CpuInfo, 0x80000000);
            if (CpuInfo.Eax >= 0x80000005)
            {
                /* Get L1 size first */
                KiCpuId(&CpuInfo, 0x80000005);
                KePrefetchNTAGranularity = CpuInfo.Ecx & 0xFF;

                /* Check if we support CPUID 0x80000006 */
                KiCpuId(&CpuInfo, 0x80000000);
                if (CpuInfo.Eax >= 0x80000006)
                {
                    /* Get 2nd level cache and tlb size */
                    KiCpuId(&CpuInfo, 0x80000006);

                    /* Cache line size */
                    CacheLine = CpuInfo.Ecx & 0xFF;

                    /* Hardcode associativity */
                    RegisterByte = (CpuInfo.Ecx >> 12) & 0xFF;
                    switch (RegisterByte)
                    {
                        case 2:
                            Associativity = 2;
                            break;

                        case 4:
                            Associativity = 4;
                            break;

                        case 6:
                            Associativity = 8;
                            break;

                        case 8:
                        case 15:
                            Associativity = 16;
                            break;

                        default:
                            Associativity = 1;
                            break;
                    }

                    /* Compute size */
                    Size = (CpuInfo.Ecx >> 16) << 10;

                    /* Hack for Model 6, Steping 300 */
                    if ((KeGetCurrentPrcb()->CpuType == 6) &&
                        (KeGetCurrentPrcb()->CpuStep == 0x300))
                    {
                        /* Stick 64K in there */
                        Size = 64 * 1024;
                    }

                    /* Set the L2 Cache Size and associativity */
                    Pcr->SecondLevelCacheSize = Size;
                    Pcr->SecondLevelCacheAssociativity = Associativity;
                }
            }
            break;

        case CPU_CYRIX:
        case CPU_TRANSMETA:
        case CPU_CENTAUR:
        case CPU_RISE:

            /* FIXME */
            break;
    }

    /* Set the cache line */
    if (CacheLine > KeLargestCacheLine) KeLargestCacheLine = CacheLine;
    DPRINT1("Prefetch Cache: %lu bytes\tL2 Cache: %lu bytes\tL2 Cache Line: %lu bytes\tL2 Cache Associativity: %lu\n",
            KePrefetchNTAGranularity,
            Pcr->SecondLevelCacheSize,
            KeLargestCacheLine,
            Pcr->SecondLevelCacheAssociativity);
}

CODE_SEG("INIT")
VOID
NTAPI
KiSetCR0Bits(VOID)
{
    ULONG Cr0;

    /* Save current CR0 */
    Cr0 = __readcr0();

    /* If this is a 486, enable Write-Protection */
    if (KeGetCurrentPrcb()->CpuType > 3) Cr0 |= CR0_WP;

    /* Set new Cr0 */
    __writecr0(Cr0);
}

CODE_SEG("INIT")
VOID
NTAPI
KiInitializeTSS2(IN PKTSS Tss,
                 IN PKGDTENTRY TssEntry OPTIONAL)
{
    PUCHAR p;

    /* Make sure the GDT Entry is valid */
    if (TssEntry)
    {
        /* Set the Limit */
        TssEntry->LimitLow = sizeof(KTSS) - 1;
        TssEntry->HighWord.Bits.LimitHi = 0;
    }

    /* Now clear the I/O Map */
    ASSERT(IOPM_COUNT == 1);
    RtlFillMemory(Tss->IoMaps[0].IoMap, IOPM_FULL_SIZE, 0xFF);

    /* Initialize Interrupt Direction Maps */
    p = (PUCHAR)(Tss->IoMaps[0].DirectionMap);
    RtlZeroMemory(p, IOPM_DIRECTION_MAP_SIZE);

    /* Add DPMI support for interrupts */
    p[0] = 4;
    p[3] = 0x18;
    p[4] = 0x18;

    /* Initialize the default Interrupt Direction Map */
    p = Tss->IntDirectionMap;
    RtlZeroMemory(Tss->IntDirectionMap, IOPM_DIRECTION_MAP_SIZE);

    /* Add DPMI support */
    p[0] = 4;
    p[3] = 0x18;
    p[4] = 0x18;
}

VOID
NTAPI
KiInitializeTSS(IN PKTSS Tss)
{
    /* Set an invalid map base */
    Tss->IoMapBase = KiComputeIopmOffset(IO_ACCESS_MAP_NONE);

    /* Disable traps during Task Switches */
    Tss->Flags = 0;

    /* Set LDT and Ring 0 SS */
    Tss->LDT = 0;
    Tss->Ss0 = KGDT_R0_DATA;
}

CODE_SEG("INIT")
VOID
FASTCALL
Ki386InitializeTss(IN PKTSS Tss,
                   IN PKIDTENTRY Idt,
                   IN PKGDTENTRY Gdt)
{
    PKGDTENTRY TssEntry, TaskGateEntry;

    /* Initialize the boot TSS. */
    TssEntry = &Gdt[KGDT_TSS / sizeof(KGDTENTRY)];
    TssEntry->HighWord.Bits.Type = I386_TSS;
    TssEntry->HighWord.Bits.Pres = 1;
    TssEntry->HighWord.Bits.Dpl = 0;
    KiInitializeTSS2(Tss, TssEntry);
    KiInitializeTSS(Tss);

    /* Load the task register */
    Ke386SetTr(KGDT_TSS);

    /* Setup the Task Gate for Double Fault Traps */
    TaskGateEntry = (PKGDTENTRY)&Idt[8];
    TaskGateEntry->HighWord.Bits.Type = I386_TASK_GATE;
    TaskGateEntry->HighWord.Bits.Pres = 1;
    TaskGateEntry->HighWord.Bits.Dpl = 0;
    ((PKIDTENTRY)TaskGateEntry)->Selector = KGDT_DF_TSS;

    /* Initialize the TSS used for handling double faults. */
    Tss = (PKTSS)KiDoubleFaultTSS;
    KiInitializeTSS(Tss);
    Tss->CR3 = __readcr3();
    Tss->Esp0 = KiDoubleFaultStack;
    Tss->Esp = KiDoubleFaultStack;
    Tss->Eip = PtrToUlong(KiTrap08);
    Tss->Cs = KGDT_R0_CODE;
    Tss->Fs = KGDT_R0_PCR;
    Tss->Ss = Ke386GetSs();
    Tss->Es = KGDT_R3_DATA | RPL_MASK;
    Tss->Ds = KGDT_R3_DATA | RPL_MASK;

    /* Setup the Double Trap TSS entry in the GDT */
    TssEntry = &Gdt[KGDT_DF_TSS / sizeof(KGDTENTRY)];
    TssEntry->HighWord.Bits.Type = I386_TSS;
    TssEntry->HighWord.Bits.Pres = 1;
    TssEntry->HighWord.Bits.Dpl = 0;
    TssEntry->BaseLow = (USHORT)((ULONG_PTR)Tss & 0xFFFF);
    TssEntry->HighWord.Bytes.BaseMid = (UCHAR)((ULONG_PTR)Tss >> 16);
    TssEntry->HighWord.Bytes.BaseHi = (UCHAR)((ULONG_PTR)Tss >> 24);
    TssEntry->LimitLow = KTSS_IO_MAPS;

    /* Now setup the NMI Task Gate */
    TaskGateEntry = (PKGDTENTRY)&Idt[2];
    TaskGateEntry->HighWord.Bits.Type = I386_TASK_GATE;
    TaskGateEntry->HighWord.Bits.Pres = 1;
    TaskGateEntry->HighWord.Bits.Dpl = 0;
    ((PKIDTENTRY)TaskGateEntry)->Selector = KGDT_NMI_TSS;

    /* Initialize the actual TSS */
    Tss = (PKTSS)KiNMITSS;
    KiInitializeTSS(Tss);
    Tss->CR3 = __readcr3();
    Tss->Esp0 = KiDoubleFaultStack;
    Tss->Esp = KiDoubleFaultStack;
    Tss->Eip = PtrToUlong(KiTrap02);
    Tss->Cs = KGDT_R0_CODE;
    Tss->Fs = KGDT_R0_PCR;
    Tss->Ss = Ke386GetSs();
    Tss->Es = KGDT_R3_DATA | RPL_MASK;
    Tss->Ds = KGDT_R3_DATA | RPL_MASK;

    /* And its associated TSS Entry */
    TssEntry = &Gdt[KGDT_NMI_TSS / sizeof(KGDTENTRY)];
    TssEntry->HighWord.Bits.Type = I386_TSS;
    TssEntry->HighWord.Bits.Pres = 1;
    TssEntry->HighWord.Bits.Dpl = 0;
    TssEntry->BaseLow = (USHORT)((ULONG_PTR)Tss & 0xFFFF);
    TssEntry->HighWord.Bytes.BaseMid = (UCHAR)((ULONG_PTR)Tss >> 16);
    TssEntry->HighWord.Bytes.BaseHi = (UCHAR)((ULONG_PTR)Tss >> 24);
    TssEntry->LimitLow = KTSS_IO_MAPS;
}

VOID
NTAPI
KeFlushCurrentTb(VOID)
{

#if !defined(_GLOBAL_PAGES_ARE_AWESOME_)

    /* Flush the TLB by resetting CR3 */
    __writecr3(__readcr3());

#else

    /* Check if global pages are enabled */
    if (KeFeatureBits & KF_GLOBAL_PAGE)
    {
        ULONG Cr4;

        /* Disable PGE (Note: may not have been enabled yet) */
        Cr4 = __readcr4();
        __writecr4(Cr4 & ~CR4_PGE);

        /* Flush everything */
        __writecr3(__readcr3());

        /* Re-enable PGE */
        __writecr4(Cr4);
    }
    else
    {
        /* No global pages, resetting CR3 is enough */
        __writecr3(__readcr3());
    }

#endif

}

VOID
NTAPI
KiRestoreProcessorControlState(PKPROCESSOR_STATE ProcessorState)
{
    PKGDTENTRY TssEntry;

    //
    // Restore the CR registers
    //
    __writecr0(ProcessorState->SpecialRegisters.Cr0);
    Ke386SetCr2(ProcessorState->SpecialRegisters.Cr2);
    __writecr3(ProcessorState->SpecialRegisters.Cr3);
    if (KeFeatureBits & KF_CR4) __writecr4(ProcessorState->SpecialRegisters.Cr4);

    //
    // Restore the DR registers
    //
    __writedr(0, ProcessorState->SpecialRegisters.KernelDr0);
    __writedr(1, ProcessorState->SpecialRegisters.KernelDr1);
    __writedr(2, ProcessorState->SpecialRegisters.KernelDr2);
    __writedr(3, ProcessorState->SpecialRegisters.KernelDr3);
    __writedr(6, ProcessorState->SpecialRegisters.KernelDr6);
    __writedr(7, ProcessorState->SpecialRegisters.KernelDr7);

    //
    // Restore GDT and IDT
    //
    Ke386SetGlobalDescriptorTable(&ProcessorState->SpecialRegisters.Gdtr.Limit);
    __lidt(&ProcessorState->SpecialRegisters.Idtr.Limit);

    //
    // Clear the busy flag so we don't crash if we reload the same selector
    //
    TssEntry = (PKGDTENTRY)(ProcessorState->SpecialRegisters.Gdtr.Base +
                            ProcessorState->SpecialRegisters.Tr);
    TssEntry->HighWord.Bytes.Flags1 &= ~0x2;

    //
    // Restore TSS and LDT
    //
    Ke386SetTr(ProcessorState->SpecialRegisters.Tr);
    Ke386SetLocalDescriptorTable(ProcessorState->SpecialRegisters.Ldtr);
}

VOID
NTAPI
KiSaveProcessorControlState(OUT PKPROCESSOR_STATE ProcessorState)
{
    /* Save the CR registers */
    ProcessorState->SpecialRegisters.Cr0 = __readcr0();
    ProcessorState->SpecialRegisters.Cr2 = __readcr2();
    ProcessorState->SpecialRegisters.Cr3 = __readcr3();
    ProcessorState->SpecialRegisters.Cr4 = (KeFeatureBits & KF_CR4) ?
                                           __readcr4() : 0;

    /* Save the DR registers */
    ProcessorState->SpecialRegisters.KernelDr0 = __readdr(0);
    ProcessorState->SpecialRegisters.KernelDr1 = __readdr(1);
    ProcessorState->SpecialRegisters.KernelDr2 = __readdr(2);
    ProcessorState->SpecialRegisters.KernelDr3 = __readdr(3);
    ProcessorState->SpecialRegisters.KernelDr6 = __readdr(6);
    ProcessorState->SpecialRegisters.KernelDr7 = __readdr(7);
    __writedr(7, 0);

    /* Save GDT, IDT, LDT and TSS */
    Ke386GetGlobalDescriptorTable(&ProcessorState->SpecialRegisters.Gdtr.Limit);
    __sidt(&ProcessorState->SpecialRegisters.Idtr.Limit);
    ProcessorState->SpecialRegisters.Tr = Ke386GetTr();
    Ke386GetLocalDescriptorTable(&ProcessorState->SpecialRegisters.Ldtr);
}

CODE_SEG("INIT")
VOID
NTAPI
KiInitializeMachineType(VOID)
{
    /* Set the Machine Type we got from NTLDR */
    KeI386MachineType = KeLoaderBlock->u.I386.MachineType & 0x000FF;
}

CODE_SEG("INIT")
ULONG_PTR
NTAPI
KiLoadFastSyscallMachineSpecificRegisters(IN ULONG_PTR Context)
{
    /* Set CS and ESP */
    __writemsr(0x174, KGDT_R0_CODE);
    __writemsr(0x175, (ULONG_PTR)KeGetCurrentPrcb()->DpcStack);

    /* Set LSTAR */
    __writemsr(0x176, (ULONG_PTR)KiFastCallEntry);
    return 0;
}

CODE_SEG("INIT")
VOID
NTAPI
KiRestoreFastSyscallReturnState(VOID)
{
    /* Check if the CPU Supports fast system call */
    if (KeFeatureBits & KF_FAST_SYSCALL)
    {
        /* Check if it has been disabled */
        if (KiFastSystemCallDisable)
        {
            /* Disable fast system call */
            KeFeatureBits &= ~KF_FAST_SYSCALL;
            KiFastCallExitHandler = KiSystemCallTrapReturn;
            DPRINT1("Support for SYSENTER disabled.\n");
        }
        else
        {
            /* Do an IPI to enable it */
            KeIpiGenericCall(KiLoadFastSyscallMachineSpecificRegisters, 0);

            /* It's enabled, so use the proper exit stub */
            KiFastCallExitHandler = KiSystemCallSysExitReturn;
            DPRINT("Support for SYSENTER detected.\n");
        }
    }
    else
    {
        /* Use the IRET handler */
        KiFastCallExitHandler = KiSystemCallTrapReturn;
        DPRINT1("No support for SYSENTER detected.\n");
    }
}

CODE_SEG("INIT")
ULONG_PTR
NTAPI
Ki386EnableDE(IN ULONG_PTR Context)
{
    /* Enable DE */
    __writecr4(__readcr4() | CR4_DE);
    return 0;
}

CODE_SEG("INIT")
ULONG_PTR
NTAPI
Ki386EnableFxsr(IN ULONG_PTR Context)
{
    /* Enable FXSR */
    __writecr4(__readcr4() | CR4_FXSR);
    return 0;
}

CODE_SEG("INIT")
ULONG_PTR
NTAPI
Ki386EnableXMMIExceptions(IN ULONG_PTR Context)
{
    PKIDTENTRY IdtEntry;

    /* Get the IDT Entry for Interrupt 0x13 */
    IdtEntry = &((PKIPCR)KeGetPcr())->IDT[0x13];

    /* Set it up */
    IdtEntry->Selector = KGDT_R0_CODE;
    IdtEntry->Offset = ((ULONG_PTR)KiTrap13 & 0xFFFF);
    IdtEntry->ExtendedOffset = ((ULONG_PTR)KiTrap13 >> 16) & 0xFFFF;
    ((PKIDT_ACCESS)&IdtEntry->Access)->Dpl = 0;
    ((PKIDT_ACCESS)&IdtEntry->Access)->Present = 1;
    ((PKIDT_ACCESS)&IdtEntry->Access)->SegmentType = I386_INTERRUPT_GATE;

    /* Enable XMMI exceptions */
    __writecr4(__readcr4() | CR4_XMMEXCPT);
    return 0;
}

CODE_SEG("INIT")
VOID
NTAPI
KiI386PentiumLockErrataFixup(VOID)
{
    KDESCRIPTOR IdtDescriptor = {0, 0, 0};
    PKIDTENTRY NewIdt, NewIdt2;
    PMMPTE PointerPte;

    /* Allocate memory for a new IDT */
    NewIdt = ExAllocatePool(NonPagedPool, 2 * PAGE_SIZE);

    /* Put everything after the first 7 entries on a new page */
    NewIdt2 = (PVOID)((ULONG_PTR)NewIdt + PAGE_SIZE - (7 * sizeof(KIDTENTRY)));

    /* Disable interrupts */
    _disable();

    /* Get the current IDT and copy it */
    __sidt(&IdtDescriptor.Limit);
    RtlCopyMemory(NewIdt2,
                  (PVOID)IdtDescriptor.Base,
                  IdtDescriptor.Limit + 1);
    IdtDescriptor.Base = (ULONG)NewIdt2;

    /* Set the new IDT */
    __lidt(&IdtDescriptor.Limit);
    ((PKIPCR)KeGetPcr())->IDT = NewIdt2;

    /* Restore interrupts */
    _enable();

    /* Set the first 7 entries as read-only to produce a fault */
    PointerPte = MiAddressToPte(NewIdt);
    ASSERT(PointerPte->u.Hard.Write == 1);
    PointerPte->u.Hard.Write = 0;
    KeInvalidateTlbEntry(NewIdt);
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

VOID
NTAPI
KiSaveProcessorState(IN PKTRAP_FRAME TrapFrame,
                     IN PKEXCEPTION_FRAME ExceptionFrame)
{
    PKPRCB Prcb = KeGetCurrentPrcb();

    //
    // Save full context
    //
    Prcb->ProcessorState.ContextFrame.ContextFlags = CONTEXT_FULL |
                                                     CONTEXT_DEBUG_REGISTERS;
    KeTrapFrameToContext(TrapFrame, NULL, &Prcb->ProcessorState.ContextFrame);

    //
    // Save control registers
    //
    KiSaveProcessorControlState(&Prcb->ProcessorState);
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
KiIsNpxErrataPresent(VOID)
{
    static double Value1 = 4195835.0, Value2 = 3145727.0;
    INT ErrataPresent;
    ULONG Cr0;

    /* Interrupts have to be disabled here. */
    ASSERT(!(__readeflags() & EFLAGS_INTERRUPT_MASK));

    /* Read CR0 and remove FPU flags */
    Cr0 = __readcr0();
    __writecr0(Cr0 & ~(CR0_MP | CR0_TS | CR0_EM));

    /* Initialize FPU state */
    Ke386FnInit();

    /* Multiply the magic values and divide, we should get the result back */
#ifdef __GNUC__
    __asm__ __volatile__
    (
        "fldl %1\n\t"
        "fdivl %2\n\t"
        "fmull %2\n\t"
        "fldl %1\n\t"
        "fsubp\n\t"
        "fistpl %0\n\t"
        : "=m" (ErrataPresent)
        : "m" (Value1),
          "m" (Value2)
    );
#else
    __asm
    {
        fld Value1
        fdiv Value2
        fmul Value2
        fld Value1
        fsubp st(1), st(0)
        fistp ErrataPresent
    };
#endif

    /* Restore CR0 */
    __writecr0(Cr0);

    /* Return if there's an errata */
    return ErrataPresent != 0;
}

VOID
NTAPI
KiFlushNPXState(IN PFLOATING_SAVE_AREA SaveArea)
{
    ULONG EFlags, Cr0;
    PKTHREAD Thread, NpxThread;
    PFX_SAVE_AREA FxSaveArea;

    /* Save volatiles and disable interrupts */
    EFlags = __readeflags();
    _disable();

    /* Save the PCR and get the current thread */
    Thread = KeGetCurrentThread();

    /* Check if we're already loaded */
    if (Thread->NpxState != NPX_STATE_LOADED)
    {
        /* If there's nothing to load, quit */
        if (!SaveArea)
        {
            /* Restore interrupt state and return */
            __writeeflags(EFlags);
            return;
        }

        /* Need FXSR support for this */
        ASSERT(KeI386FxsrPresent == TRUE);

        /* Check for sane CR0 */
        Cr0 = __readcr0();
        if (Cr0 & (CR0_MP | CR0_TS | CR0_EM))
        {
            /* Mask out FPU flags */
            __writecr0(Cr0 & ~(CR0_MP | CR0_TS | CR0_EM));
        }

        /* Get the NPX thread and check its FPU state */
        NpxThread = KeGetCurrentPrcb()->NpxThread;
        if ((NpxThread) && (NpxThread->NpxState == NPX_STATE_LOADED))
        {
            /* Get the FX frame and store the state there */
            FxSaveArea = KiGetThreadNpxArea(NpxThread);
            Ke386FxSave(FxSaveArea);

            /* NPX thread has lost its state */
            NpxThread->NpxState = NPX_STATE_NOT_LOADED;
        }

        /* Now load NPX state from the NPX area */
        FxSaveArea = KiGetThreadNpxArea(Thread);
        Ke386FxStore(FxSaveArea);
    }
    else
    {
        /* Check for sane CR0 */
        Cr0 = __readcr0();
        if (Cr0 & (CR0_MP | CR0_TS | CR0_EM))
        {
            /* Mask out FPU flags */
            __writecr0(Cr0 & ~(CR0_MP | CR0_TS | CR0_EM));
        }

        /* Get FX frame */
        FxSaveArea = KiGetThreadNpxArea(Thread);
        Thread->NpxState = NPX_STATE_NOT_LOADED;

        /* Save state if supported by CPU */
        if (KeI386FxsrPresent) Ke386FxSave(FxSaveArea);
    }

    /* Now save the FN state wherever it was requested */
    if (SaveArea) Ke386FnSave(SaveArea);

    /* Clear NPX thread */
    KeGetCurrentPrcb()->NpxThread = NULL;

    /* Add the CR0 from the NPX frame */
    Cr0 |= NPX_STATE_NOT_LOADED;
    Cr0 |= FxSaveArea->Cr0NpxState;
    __writecr0(Cr0);

    /* Restore interrupt state */
    __writeeflags(EFlags);
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
KiCoprocessorError(VOID)
{
    PFX_SAVE_AREA NpxArea;

    /* Get the FPU area */
    NpxArea = KiGetThreadNpxArea(KeGetCurrentThread());

    /* Set CR0_TS */
    NpxArea->Cr0NpxState = CR0_TS;
    __writecr0(__readcr0() | CR0_TS);
}

/**
 * @brief
 * Saves the current floating point unit state
 * context of the current calling thread.
 *
 * @param[out] Save
 * The saved floating point context given to the
 * caller at the end of function's operations.
 * The structure whose data contents are opaque
 * to the calling thread.
 *
 * @return
 * Returns STATUS_SUCCESS if the function has
 * successfully completed its operations.
 * STATUS_INSUFFICIENT_RESOURCES is returned
 * if the function couldn't allocate memory
 * for FPU state information.
 *
 * @remarks
 * The function performs a FPU state save
 * in two ways. A normal FPU save (FNSAVE)
 * is performed if the system doesn't have
 * SSE/SSE2, otherwise the function performs
 * a save of FPU, MMX and SSE states save (FXSAVE).
 */
#if defined(__clang__)
__attribute__((__target__("sse")))
#endif
NTSTATUS
NTAPI
KeSaveFloatingPointState(
    _Out_ PKFLOATING_SAVE Save)
{
    PFLOATING_SAVE_CONTEXT FsContext;
    PFX_SAVE_AREA FxSaveAreaFrame;
    PKPRCB CurrentPrcb;

    /* Sanity checks */
    ASSERT(Save);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(KeI386NpxPresent);

    /* Initialize the floating point context */
    FsContext = ExAllocatePoolWithTag(NonPagedPool,
                                      sizeof(FLOATING_SAVE_CONTEXT),
                                      TAG_FLOATING_POINT_CONTEXT);
    if (!FsContext)
    {
        /* Bail out if we failed */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /*
     * Allocate some memory pool for the buffer. The size
     * of this allocated buffer is the FX area plus the
     * alignment requirement needed for FXSAVE as a 16-byte
     * aligned pointer is compulsory in order to save the
     * FPU state.
     */
    FsContext->Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                              sizeof(FX_SAVE_AREA) + FXSAVE_ALIGN,
                                              TAG_FLOATING_POINT_FX);
    if (!FsContext->Buffer)
    {
        /* Bail out if we failed */
        ExFreePoolWithTag(FsContext, TAG_FLOATING_POINT_CONTEXT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /*
     * Now cache the allocated buffer into the save area
     * and align the said area to a 16-byte boundary. Why
     * do we have to do this is because of ExAllocate function.
     * We gave the necessary alignment requirement in the pool
     * allocation size although the function will always return
     * a 8-byte aligned pointer. Aligning the given pointer directly
     * can cause issues when freeing it from memory afterwards. With
     * that said, we have to cache the buffer to the area so that we
     * do not touch or mess the allocated buffer any further.
     */
    FsContext->PfxSaveArea = ALIGN_UP_POINTER_BY(FsContext->Buffer, 16);

    /* Disable interrupts and get the current processor control region */
    _disable();
    CurrentPrcb = KeGetCurrentPrcb();

    /* Store the current thread to context */
    FsContext->CurrentThread = KeGetCurrentThread();

    /*
     * Save the previous NPX thread state registers (aka Numeric
     * Processor eXtension) into the current context so that
     * we are informing the scheduler the current FPU state
     * belongs to this thread.
     */
    if (FsContext->CurrentThread != CurrentPrcb->NpxThread)
    {
        if ((CurrentPrcb->NpxThread != NULL) &&
            (CurrentPrcb->NpxThread->NpxState == NPX_STATE_LOADED))
        {
            /* Get the FX frame */
            FxSaveAreaFrame = KiGetThreadNpxArea(CurrentPrcb->NpxThread);

            /* Save the FPU state */
            Ke386SaveFpuState(FxSaveAreaFrame);

            /* NPX thread has lost its state */
            CurrentPrcb->NpxThread->NpxState = NPX_STATE_NOT_LOADED;
            FxSaveAreaFrame->NpxSavedCpu = 0;
        }

        /* The new NPX thread is the current thread */
        CurrentPrcb->NpxThread = FsContext->CurrentThread;
    }

    /* Perform the save */
    Ke386SaveFpuState(FsContext->PfxSaveArea);

    /* Store the NPX IRQL */
    FsContext->OldNpxIrql = FsContext->CurrentThread->Header.NpxIrql;

    /* Set the current IRQL to NPX */
    FsContext->CurrentThread->Header.NpxIrql = KeGetCurrentIrql();

    /* Initialize the FPU */
    Ke386FnInit();

    /* Enable interrupts back */
    _enable();

    /* Give the saved FPU context to the caller */
    *((PVOID *) Save) = FsContext;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Restores the original FPU state context that has
 * been saved by a API call of KeSaveFloatingPointState.
 * Callers are expected to restore the floating point
 * state by calling this function when they've finished
 * doing FPU operations.
 *
 * @param[in] Save
 * The saved floating point context that is to be given
 * to the function to restore the FPU state.
 *
 * @return
 * Returns STATUS_SUCCESS indicating the function
 * has fully completed its operations.
 */
#if defined(__clang__)
__attribute__((__target__("sse")))
#endif
NTSTATUS
NTAPI
KeRestoreFloatingPointState(
    _In_ PKFLOATING_SAVE Save)
{
    PFLOATING_SAVE_CONTEXT FsContext;

    /* Sanity checks */
    ASSERT(Save);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(KeI386NpxPresent);

    /* Cache the saved FS context */
    FsContext = *((PVOID *) Save);

    /*
     * We have to restore the regular saved FPU
     * state. For this we must first do some
     * validation checks so that we are sure
     * ourselves the state context is saved
     * properly. Check if we are in the same
     * calling thread.
     */
    if (FsContext->CurrentThread != KeGetCurrentThread())
    {
        /*
         * This isn't the thread that saved the
         * FPU state context, crash the system!
         */
        KeBugCheckEx(INVALID_FLOATING_POINT_STATE,
                     0x2,
                     (ULONG_PTR)FsContext->CurrentThread,
                     (ULONG_PTR)KeGetCurrentThread(),
                     0);
    }

    /* Are we under the same NPX interrupt level? */
    if (FsContext->CurrentThread->Header.NpxIrql != KeGetCurrentIrql())
    {
        /* The interrupt level has changed, crash the system! */
        KeBugCheckEx(INVALID_FLOATING_POINT_STATE,
                     0x1,
                     (ULONG_PTR)FsContext->CurrentThread->Header.NpxIrql,
                     (ULONG_PTR)KeGetCurrentIrql(),
                     0);
    }

    /* Disable interrupts */
    _disable();

    /*
     * The saved FPU state context is valid,
     * it's time to restore the state. First,
     * clear FPU exceptions now.
     */
    Ke386ClearFpExceptions();

    /* Restore the state */
    Ke386RestoreFpuState(FsContext->PfxSaveArea);

    /* Give the saved NPX IRQL back to the NPX thread */
    FsContext->CurrentThread->Header.NpxIrql = FsContext->OldNpxIrql;

    /* Enable interrupts back */
    _enable();

    /* We're done, free the allocated area and context */
    ExFreePoolWithTag(FsContext->Buffer, TAG_FLOATING_POINT_FX);
    ExFreePoolWithTag(FsContext, TAG_FLOATING_POINT_CONTEXT);

    return STATUS_SUCCESS;
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

VOID
NTAPI
KiFlushTargetEntireTb(IN PKIPI_CONTEXT PacketContext,
                      IN PVOID Ignored1,
                      IN PVOID Ignored2,
                      IN PVOID Ignored3)
{
    /* Signal this packet as done */
    KiIpiSignalPacketDone(PacketContext);

    /* Flush the TB for the Current CPU */
    KeFlushCurrentTb();
}

/*
 * @implemented
 */
VOID
NTAPI
KeFlushEntireTb(IN BOOLEAN Invalid,
                IN BOOLEAN AllProcessors)
{
    KIRQL OldIrql;
#ifdef CONFIG_SMP
    KAFFINITY TargetAffinity;
    PKPRCB Prcb = KeGetCurrentPrcb();
#endif

    /* Raise the IRQL for the TB Flush */
    OldIrql = KeRaiseIrqlToSynchLevel();

#ifdef CONFIG_SMP
    /* FIXME: Use KiTbFlushTimeStamp to synchronize TB flush */

    /* Get the current processor affinity, and exclude ourselves */
    TargetAffinity = KeActiveProcessors;
    TargetAffinity &= ~Prcb->SetMember;

    /* Make sure this is MP */
    if (TargetAffinity)
    {
        /* Send an IPI TB flush to the other processors */
        KiIpiSendPacket(TargetAffinity,
                        KiFlushTargetEntireTb,
                        NULL,
                        0,
                        NULL);
    }
#endif

    /* Flush the TB for the Current CPU, and update the flush stamp */
    KeFlushCurrentTb();

#ifdef CONFIG_SMP
    /* If this is MP, wait for the other processors to finish */
    if (TargetAffinity)
    {
        /* Sanity check */
        ASSERT(Prcb == KeGetCurrentPrcb());

        /* FIXME: TODO */
        ASSERTMSG("Not yet implemented\n", FALSE);
    }
#endif

    /* Update the flush stamp and return to original IRQL */
    InterlockedExchangeAdd(&KiTbFlushTimeStamp, 1);
    KeLowerIrql(OldIrql);
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

/*
 * @implemented
 */
KAFFINITY
NTAPI
KeQueryActiveProcessors(VOID)
{
    PAGED_CODE();

    /* Simply return the number of active processors */
    return KeActiveProcessors;
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
