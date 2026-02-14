/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for extended state
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"
#include <windows.h>
#include <versionhelpers.h>
#include <x86x64/Cpuid.h>

// These are not officially documented
#define XSTATE_PKRU                         9
#define XSTATE_HDC                          13
#define XSTATE_UINTR                        14
#define XSTATE_LBR                          15
#define XSTATE_MASK_PKRU                    (1LL << (XSTATE_PKRU))
#define XSTATE_MASK_HDC                     (1LL << (XSTATE_HDC))
#define XSTATE_MASK_UINTR                   (1LL << (XSTATE_UINTR))
#define XSTATE_MASK_LBR                     (1LL << (XSTATE_LBR))

#define XSTATE_MASK_SUPERVISOR \
    (XSTATE_MASK_IPT | \
     XSTATE_MASK_PASID | \
     XSTATE_MASK_CET_U | \
     XSTATE_MASK_CET_S | \
     XSTATE_MASK_HDC | \
     XSTATE_MASK_UINTR | \
     XSTATE_MASK_LBR)

template<ULONG NtDdiVersion>
struct TXSTATE_CONFIGURATION;

template<>
struct TXSTATE_CONFIGURATION<NTDDI_WIN7>
{
    ULONGLONG EnabledFeatures;                                              //0x0
    ULONG Size;                                                             //0x8
    ULONG OptimizedSave:1;                                                  //0xc
    struct _XSTATE_FEATURE Features[64];                                    //0x10
};

template<>
struct TXSTATE_CONFIGURATION<NTDDI_WIN8>
{
    ULONGLONG EnabledFeatures;                                              //0x0
    ULONGLONG EnabledVolatileFeatures;                                      //0x8
    ULONG Size;                                                             //0x10
    union
    {
        ULONG ControlFlags;                                                 //0x14
        struct
        {
            ULONG OptimizedSave:1;                                          //0x14
            ULONG CompactionEnabled:1;                                      //0x14
            ULONG ExtendedFeatureDisable:1;                                 //0x14
        };
    };
    struct _XSTATE_FEATURE Features[64];                                    //0x18
};

template<>
struct TXSTATE_CONFIGURATION<NTDDI_WIN10> : TXSTATE_CONFIGURATION<NTDDI_WIN8>
{
    ULONGLONG EnabledSupervisorFeatures;                                    //0x218
    ULONGLONG AlignedFeatures;                                              //0x220
    ULONG AllFeatureSize;                                                   //0x228
    ULONG AllFeatures[64];                                                  //0x22c
};

template<>
struct TXSTATE_CONFIGURATION<NTDDI_WIN10_RS5> : TXSTATE_CONFIGURATION<NTDDI_WIN10>
{
    ULONGLONG EnabledUserVisibleSupervisorFeatures;                         //0x330
};

template<>
struct TXSTATE_CONFIGURATION<NTDDI_WIN11> : TXSTATE_CONFIGURATION<NTDDI_WIN10_RS5>
{
    ULONGLONG ExtendedFeatureDisableFeatures;                               //0x338
    ULONG AllNonLargeFeatureSize;                                           //0x340
    ULONG Spare;                                                            //0x344
};

ULONG GetXStateNtDdiVersion(void)
{
    if (IsReactOS())
    {
        return NTDDI_WIN11;
    }

    /* Get the NTDDI_VERSION from the PEB fields */
    PPEB Peb = NtCurrentPeb();
    ULONG WinVersion = (Peb->OSMajorVersion << 8) | Peb->OSMinorVersion;
    switch (WinVersion)
    {
        case _WIN32_WINNT_WIN7:
            return NTDDI_WIN7;

        case _WIN32_WINNT_WIN8:
            return NTDDI_WIN8;

        case _WIN32_WINNT_WINBLUE:
            return NTDDI_WIN8; // same as Win8

        case _WIN32_WINNT_WIN10:
        {
            switch (Peb->OSBuildNumber)
            {
                case 10240: // 10.0.10240 / 1507 / Threshold 1
                case 10586: // 10.0.10586 / 1511 / Threshold 2
                case 14393: // 10.0.14393 / 1607 / Redstone 1
                case 15063: // 10.0.15063 / 1703 / Redstone 2
                case 16299: // 10.0.16299 / 1709 / Redstone 3
                case 17134: // 10.0.17134 / 1803 / Redstone 4
                    return NTDDI_WIN10;
                case 17763: // 10.0.17763 / 1809 / Redstone 5
                case 18362: // 10.0.18362 / 1903 / 19H1 "Titanium"
                case 18363: // 10.0.18363 / Vanadium
                case 19041: // 10.0.19041 / 2004 / Vibranium R1
                case 19042: // 10.0.19042 / 20H2 / Vibranium R2 aka Manganese
                case 19043: // 10.0.19043 / 21H1 / Vibranium R3 aka Ferrum
                case 19044: // 10.0.19044 / 21H2 / Vibranium R4 aka Cobalt
                case 19045: // 10.0.19045 / 22H2 / Vibranium R5
                    return NTDDI_WIN10_RS5;

                // Win 11
                case 22000: // Cobalt
                case 22621: // 22H2 Nickel R1
                case 22631: // 23H2 Nickel R2
                case 26100: // 24H2 Germanium
                    return NTDDI_WIN11;

                default:
                    trace("Unknown Windows 10 build number: %d\n", Peb->OSBuildNumber);
                    return 0;
            }
        }

        default:
            trace("UnsuUnknown Windows version: 0x%lX\n", WinVersion);
            return 0;
    }

    return 0;
}

