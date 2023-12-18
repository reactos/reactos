/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for Extended Context functions
 * COPYRIGHT:   Copyright 2022 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <rtltests.h>
#include <x86x64/Cpuid.h>

PXSTATE_CONFIGURATION gXStateConfig = NULL;

typedef ULONG64 NTAPI FN_RtlGetEnabledExtendedFeatures(ULONG64 FeatureMask);
FN_RtlGetEnabledExtendedFeatures* pfnRtlGetEnabledExtendedFeatures;

typedef NTSTATUS WINAPI FN_RtlGetExtendedContextLength2(ULONG ContextFlags, PULONG Length, ULONG64 XStateCompactionMask);
FN_RtlGetExtendedContextLength2* pfnRtlGetExtendedContextLength2;

void Test_RtlGetEnabledExtendedFeatures(void)
{
    if (!pfnRtlGetEnabledExtendedFeatures)
    {
        skip("RtlGetEnabledExtendedFeatures not found\n");
        return;
    }

    for (ULONG64 Mask = 1; Mask != 0; Mask <<= 1)
    {
        ULONG64 Features = pfnRtlGetEnabledExtendedFeatures(Mask);
        ULONG64 Expected = gXStateConfig->EnabledFeatures & Mask;
        ok_eq_hex64(Features, Expected);
    }

}

#define CONTEXT_XSTATE          (CONTEXT_AMD64 | 0x00000040L)
#define CONTEXT_KERNEL_CET      (CONTEXT_AMD64 | 0x00000080L)

#ifndef CONTEXT_i386
#define CONTEXT_i386    0x10000
#endif
#ifndef CONTEXT_AMD64
#define CONTEXT_AMD64   0x00100000
#endif
#ifndef CONTEXT_ARM32
#define CONTEXT_ARM32   0x00200000
#endif
#ifndef CONTEXT_ARM64
#define CONTEXT_ARM64   0x00400000
#endif

#define XSTATE_LEGACY_FLOATING_POINT        0
#define XSTATE_LEGACY_SSE                   1
#define XSTATE_GSSE                         2
#define XSTATE_AVX                          XSTATE_GSSE
#define XSTATE_MPX_BNDREGS                  3
#define XSTATE_MPX_BNDCSR                   4
#define XSTATE_AVX512_KMASK                 5
#define XSTATE_AVX512_ZMM_H                 6
#define XSTATE_AVX512_ZMM                   7
#define XSTATE_IPT                          8
#define XSTATE_PASID                        10
#define XSTATE_CET_U                        11
#define XSTATE_CET_S                        12
#define XSTATE_AMX_TILE_CONFIG              17
#define XSTATE_AMX_TILE_DATA                18
#define XSTATE_LWP                          62
#define MAXIMUM_XSTATE_FEATURES             64

#define XSTATE_MASK_LEGACY_FLOATING_POINT   (1LL << (XSTATE_LEGACY_FLOATING_POINT))
#define XSTATE_MASK_LEGACY_SSE              (1LL << (XSTATE_LEGACY_SSE))
#define XSTATE_MASK_LEGACY                  (XSTATE_MASK_LEGACY_FLOATING_POINT | XSTATE_MASK_LEGACY_SSE)
#define XSTATE_MASK_GSSE                    (1LL << (XSTATE_GSSE))
#define XSTATE_MASK_AVX                     XSTATE_MASK_GSSE
#define XSTATE_MASK_MPX                     ((1LL << (XSTATE_MPX_BNDREGS)) | (1LL << (XSTATE_MPX_BNDCSR)))
#define XSTATE_MASK_AVX512                  ((1LL << (XSTATE_AVX512_KMASK)) | (1LL << (XSTATE_AVX512_ZMM_H)) |  (1LL << (XSTATE_AVX512_ZMM)))
#define XSTATE_MASK_AVX512                  ((1LL << (XSTATE_AVX512_KMASK)) | (1LL << (XSTATE_AVX512_ZMM_H)) |  (1LL << (XSTATE_AVX512_ZMM)))
#define XSTATE_MASK_IPT                     (1LL << (XSTATE_IPT))
#define XSTATE_MASK_PASID                   (1LL << (XSTATE_PASID))
#define XSTATE_MASK_CET_U                   (1LL << (XSTATE_CET_U))
#define XSTATE_MASK_CET_S                   (1LL << (XSTATE_CET_S))
#define XSTATE_MASK_AMX_TILE_CONFIG         (1LL << (XSTATE_AMX_TILE_CONFIG))
#define XSTATE_MASK_AMX_TILE_DATA           (1LL << (XSTATE_AMX_TILE_DATA))
#define XSTATE_MASK_LWP                     (1LL << (XSTATE_LWP))

