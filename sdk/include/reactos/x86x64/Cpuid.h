/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Provides CPUID structure definitions
 * COPYRIGHT:   Copyright 2023 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define CHAR8 char

#include "Intel/Cpuid.h"
#include "Amd/Cpuid.h"

// CPUID_SIGNATURE (0)
typedef union
{
    INT32 AsInt32[4];
    struct
    {
        UINT32 MaxLeaf;
        CHAR SignatureScrambled[12];
    };
} CPUID_SIGNATURE_REGS;

// CPUID_VERSION_INFO (1)
typedef union
{
    INT32 AsInt32[4];
    struct
    {
        CPUID_VERSION_INFO_EAX Eax;
        CPUID_VERSION_INFO_EBX Ebx;
        CPUID_VERSION_INFO_ECX Ecx;
        CPUID_VERSION_INFO_EDX Edx;
    };
} CPUID_VERSION_INFO_REGS;

// CPUID_CACHE_INFO (2)
typedef union
{
    INT32 AsInt32[4];
    struct
    {
        CPUID_CACHE_INFO_CACHE_TLB Eax;
        CPUID_CACHE_INFO_CACHE_TLB Ebx;
        CPUID_CACHE_INFO_CACHE_TLB Ecx;
        CPUID_CACHE_INFO_CACHE_TLB Edx;
    };
    CPUID_CACHE_INFO_CACHE_TLB Regs[4];
} CPUID_CACHE_INFO_REGS;

// CPUID_CACHE_PARAMS (4)
typedef union
{
    INT32 AsInt32[4];
    struct
    {
        CPUID_CACHE_PARAMS_EAX Eax;
        CPUID_CACHE_PARAMS_EBX Ebx;
        UINT32 NumberOfSets;
        CPUID_CACHE_PARAMS_EDX Edx;
    };
} CPUID_CACHE_PARAMS_REGS;

// CPUID_THERMAL_POWER_MANAGEMENT (6)
typedef union
{
    INT32 AsInt32[4];
    struct
    {
        CPUID_THERMAL_POWER_MANAGEMENT_EAX Eax;
        CPUID_THERMAL_POWER_MANAGEMENT_EBX Ebx;
        CPUID_THERMAL_POWER_MANAGEMENT_ECX Ecx;
        UINT32 ReservedEdx;
    };
    struct
    {
        UINT32 Eax;
        UINT32 Ebx;
        struct
        {
            UINT32 HardwareCoordinationFeedback : 1;
            UINT32 ACNT2 : 1; // See https://en.wikipedia.org/wiki/CPUID
        } Ecx;
    } Undoc;
} CPUID_THERMAL_POWER_MANAGEMENT_REGS;

// CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS (0x07)
typedef union
{
    INT32 AsInt32[4];
    struct
    {
        UINT32 Eax;
        CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_EBX Ebx;
        CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_ECX Ecx;
        CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_EDX Edx;
    };
} CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_REGS;

// CPUID_EXTENDED_STATE (0x0D)
// CPUID_EXTENDED_STATE_MAIN_LEAF (0x00)
typedef union
{
    INT32 AsInt32[4];
    struct
    {
        CPUID_EXTENDED_STATE_MAIN_LEAF_EAX Eax;
        UINT32 Ebx;
        UINT32 Ecx;
        UINT32 Edx;
    };
} CPUID_EXTENDED_STATE_MAIN_LEAF_REGS;

// CPUID_EXTENDED_STATE (0x0D)
// CPUID_EXTENDED_STATE_SUB_LEAF (0x01)
typedef union
{
    INT32 AsInt32[4];
    struct
    {
        CPUID_EXTENDED_STATE_SUB_LEAF_EAX Eax;
        struct
        {
            UINT32 XSaveAreaSize; // The size in bytes of the XSAVE area containing all states enabled by XCRO | IA32_XSS.
        } Ebx;
        CPUID_EXTENDED_STATE_SUB_LEAF_ECX Ecx;
        UINT32 Edx; // Reports the supported bits of the upper 32 bits of the IA32_XSS MSR. IA32_XSS[n + 32] can be set to 1 only if EDX[n] is 1.
    };
} CPUID_EXTENDED_STATE_SUB_LEAF_REGS;