template<ULONG NtDdiVersion>
SIZE_T GetXStateOffset(void)
{
    if (IsReactOS())
    {
        return FIELD_OFFSET(KUSER_SHARED_DATA, XState); // ReactOS
    }
    if (NtDdiVersion < NTDDI_WIN8)
    {
        return 0x3e0; // Win 7
    }
    else
    {
        return 0x3d8; // Win 8 - Win 11
    }
}

template<ULONG NtDdiVersion>
TXSTATE_CONFIGURATION<NtDdiVersion>* GetOsXState(void)
{
    SIZE_T Offset = GetXStateOffset<NtDdiVersion>();
    PVOID Pointer = (PVOID)((ULONG_PTR)SharedUserData + Offset);
    return (TXSTATE_CONFIGURATION<NtDdiVersion>*)Pointer;
}

void GetExpectedXStateConfig(TXSTATE_CONFIGURATION<NTDDI_WIN11>* XStateConfig)
{
    ULONG64 SupportedUserMask;
    ULONG64 SupportedSupervisorMask;
    ULONG64 SupportedComponentMask;
    ULONG NextUserOffset, NextSupervisorOffset, NextOffset;

    RtlZeroMemory(XStateConfig, sizeof(*XStateConfig));

    if (!IsProcessorFeaturePresent(PF_XSAVE_ENABLED))
    {
        trace("XSAVE not supported\n");
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

    /* Mask the allowed components */
    SupportedUserMask &= XSTATE_MASK_ALLOWED;
    XStateConfig->EnabledFeatures = SupportedUserMask;
    XStateConfig->EnabledVolatileFeatures = SupportedUserMask & ~XSTATE_MASK_PERSISTENT;

    /* Read CPUID_EXTENDED_STATE sub-leaf (0x0D, 0x01) */
    CPUID_EXTENDED_STATE_SUB_LEAF_REGS ExtStateSub;
    __cpuidex(ExtStateSub.AsInt32,
              CPUID_EXTENDED_STATE,
              CPUID_EXTENDED_STATE_SUB_LEAF);

    /* Save control flags */
    XStateConfig->OptimizedSave = ExtStateSub.Eax.Bits.XSAVEOPT;
    XStateConfig->CompactionEnabled = ExtStateSub.Eax.Bits.XSAVEC;
    XStateConfig->ExtendedFeatureDisable = ExtStateSub.Eax.Bits.Xfd;

    /* Determine supported supervisor features */
    SupportedSupervisorMask = 0;
    if (ExtStateSub.Eax.Bits.XSAVES)
    {
        SupportedSupervisorMask = (ULONG64)ExtStateSub.Edx << 32 |
                                  (ULONG64)ExtStateSub.Ecx.Uint32;
        SupportedSupervisorMask &= XSTATE_MASK_ALLOWED & XSTATE_MASK_SUPERVISOR;
    }

    /* Save the supervisor features */
    XStateConfig->EnabledSupervisorFeatures = SupportedSupervisorMask;
    XStateConfig->EnabledUserVisibleSupervisorFeatures = SupportedSupervisorMask & XSTATE_MASK_USER_VISIBLE_SUPERVISOR;

    /* Calculate full mask */
    SupportedComponentMask = SupportedUserMask | SupportedSupervisorMask;

    /* Basic features (always enabled) */
    XStateConfig->Features[XSTATE_LEGACY_FLOATING_POINT].Offset = 0;
    XStateConfig->Features[XSTATE_LEGACY_FLOATING_POINT].Size = FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters);
    XStateConfig->AllFeatures[XSTATE_LEGACY_FLOATING_POINT] = FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters);
    XStateConfig->Features[XSTATE_LEGACY_SSE].Offset = FIELD_OFFSET(XSAVE_FORMAT, XmmRegisters);
    XStateConfig->Features[XSTATE_LEGACY_SSE].Size = RTL_FIELD_SIZE(XSAVE_FORMAT, XmmRegisters);
    XStateConfig->AllFeatures[XSTATE_LEGACY_SSE] = RTL_FIELD_SIZE(XSAVE_FORMAT, XmmRegisters);

    /* Other components start after legacy state + header */
    NextUserOffset = NextSupervisorOffset = sizeof(XSAVE_AREA);

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

        /* If the offset is 0, this component isn't valid */
        if (ExtStateComponent.Size == 0) continue;

        /* Check for components that are not OS supported */
        if ((ComponentBit & SupportedComponentMask) == 0)
        {
            /* This emulates weird (broken) Windows behavior */
            if ((ComponentBit & XSTATE_MASK_SUPERVISOR) == 0)
            {
                XStateConfig->Features[Component].Offset = ExtStateComponent.Offset;
                XStateConfig->Features[Component].Size = ExtStateComponent.Size;
            }

            /* Skip the rest */
            continue;
        }

        /* Check if compaction is enabled */
        if (XStateConfig->CompactionEnabled)
        {
            /* Align the offsets, if needed */
            if (ExtStateComponent.Ecx.Bits.Aligned)
            {
                XStateConfig->AlignedFeatures |= ComponentBit;
                NextSupervisorOffset = ALIGN_UP(NextSupervisorOffset, 64);
                if ((ComponentBit & SupportedUserMask) != 0)
                {
                    NextUserOffset = ALIGN_UP(NextUserOffset, 64);
                }
            }

            /* Update the supervisor offset */
            NextSupervisorOffset += ExtStateComponent.Size;

            /* For user components save and update the offset and size */
            if ((ComponentBit & SupportedUserMask) != 0)
            {
                XStateConfig->Features[Component].Offset = NextUserOffset;
                XStateConfig->Features[Component].Size = ExtStateComponent.Size;
                NextUserOffset += ExtStateComponent.Size;
            }
        }
        else
        {
            /* Not compacted, use the offset and size specified by the CPUID */
            NextOffset = ExtStateComponent.Offset + ExtStateComponent.Size;
            NextSupervisorOffset = max(NextSupervisorOffset, NextOffset);

            /* For user components save and update the offset and size */
            if ((ComponentBit & SupportedUserMask) != 0)
            {
                XStateConfig->Features[Component].Offset = ExtStateComponent.Offset;
                XStateConfig->Features[Component].Size = ExtStateComponent.Size;
                NextUserOffset = max(NextUserOffset, NextOffset);
            }
        }
    }

    XStateConfig->Size = NextUserOffset;
    XStateConfig->AllFeatureSize = NextSupervisorOffset;
}