typedef struct DECLSPEC_ALIGN(16) _XSAVE_FORMAT {
  USHORT ControlWord;
  USHORT StatusWord;
  UCHAR TagWord;
  UCHAR Reserved1;
  USHORT ErrorOpcode;
  ULONG ErrorOffset;
  USHORT ErrorSelector;
  USHORT Reserved2;
  ULONG DataOffset;
  USHORT DataSelector;
  USHORT Reserved3;
  ULONG MxCsr;
  ULONG MxCsr_Mask;
  M128A FloatRegisters[8];
#if defined(_WIN64)
  M128A XmmRegisters[16];
  UCHAR Reserved4[96];
#else
  M128A XmmRegisters[8];
  UCHAR Reserved4[192];
  ULONG StackControl[7];
  ULONG Cr0NpxState;
#endif
} XSAVE_FORMAT, *PXSAVE_FORMAT;

typedef struct DECLSPEC_ALIGN(8) _XSAVE_AREA_HEADER {
  ULONG64 Mask;
  ULONG64 Reserved[7];
} XSAVE_AREA_HEADER, *PXSAVE_AREA_HEADER;

typedef struct DECLSPEC_ALIGN(16) _XSAVE_AREA {
  XSAVE_FORMAT LegacyState;
  XSAVE_AREA_HEADER Header;
} XSAVE_AREA, *PXSAVE_AREA;

ULONG
GetExpectedLength(ULONG ContextFlags, ULONG64 CompactionMask)
{
    ULONG Length = 0;
    //ULONG Alignment;

    if (ContextFlags & CONTEXT_i386)
    {
        // sizeof(CONTEXT) + sizeof(CONTEXT_EX) + 3 for alignment
        Length = 0x2cc + 0x18 + 3;
    }
    else if (ContextFlags & CONTEXT_AMD64)
    {
        // sizeof(CONTEXT) + sizeof(CONTEXT_EX) + 7 for alignment
        Length = 0x4D0 + 0x20 + 7;
    }
    else if (ContextFlags & CONTEXT_ARM32)
    {
        return 0x1bf;
    }
    else if (ContextFlags & CONTEXT_ARM64)
    {
        return 0x3b7;
    }

    if ((ContextFlags & 0x40) == 0)
    {
        return Length;
    }

    Length += sizeof(XSAVE_AREA) + 0x20;

    CompactionMask &= pfnRtlGetEnabledExtendedFeatures(0xffffffffffffffffULL);

    if (CompactionMask & XSTATE_MASK_AVX)
    {
        Length += 0x40;
    }

    return Length;
}