// CPUID_EXTENDED_STATE (0x0D)
// CPUID_EXTENDED_STATE_SIZE_OFFSET (0x02 .. 0x1F)
typedef union
{
    INT32 AsInt32[4];
    struct
    {
        UINT32 Size;
        UINT32 Offset;
        CPUID_EXTENDED_STATE_SIZE_OFFSET_ECX Ecx;
        UINT32 Edx;
    };
} CPUID_EXTENDED_STATE_SIZE_OFFSET_REGS;

// CPUID_EXTENDED_FUNCTION (0x80000000)
typedef union
{
    INT32 AsInt32[4];
    struct
    {
        UINT32 MaxLeaf;
        UINT32 ReservedEbx;
        UINT32 ReservedEcx;
        UINT32 ReservedEdx;
    };
} CPUID_EXTENDED_FUNCTION_REGS;

// CPUID_EXTENDED_CPU_SIG (0x80000001)
typedef union
{
    INT32 AsInt32[4];
    struct
    {
        UINT32 Signature;
        UINT32 ReservedEbx;
        CPUID_EXTENDED_CPU_SIG_ECX Ecx;
        CPUID_EXTENDED_CPU_SIG_EDX Edx;
    } Intel;
    struct
    {
        CPUID_AMD_EXTENDED_CPU_SIG_EAX Eax;
        CPUID_AMD_EXTENDED_CPU_SIG_EBX Ebx;
        CPUID_AMD_EXTENDED_CPU_SIG_ECX Ecx;
        CPUID_AMD_EXTENDED_CPU_SIG_EDX Edx;
    } Amd;
} CPUID_EXTENDED_CPU_SIG_REGS;


// https://kib.kiev.ua/x86docs/AMD/AMD-CPUID-Spec/25481-r2.34.pdf#G3.2106230
#define CPUID_L1_TLB_INFO 0x80000005

// https://docs.amd.com/v/u/en-US/24594_3.38_APM_Vol3 (E.4.4 Function 8000_0005h—L1 Cache and TLB Information)
typedef union
{
    struct
    {
        UINT32 L1ITlb2and4MSize : 8; ///< EAX[7..0] - ITLB number of entries for 2 MB and 4 MB pages
        UINT32 L1ITlb2and4MAssoc : 8; ///< EAX[15..8] - ITLB associativity for 2 MB and 4 MB pages (0xFF = Fully associative)
        UINT32 L1DTlb2and4MSize : 8; ///< EAX[23..16] - DTLB number of entries for 2 MB and 4 MB pages
        UINT32 L1DTlb2and4MAssoc : 8; ///< EAX[31..24] - DTLB associativity for 2 MB and 4 MB pages (0xFF = Fully associative)
    } Bits;
    UINT32 Uint32;
} CPUID_AMD_L1_TLB_INFO_EAX;

typedef union
{
    struct
    {
        UINT32 L1ITlb4KSize : 8; ///< EBX[7..0] - ITLB number of entries for 4 KB pages
        UINT32 L1ITlb4KAssoc : 8; ///< EBX[15..8] - ITLB associativity for 4 KB pages (0xFF = Fully associative)
        UINT32 L1DTlb4KSize : 8; ///< EBX[23..16] - DTLB number of entries for 4 KB pages
        UINT32 L1DTlb4KAssoc : 8; ///< EBX[31..24] - DTLB associativity for 4 KB pages (0xFF = Fully associative)
    } Bits;
    UINT32 Uint32;
} CPUID_AMD_L1_TLB_INFO_EBX;

typedef union
{
    struct
    {
        UINT32 L1DcLineSize : 8; ///< ECX[7..0] - L1 data cache line size in bytes
        UINT32 L1DcLinesPerTag : 8; ///< ECX[15..8]- L1 data cache lines per tag
        UINT32 L1DcAssoc : 8; ///< ECX[23..16] - L1 data cache associativity (0xFF = Fully associative)
        UINT32 L1DcSize : 8; ///< ECX[31..24] - L1 data cache size in KB
    } Bits;
    UINT32 Uint32;
} CPUID_AMD_L1_TLB_INFO_ECX;

typedef union
{
    struct
    {
        UINT32 L1IcLineSize : 8; ///< EDX[7..0] - L1 instruction cache line size in bytes
        UINT32 L1IcLinesPerTag : 8; ///< EDX[15..8] - L1 instruction cache lines per tag
        UINT32 L1IcAssoc : 8; ///< EDX[23..16] - L1 instruction cache associativity (0xFF = Fully associative)
        UINT32 L1IcSize : 8; ///< EDX[31..24] - L1 instruction cache size KB
    } Bits;
    UINT32 Uint32;
} CPUID_AMD_L1_TLB_INFO_EDX;