template<ULONG NtDdiVersion>
void ValidateXState(
    TXSTATE_CONFIGURATION<NtDdiVersion>* XStateConfig,
    TXSTATE_CONFIGURATION<NTDDI_WIN11>* ExpectedConfig);

template<>
void ValidateXState<NTDDI_WIN7>(
    TXSTATE_CONFIGURATION<NTDDI_WIN7>* XStateConfig,
    TXSTATE_CONFIGURATION<NTDDI_WIN11>* ExpectedConfig)
{
    ok_eq_hex64(XStateConfig->EnabledFeatures, ExpectedConfig->EnabledFeatures);
    ok_eq_ulong(XStateConfig->Size, ExpectedConfig->Size);
    ok_eq_ulong(XStateConfig->OptimizedSave, ExpectedConfig->OptimizedSave);
    for (ULONG i = 0; i < 64; i++)
    {
        ok(XStateConfig->Features[i].Offset == ExpectedConfig->Features[i].Offset,
           "XStateConfig->Features[%lu].Offset = 0x%lx, expected 0x%lx\n",
           i, XStateConfig->Features[i].Offset, ExpectedConfig->Features[i].Offset);
        ok(XStateConfig->Features[i].Offset == ExpectedConfig->Features[i].Offset,
           "XStateConfig->Features[%lu].Size = 0x%lx, expected 0x%lx\n",
           i, XStateConfig->Features[i].Size, ExpectedConfig->Features[i].Size);
    }
};