void Test_RtlGetExtendedContextLength2(void)
{
    if (!pfnRtlGetExtendedContextLength2)
    {
        skip("RtlGetExtendedContextLength2 not found\n");
        return;
    }

    ULONG Length;

    /* Valid call */
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ALL, &Length, 0), STATUS_SUCCESS);

    /* This one is an access violation */
    StartSeh()
        pfnRtlGetExtendedContextLength2(CONTEXT_ALL, NULL, 0);
    EndSeh(STATUS_ACCESS_VIOLATION);

    /* Check different architectures in ContextFlags */
    for (ULONG ContextFlags = 1; ContextFlags != 0; ContextFlags <<= 1)
    {
        NTSTATUS ExpectedStatus = STATUS_INVALID_PARAMETER;
        if ((ContextFlags == CONTEXT_i386) ||
            (ContextFlags == CONTEXT_AMD64) ||
            (ContextFlags == CONTEXT_ARM32) ||
            (ContextFlags == CONTEXT_ARM64))
        {
            ExpectedStatus = STATUS_SUCCESS;
        }

        NTSTATUS Status = pfnRtlGetExtendedContextLength2(ContextFlags, &Length, 0);
        ok(Status == ExpectedStatus,
           "Unexpected Status for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedStatus, Status);
    }

    /* Invalid architecture combinations */
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_i386 | CONTEXT_AMD64, &Length, 0), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_i386 | CONTEXT_ARM32, &Length, 0), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_i386 | CONTEXT_ARM64, &Length, 0), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_AMD64 | CONTEXT_ARM32, &Length, 0), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_AMD64 | CONTEXT_ARM64, &Length, 0), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM32 | CONTEXT_ARM64, &Length, 0), STATUS_INVALID_PARAMETER);

    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_i386, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x2e7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_i386 | 0x1, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x2e7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_i386 | 0x2, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x2e7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_i386 | 0x4, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x2e7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_i386 | 0x8, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x2e7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_i386 | 0x10, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x2e7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_i386 | 0x20, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x2e7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_i386 | 0x40, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x363); // +0x7c
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_i386 | 0x80, &Length, 0), STATUS_INVALID_PARAMETER);

    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_AMD64, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x4f7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_AMD64 | 0x1, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x4f7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_AMD64 | 0x2, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x4f7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_AMD64 | 0x4, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x4f7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_AMD64 | 0x8, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x4f7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_AMD64 | 0x10, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x4f7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_AMD64 | 0x20, &Length, 0), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_AMD64 | 0x40, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x56f); // +0x78
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_AMD64 | 0x80, &Length, 0), STATUS_INVALID_PARAMETER);

    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM32, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x1bf);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM32 | 0x1, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x1bf);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM32 | 0x2, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x1bf);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM32 | 0x4, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x1bf);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM32 | 0x8, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x1bf);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM32 | 0x10, &Length, 0), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM32 | 0x20, &Length, 0), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM32 | 0x40, &Length, 0), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM32 | 0x80, &Length, 0), STATUS_INVALID_PARAMETER);
    ok_eq_hex(Length, 0x1bf);

    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM64, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x3b7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM64 | 0x1, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x3b7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM64 | 0x2, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x3b7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM64 | 0x4, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x3b7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM64 | 0x8, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x3b7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM64 | 0x10, &Length, 0), STATUS_SUCCESS), ok_eq_hex(Length, 0x3b7);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM64 | 0x20, &Length, 0), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM64 | 0x40, &Length, 0), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_ARM64 | 0x80, &Length, 0), STATUS_INVALID_PARAMETER);
    ok_eq_hex(Length, 0x3b7);

    /* Check status for different compaction masks */
    for (ULONG64 CompactionMask = 1; CompactionMask != 2; CompactionMask <<= 1)
    {
        /* i386 */
        ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_i386, &Length, CompactionMask), STATUS_SUCCESS);
        ok_eq_hex(Length, 0x2e7);
        ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_i386 | 0x40, &Length, CompactionMask), STATUS_SUCCESS);
        ok_eq_hex(Length, GetExpectedLength(CONTEXT_i386 | 0x40, CompactionMask));

        /* AMD64 */
        ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_AMD64, &Length, CompactionMask), STATUS_SUCCESS);
        ok_eq_hex(Length, 0x4f7);
        ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_AMD64 | 0x40, &Length, CompactionMask), STATUS_SUCCESS);
    }

    ok_eq_hex(pfnRtlGetExtendedContextLength2(CONTEXT_i386 | 0x40, &Length, 0x01), STATUS_SUCCESS);
    ok_eq_hex(Length, 0x363);

}

#define KF_XSTATE 1
#define KiGetFeatureBits64() KF_XSTATE
#define XSTATE_MASK_ALLOWED \
    (XSTATE_MASK_LEGACY | \
     XSTATE_MASK_AVX | \
     XSTATE_MASK_MPX | \
     XSTATE_MASK_AVX512 | \
     XSTATE_MASK_IPT | \
     XSTATE_MASK_PASID | \
     XSTATE_MASK_CET_U | \
     XSTATE_MASK_AMX_TILE_CONFIG | \
     XSTATE_MASK_AMX_TILE_DATA | \
     XSTATE_MASK_LWP)
#define XSTATE_MASK_SUPERVISOR \
    (XSTATE_MASK_IPT | \
     XSTATE_MASK_PASID | \
     XSTATE_MASK_CET_U | \
     XSTATE_MASK_CET_S)

#ifndef FIELD_SIZE
#define FIELD_SIZE(type, field) (sizeof(((type *)0)->field))
#endif

