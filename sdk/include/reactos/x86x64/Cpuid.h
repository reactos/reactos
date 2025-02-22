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