template<>
void ValidateXState<NTDDI_WIN8>(
    TXSTATE_CONFIGURATION<NTDDI_WIN8>* XStateConfig,
    TXSTATE_CONFIGURATION<NTDDI_WIN11>* ExpectedConfig)
{
    ok_eq_hex64(XStateConfig->EnabledFeatures, ExpectedConfig->EnabledFeatures);
    ok_eq_hex64(XStateConfig->EnabledVolatileFeatures, ExpectedConfig->EnabledVolatileFeatures);
    ok_eq_ulong(XStateConfig->Size, ExpectedConfig->Size);
    ok_eq_ulong(XStateConfig->OptimizedSave, ExpectedConfig->OptimizedSave);
    for (ULONG i = 0; i < 64; i++)
    {
        ok(XStateConfig->Features[i].Offset == ExpectedConfig->Features[i].Offset,
           "XStateConfig->Features[%lu].Offset = 0x%lx, expected 0x%lx\n",
           i, XStateConfig->Features[i].Offset, ExpectedConfig->Features[i].Offset);
        ok(XStateConfig->Features[i].Size == ExpectedConfig->Features[i].Size,
           "XStateConfig->Features[%lu].Size = 0x%lx, expected 0x%lx\n",
           i, XStateConfig->Features[i].Size, ExpectedConfig->Features[i].Size);
    }
}

template<>
void ValidateXState<NTDDI_WIN10>(
    TXSTATE_CONFIGURATION<NTDDI_WIN10>* XStateConfig,
    TXSTATE_CONFIGURATION<NTDDI_WIN11>* ExpectedConfig)
{
    ValidateXState<NTDDI_WIN8>(XStateConfig, ExpectedConfig);
    ok_eq_hex64(XStateConfig->EnabledSupervisorFeatures, ExpectedConfig->EnabledSupervisorFeatures);
    ok_eq_hex64(XStateConfig->AlignedFeatures, ExpectedConfig->AlignedFeatures);
    ok_eq_ulong(XStateConfig->AllFeatureSize, ExpectedConfig->AllFeatureSize);
    for (ULONG i = 0; i < 64; i++)
    {
        ok(XStateConfig->AllFeatures[i] == ExpectedConfig->AllFeatures[i],
           "XStateConfig->AllFeatures[%lu] = 0x%lx, expected 0x%lx\n",
           i, XStateConfig->AllFeatures[i], ExpectedConfig->AllFeatures[i]);
    }
}

template<>
void ValidateXState<NTDDI_WIN10_RS5>(
    TXSTATE_CONFIGURATION<NTDDI_WIN10_RS5>* XStateConfig,
    TXSTATE_CONFIGURATION<NTDDI_WIN11>* ExpectedConfig)
{
    ValidateXState<NTDDI_WIN10>(XStateConfig, ExpectedConfig);
    ok_eq_hex64(XStateConfig->EnabledUserVisibleSupervisorFeatures, ExpectedConfig->EnabledUserVisibleSupervisorFeatures);
}

template<>
void ValidateXState<NTDDI_WIN11>(
    TXSTATE_CONFIGURATION<NTDDI_WIN11>* XStateConfig,
    TXSTATE_CONFIGURATION<NTDDI_WIN11>* ExpectedConfig)
{
    ValidateXState<NTDDI_WIN10_RS5>(XStateConfig, ExpectedConfig);
    ok_eq_hex64(XStateConfig->ExtendedFeatureDisableFeatures, ExpectedConfig->ExtendedFeatureDisableFeatures);
    ok_eq_ulong(XStateConfig->AllNonLargeFeatureSize, ExpectedConfig->AllNonLargeFeatureSize);
    ok_eq_ulong(XStateConfig->Spare, ExpectedConfig->Spare);
}

template<ULONG NtDdiVersion>
void TestXStateConfig(void)
{
    TXSTATE_CONFIGURATION<NTDDI_WIN11> ExpectedXState;
    TXSTATE_CONFIGURATION<NtDdiVersion>* ActualXState = GetOsXState<NtDdiVersion>();

    GetExpectedXStateConfig(&ExpectedXState);

    ValidateXState<NtDdiVersion>(ActualXState, &ExpectedXState);

    if (IsProcessorFeaturePresent(PF_XSAVE_ENABLED))
    {
        ULONG64 xcr0 = _xgetbv(0);
        ok_eq_hex64(ActualXState->EnabledFeatures, xcr0);
    }
}

START_TEST(XStateConfig)
{
    ULONG NtDdiVersion = GetXStateNtDdiVersion();

    switch (NtDdiVersion)
    {
        case NTDDI_WIN7:
            TestXStateConfig<NTDDI_WIN7>();
            break;
        case NTDDI_WIN8:
            TestXStateConfig<NTDDI_WIN8>();
            break;
        case NTDDI_WIN10:
            TestXStateConfig<NTDDI_WIN10>();
            break;
        case NTDDI_WIN10_RS5:
            TestXStateConfig<NTDDI_WIN10_RS5>();
            break;
        case NTDDI_WIN11:
            TestXStateConfig<NTDDI_WIN11>();
            break;

        default:
            skip("Skipping XStateConfig test on usupported Windows version\n");
            break;
    }
}