static
VOID
KiGetXStateConfiguration(
    PXSTATE_CONFIGURATION XStateConfig)
{
    ULONG FeatureBits = KiGetFeatureBits64();
    ULONG64 SupportedUserMask;
    ULONG64 SupportedSupervisorMask;
    ULONG64 SupportedComponentMask;
    //ULONG64 EnabledMask = XSTATE_MASK_LEGACY;
    ULONG CurrentOffset;
    ULONG NextOffset, Size, AllFeatureSize;

    RtlZeroMemory(XStateConfig, sizeof(*XStateConfig));

    /* Check if XSAVE is supported */
    if ((FeatureBits & KF_XSTATE) == 0)
    {
        /* XSAVE is not supported */
        return;
    }

    /* Read CPUID_EXTENDED_STATE main leaf (0x0D, 0x00) */
    CPUID_EXTENDED_STATE_MAIN_LEAF_REGS ExtStateMain;
    __cpuidex(ExtStateMain.AsInt32,
              CPUID_EXTENDED_STATE,
              CPUID_EXTENDED_STATE_MAIN_LEAF);

    /* Get the supported XCR0 bits */
    SupportedUserMask = (ULONG64)ExtStateMain.Edx << 32 |
                        (ULONG64)ExtStateMain.Eax.Uint32;

    /* Make sure legacy components are supported */
    if ((SupportedUserMask & XSTATE_MASK_LEGACY) != XSTATE_MASK_LEGACY)
    {
        return;
    }

    /* Read CPUID_EXTENDED_STATE sub-leaf (0x0D, 0x01) */
    CPUID_EXTENDED_STATE_SUB_LEAF_REGS ExtStateSub;
    __cpuidex(ExtStateSub.AsInt32,
              CPUID_EXTENDED_STATE,
              CPUID_EXTENDED_STATE_SUB_LEAF);

    /* Determine supported supervisor features */
    SupportedSupervisorMask = 0;
    if (ExtStateSub.Eax.Bits.XSAVES)
    {
        SupportedSupervisorMask = (ULONG64)ExtStateSub.Edx << 32 |
                                  (ULONG64)ExtStateSub.Ecx.Uint32;
    }

    /* Calculate full mask */
    SupportedComponentMask = SupportedUserMask | SupportedSupervisorMask;

    /* Save the features to be enabled */
    XStateConfig->EnabledFeatures = SupportedUserMask & XSTATE_MASK_ALLOWED;
    XStateConfig->EnabledSupervisorFeatures = SupportedSupervisorMask & XSTATE_MASK_ALLOWED;

    /* Save control flags */
    XStateConfig->OptimizedSave = ExtStateSub.Eax.Bits.XSAVEOPT;
    XStateConfig->CompactionEnabled = ExtStateSub.Eax.Bits.XSAVEC;

    /* Initialize legacy features (always enabled) */
    XStateConfig->Features[XSTATE_LEGACY_FLOATING_POINT].Offset = 0;
    XStateConfig->Features[XSTATE_LEGACY_FLOATING_POINT].Size = FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters);
    XStateConfig->AllFeatures[XSTATE_LEGACY_FLOATING_POINT] = FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters);
    XStateConfig->Features[XSTATE_LEGACY_SSE].Offset = FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters);
    XStateConfig->Features[XSTATE_LEGACY_SSE].Size = FIELD_SIZE(XSAVE_FORMAT, XmmRegisters);
    XStateConfig->AllFeatures[XSTATE_LEGACY_SSE] = FIELD_SIZE(XSAVE_FORMAT, XmmRegisters);

    /* Other components start after legacy state + header */
    AllFeatureSize = Size = NextOffset = sizeof(XSAVE_AREA);

    /* Loop all components from 2 up */
    for (ULONG Component = 2; Component < MAXIMUM_XSTATE_FEATURES; Component++)
    {
        ULONG64 ComponentBit = (1ULL << Component);

        /* Query component features */
        CPUID_EXTENDED_STATE_SIZE_OFFSET_REGS ExtStateComponent;
        __cpuidex(ExtStateComponent.AsInt32,
                  CPUID_EXTENDED_STATE,
                  Component);

        /* Save size for all features */
        XStateConfig->AllFeatures[Component] = ExtStateComponent.Size;

        /* Skip components that are not supported */
        if ((ComponentBit & SupportedComponentMask) == 0) continue;

        /* If the offset is 0, this component isn't valid */
        if (ExtStateComponent.Size == 0) __debugbreak();

        /* For all allowed features update AllFeatureSize */
        if (ComponentBit & XSTATE_MASK_ALLOWED)
        {
            /* AllFeatureSize includes supervisor components, so we always use
               the compacted format, because that is implicit in XSAVES. */
            if (ExtStateComponent.Ecx.Bits.Aligned)
            {
                AllFeatureSize = ALIGN_UP(AllFeatureSize, 64);
                XStateConfig->AlignedFeatures |= ComponentBit;
            }
            AllFeatureSize += ExtStateComponent.Size;
        }

        /* Check if compaction is enabled */
        if (XStateConfig->CompactionEnabled)
        {
            /* Align the offset, if needed */
            if (ExtStateComponent.Ecx.Bits.Aligned)
            {
                NextOffset = ALIGN_UP(NextOffset, 64);

                /* Update the size only for enabled user features */
                if (ComponentBit & XStateConfig->EnabledFeatures)
                {
                    Size = ALIGN_UP(Size, 64);
                }
            }

            /* Save the offset/size only for supported user features */
            if (ComponentBit & SupportedUserMask)
            {
                XStateConfig->Features[Component].Offset = NextOffset;
                XStateConfig->Features[Component].Size = ExtStateComponent.Size;
            }

            /* Update the offset/size only for enabled user features */
            if (ComponentBit & XStateConfig->EnabledFeatures)
            {
                Size += ExtStateComponent.Size;
            }

            NextOffset += ExtStateComponent.Size;
        }
        else
        {
            /* Save the offset/size only for supported user features */
            if (ComponentBit & SupportedUserMask)
            {
                /* Not compacted, we use the offset specified by the CPUID */
                XStateConfig->Features[Component].Offset = ExtStateComponent.Offset;
                XStateConfig->Features[Component].Size = ExtStateComponent.Size;
            }

            /* Update the size only for enabled user features */
            if (ComponentBit & XStateConfig->EnabledFeatures)
            {
                /* Update the offset, using the highest value */
                CurrentOffset = XStateConfig->Features[Component].Offset +
                                XStateConfig->Features[Component].Size;
                Size = max(NextOffset, CurrentOffset);
            }
        }
    }

     /* Save the sizes */
    XStateConfig->Size = Size;
    XStateConfig->AllFeatureSize = AllFeatureSize;


    /* Query CPUID again to get the full size based on enabled components */
    __cpuidex(ExtStateMain.AsInt32,
              CPUID_EXTENDED_STATE,
              CPUID_EXTENDED_STATE_MAIN_LEAF);

    /* Get the full size */
    //XStateConfig->AllFeatureSize = ExtStateMain.Ecx;
}

