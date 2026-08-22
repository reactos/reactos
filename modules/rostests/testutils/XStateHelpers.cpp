/*
 * PROJECT:     ReactOS tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Source file for helper functions related to extended state
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <ndk/ntndk.h>
#include <windows.h>
#include <XStateHelpers.h>
#include <versionhelpers.h>
#include <../apitests/include/apitest.h>

EXTERN_C ULONG GetXStateNtDdiVersion(void)
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

EXTERN_C SIZE_T Get_UserShareData_XStateOffset(ULONG NtDdiVersion)
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

EXTERN_C PXSTATE_CONFIGURATION KUSER_SHARED_DATA_XState(PKUSER_SHARED_DATA _SharedUserData)
{
    SIZE_T XstateOffset = Get_UserShareData_XStateOffset(GetXStateNtDdiVersion());
    return (PXSTATE_CONFIGURATION)((ULONG_PTR)_SharedUserData + XstateOffset);
}

EXTERN_C ULONG64 XSTATE_CONFIGURATION_EnabledFeatures(PXSTATE_CONFIGURATION XStateConfig)
{
    return XStateConfig->EnabledFeatures;
}

EXTERN_C ULONG64 XSTATE_CONFIGURATION_EnabledUserVisibleSupervisorFeatures(PXSTATE_CONFIGURATION XStateConfig)
{
    ULONG NtDdiVersion = GetXStateNtDdiVersion();

    if (IsReactOS())
    {
        return XStateConfig->EnabledUserVisibleSupervisorFeatures;
    }
    else if (NtDdiVersion >= NTDDI_WIN10_RS5)
    {
        TXSTATE_CONFIGURATION<NTDDI_WIN10_RS5>* XStateConfigWin10RS5 = (TXSTATE_CONFIGURATION<NTDDI_WIN10_RS5>*)XStateConfig;
        return XStateConfigWin10RS5->EnabledUserVisibleSupervisorFeatures;
    }
    else
    {
        return 0;
    }
}

EXTERN_C ULONG XSTATE_CONFIGURATION_Size(PXSTATE_CONFIGURATION XStateConfig)
{
    ULONG NtDdiVersion = GetXStateNtDdiVersion();

    if (IsReactOS())
    {
        return XStateConfig->Size;
    }
    else if (NtDdiVersion <= NTDDI_WIN7)
    {
        TXSTATE_CONFIGURATION<NTDDI_WIN7>* XStateConfigWin7 = (TXSTATE_CONFIGURATION<NTDDI_WIN7>*)XStateConfig;
        return XStateConfigWin7->Size;
    }
    else
    {
        TXSTATE_CONFIGURATION<NTDDI_WIN8>* XStateConfigWin8 = (TXSTATE_CONFIGURATION<NTDDI_WIN8>*)XStateConfig;
        return XStateConfigWin8->Size;
    }
}

EXTERN_C XSTATE_FEATURE* XSTATE_CONFIGURATION_Features(PXSTATE_CONFIGURATION XStateConfig)
{
    ULONG NtDdiVersion = GetXStateNtDdiVersion();

    if (IsReactOS())
    {
        return XStateConfig->Features;
    }
    else if (NtDdiVersion <= NTDDI_WIN7)
    {
        TXSTATE_CONFIGURATION<NTDDI_WIN7>* XStateConfigWin7 = (TXSTATE_CONFIGURATION<NTDDI_WIN7>*)XStateConfig;
        return XStateConfigWin7->Features;
    }
    else
    {
        TXSTATE_CONFIGURATION<NTDDI_WIN8>* XStateConfigWin8 = (TXSTATE_CONFIGURATION<NTDDI_WIN8>*)XStateConfig;
        return XStateConfigWin8->Features;
    }
}

EXTERN_C PULONG XSTATE_CONFIGURATION_AllFeatures(PXSTATE_CONFIGURATION XStateConfig)
{
    ULONG NtDdiVersion = GetXStateNtDdiVersion();

    if (IsReactOS())
    {
        return XStateConfig->AllFeatures;
    }
    else if (NtDdiVersion >= NTDDI_WIN10)
    {
        TXSTATE_CONFIGURATION<NTDDI_WIN10>* XStateConfigWin10 = (TXSTATE_CONFIGURATION<NTDDI_WIN10>*)XStateConfig;
        return XStateConfigWin10->AllFeatures;
    }
    else
    {
        return NULL;
    }
}

EXTERN_C ULONG64 XSTATE_CONFIGURATION_AlignedFeatures(PXSTATE_CONFIGURATION XStateConfig)
{
    ULONG NtDdiVersion = GetXStateNtDdiVersion();
    if (IsReactOS())
    {
        return XStateConfig->AlignedFeatures;
    }
    else if (NtDdiVersion >= NTDDI_WIN10)
    {
        TXSTATE_CONFIGURATION<NTDDI_WIN10>* XStateConfigWin10 = (TXSTATE_CONFIGURATION<NTDDI_WIN10>*)XStateConfig;
        return XStateConfigWin10->AlignedFeatures;
    }
    else
    {
        return 0;
    }
}

EXTERN_C ULONG XSTATE_CONFIGURATION_ControlFlags(PXSTATE_CONFIGURATION XStateConfig)
{
    ULONG NtDdiVersion = GetXStateNtDdiVersion();

    if (IsReactOS())
    {
        return XStateConfig->ControlFlags;
    }
    else if (NtDdiVersion <= NTDDI_WIN7)
    {
        TXSTATE_CONFIGURATION<NTDDI_WIN7>* XStateConfigWin7 = (TXSTATE_CONFIGURATION<NTDDI_WIN7>*)XStateConfig;
        return XStateConfigWin7->OptimizedSave;
    }
    else
    {
        TXSTATE_CONFIGURATION<NTDDI_WIN8>* XStateConfigWin8 = (TXSTATE_CONFIGURATION<NTDDI_WIN8>*)XStateConfig;
        return XStateConfigWin8->ControlFlags;
    }
}

EXTERN_C ULONG XSTATE_CONFIGURATION_CalculateSizeByComponentMask(PXSTATE_CONFIGURATION XStateConfig, ULONG64 ComponentMask)
{
    XSTATE_FEATURE* Features = XSTATE_CONFIGURATION_Features(XStateConfig);
    PULONG AllFeatures = XSTATE_CONFIGURATION_AllFeatures(XStateConfig);
    ULONG64 AlignedFeatures = XSTATE_CONFIGURATION_AlignedFeatures(XStateConfig);
    ULONG Size = 0x240;

    for (ULONG i = 2; i < 64; i++)
    {
        if (ComponentMask & (1ULL << i))
        {
            Size += AllFeatures ? AllFeatures[i] : Features[i].Size;
            if (AlignedFeatures & (1ULL << i))
            {
                /* Add padding for alignment */
                Size = ALIGN_UP_BY(Size, 64);
            }
        }
    }

    return Size;
}

EXTERN_C ULONG XSTATE_CONFIGURATION_GetXSaveSize(PXSTATE_CONFIGURATION XStateConfig, ULONG64 ComponentMask)
{
    ULONG ControlFlags = XSTATE_CONFIGURATION_ControlFlags(XStateConfig);
    if (ControlFlags & 2) // CompactionEnabled
    {
        return XSTATE_CONFIGURATION_CalculateSizeByComponentMask(XStateConfig, ComponentMask);
    }
    else
    {
        return XSTATE_CONFIGURATION_Size(XStateConfig);
    }
}