typedef union
{
    INT32 AsInt32[4];
    struct
    {
        UINT32 ReservedEax;
        UINT32 ReservedEbx;
        UINT32 ReservedEcx;
        UINT32 ReservedEdx;
    } Intel;
    struct
    {
        CPUID_AMD_L1_TLB_INFO_EAX Eax;
        CPUID_AMD_L1_TLB_INFO_EBX Ebx;
        CPUID_AMD_L1_TLB_INFO_ECX Ecx;
        CPUID_AMD_L1_TLB_INFO_EDX Edx;
    } Amd;
    struct
    {
        // Same as AMD, but no TLB info in EAX
        UINT32 Eax; // Reserved
        CPUID_AMD_L1_TLB_INFO_EBX Ebx;
        CPUID_AMD_L1_TLB_INFO_ECX Ecx;
        CPUID_AMD_L1_TLB_INFO_EDX Edx;
    } Via;
} CPUID_L1_TLB_INFO_REGS;

// CPUID_EXTENDED_CACHE_INFO (0x80000006)
// https://docs.amd.com/v/u/en-US/24594_3.38_APM_Vol3 (E.4.5 Function 8000_0006h—L2 Cache and TLB and L3 Cache Information)
typedef union
{
    struct
    {
        UINT32 L2ITlb2and4MSize : 12; ///< EAX[11..0] - ITLB number of entries for 2 MB and 4 MB pages
        UINT32 L2ITlb2and4MAssoc : 4; ///< EAX[15..12] - ITLB associativity for 2 MB and 4 MB pages
        UINT32 L2DTlb2and4MSize : 12; ///< EAX[27..16] - DTLB number of entries for 2 MB and 4 MB pages
        UINT32 L2DTlb2and4MAssoc : 4; ///< EAX[31..28] - DTLB associativity for 2 MB and 4 MB pages
    } Bits;
    UINT32 Uint32;
} CPUID_AMD_EXTENDED_CACHE_INFO_EAX;

typedef union
{
    struct
    {
        UINT32 L2ITlb4KSize : 12; ///< EBX[11..0] - ITLB number of entries for 4 KB pages
        UINT32 L2ITlb4KAssoc : 4; ///< EBX[15..12] - ITLB associativity for 4 KB pages
        UINT32 L2DTlb4KSize : 12; ///< EBX[27..16] - DTLB number of entries for 4 KB pages
        UINT32 L2DTlb4KAssoc : 4; ///< EBX[31..28] - DTLB associativity for 4 KB pages
    } Bits;
    UINT32 Uint32;
} CPUID_AMD_EXTENDED_CACHE_INFO_EBX;

typedef union
{
    struct
    {
        UINT32 L2LineSize : 8; ///< ECX[7..0] - L2 cache line size in bytes
        UINT32 L2LinesPerTag : 4; ///< ECX[11..8] - L2 cache lines per tag
        UINT32 L2Assoc : 4; ///< ECX[15..12] - L2 cache associativity
        UINT32 L2Size : 16; ///< ECX[31..16] - L2 cache size in KB
    } Bits;
    UINT32 Uint32;
} CPUID_AMD_EXTENDED_CACHE_INFO_ECX;

typedef union
{
    struct
    {
        UINT32 L3LineSize : 8; ///< EDX[7..0] - L3 cache line size in bytes
        UINT32 L3LinesPerTag : 4; ///< EDX[11..8] - L3 cache lines per tag
        UINT32 L3Assoc : 4; ///< EDX[15..12] - L3 cache associativity
        UINT32 Reserved : 2; ///< EDX[17..16] - Reserved
        UINT32 L3Size : 14; ///< EDX[31..18] - L3 cache size in 512 KB
    } Bits;
    UINT32 Uint32;
} CPUID_AMD_EXTENDED_CACHE_INFO_EDX;