static
VOID
ValidateXStateConfig(
    PXSTATE_CONFIGURATION XState)
{
    //PXSTATE_CONFIGURATION GlobalXState = &SharedUserData->XState;

    ok_eq_hex(XState->EnabledFeatures, gXStateConfig->EnabledFeatures);
    ok_eq_hex(XState->EnabledSupervisorFeatures, gXStateConfig->EnabledSupervisorFeatures);
    ok_eq_hex(XState->Size, gXStateConfig->Size);
    ok_eq_hex(XState->AllFeatureSize, gXStateConfig->AllFeatureSize);

    for (ULONG i = 0; i < MAXIMUM_XSTATE_FEATURES; i++)
    {
        ok_eq_hex(XState->Features[i].Size, gXStateConfig->Features[i].Size);
        ok_eq_hex(XState->Features[i].Offset, gXStateConfig->Features[i].Offset);
        ok_eq_hex(XState->AllFeatures[i], gXStateConfig->AllFeatures[i]);
    }
}

START_TEST(RtlExContext)
{
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    pfnRtlGetEnabledExtendedFeatures = (FN_RtlGetEnabledExtendedFeatures*)GetProcAddress(hNtdll, "RtlGetEnabledExtendedFeatures");
    pfnRtlGetExtendedContextLength2 = (FN_RtlGetExtendedContextLength2*)GetProcAddress(hNtdll, "RtlGetExtendedContextLength2");

    DWORD dwVersion = MAKELONG(NtCurrentPeb()->OSMinorVersion,
                               NtCurrentPeb()->OSMajorVersion);

    if (dwVersion >= 0x602)
    {
        gXStateConfig = (PXSTATE_CONFIGURATION)((PUCHAR)SharedUserData + 0x3d8);
    }
    else if (dwVersion >= 0x601)
    {
        gXStateConfig = (PXSTATE_CONFIGURATION)((PUCHAR)SharedUserData + 0x3e0);
    }
    else
    {
        // REACTOS!
        gXStateConfig = (PXSTATE_CONFIGURATION)((PUCHAR)SharedUserData + FIELD_OFFSET(KUSER_SHARED_DATA, XState));
    }

    XSTATE_CONFIGURATION XStateConfig;
    KiGetXStateConfiguration(&XStateConfig);
    ValidateXStateConfig(&XStateConfig);

    Test_RtlGetEnabledExtendedFeatures();
    Test_RtlGetExtendedContextLength2();
}