typedef union
{
    INT32 AsInt32[4];
    struct
    {
        CPUID_AMD_EXTENDED_CACHE_INFO_EAX Eax;
        CPUID_AMD_EXTENDED_CACHE_INFO_EBX Ebx;
        CPUID_AMD_EXTENDED_CACHE_INFO_ECX Ecx;
        CPUID_AMD_EXTENDED_CACHE_INFO_EDX Edx;
    };
} CPUID_AMD_EXTENDED_CACHE_INFO_REGS;

// Additional AMD specific CPUID:
// See
// - AMD64 Architecture Programmer’s Manual Volume 2: System Programming (https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24593.pdf)
// - http://www.flounder.com/cpuid_explorer2.htm#CPUID(0x8000000A)
// - https://www.spinics.net/lists/kvm/msg279165.html
// - https://qemu-devel.nongnu.narkive.com/zgmvxGLq/patch-0-3-svm-feature-support-for-qemu
// - https://github.com/torvalds/linux/blob/28f20a19294da7df158dfca259d0e2b5866baaf9/arch/x86/include/asm/cpufeatures.h#L361

#define CPUID_AMD_SVM_FEATURES 0x8000000A

typedef union
{
    struct
    {
        UINT SVMRev : 8;     // EAX[7..0]
        UINT Reserved : 24;  // EAX[31..8]
    } Bits;

    UINT32    Uint32;
} CPUID_AMD_SVM_FEATURES_EAX;

typedef union
{
    struct
    {
        UINT32 NP : 1; // EDX[0] Nested paging support
        UINT32 LbrVirt : 1; // EDX[1] LBR virtualization
        UINT32 SVML : 1; // EDX[2] SVM Lock
        UINT32 NRIPS : 1; // EDX[3] Next RIP save on VMEXIT
        UINT32 TscRateMsr : 1; // EDX[4] MSR based TSC ratio control
        UINT32 VmcbClean : 1; // EDX[5] VMCB Clean bits support
        UINT32 FlushByAsid : 1; // EDX[6] Flush by ASID support
        UINT32 DecodeAssists : 1; // EDX[7] Decode assists support
        UINT32 Reserved1 : 2; // EDX[9:8]   Reserved
        UINT32 PauseFilter : 1; // EDX[10] Pause filter support
        UINT32 Reserved2 : 1; // EDX[11] Reserved
        UINT32 PauseFilterThreshold : 1; // EDX[12] Pause filter threshold support
        UINT32 AVIC : 1; // EDX[13:13] Advanced Virtual Interrupt Controller
        UINT32 Unknown14 : 1; // EDX[14] Unknown. Described in AMD doc as X2AVIC, but that was probably a typo, since x2AVIC is bit 18.
        UINT32 VMSAVEVirt : 1; // EDX[15] MSAVE and VMLOAD Virtualization
        UINT32 VGIF : 1; // EDX[16] Virtual Global-Interrupt Flag
        UINT32 GMET : 1; // EDX[17] Guest Mode Execute Trap Extension
        UINT32 x2AVIC : 1; // EDX[18] Virtual x2APIC
        UINT32 SSSCheck : 1; // EDX[19] AKA SupervisorShadowStack
        UINT32 V_SPEC_CTRL : 1; // EDX[20] Virtual SPEC_CTRL
        UINT32 ROGPT : 1; // EDX[21]
        UINT32 Unknown22 : 1; // EDX[22]
        UINT32 HOST_MCE_OVERRIDE : 1; // EDX[23]
        UINT32 TLBSYNC : 1; // EDX[24] TLBSYNC instruction can be intercepted
        UINT32 VNMI : 1; // EDX[25] NMI Virtualization support
        UINT32 IbsVirt : 1; // EDX[26] Instruction Based Sampling Virtualization
        UINT32 LVTReadAllowed : 1; // EDX[27]
        UINT32 Unknown28 : 1; // EDX[28]
        UINT32 BusLockThreshold : 1; // EDX[29]
    } Bits;

    UINT32    Uint32;
} CPUID_AMD_SVM_FEATURES_EDX;

// CPUID_AMD_SVM_FEATURES (0x8000000A)
typedef union
{
    INT32 AsInt32[4];
    struct
    {
        CPUID_AMD_SVM_FEATURES_EAX Eax;
        UINT32 NumberOfSupportedASIDs;
        UINT32 Ecx;
        CPUID_AMD_SVM_FEATURES_EDX Edx;
    };
} CPUID_AMD_SVM_FEATURES_REGS;

#define CPUID_CACHE_PARAMS_AMD 0x8000001D
