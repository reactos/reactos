/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/settings.c
 * PURPOSE:         Device settings support functions
 * PROGRAMMERS:     Colin Finck
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"
#include "genlist.h"
#include "infsupp.h"
#include "mui.h"
#include "registry.h"

#include "settings.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

static ULONG DefaultLanguageIndex = 0;

/* FUNCTIONS ****************************************************************/

static
BOOLEAN
IsAcpiComputer(VOID)
{
    UNICODE_STRING MultiKeyPathU = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\MultifunctionAdapter");
    UNICODE_STRING IdentifierU = RTL_CONSTANT_STRING(L"Identifier");
    UNICODE_STRING AcpiBiosIdentifier = RTL_CONSTANT_STRING(L"ACPI BIOS");
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_BASIC_INFORMATION pDeviceInformation = NULL;
    ULONG DeviceInfoLength = sizeof(KEY_BASIC_INFORMATION) + 50 * sizeof(WCHAR);
    PKEY_VALUE_PARTIAL_INFORMATION pValueInformation = NULL;
    ULONG ValueInfoLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 50 * sizeof(WCHAR);
    ULONG RequiredSize;
    ULONG IndexDevice = 0;
    UNICODE_STRING DeviceName, ValueName;
    HANDLE hDevicesKey = NULL;
    HANDLE hDeviceKey = NULL;
    NTSTATUS Status;
    BOOLEAN ret = FALSE;

    InitializeObjectAttributes(&ObjectAttributes,
                               &MultiKeyPathU,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&hDevicesKey,
                       KEY_ENUMERATE_SUB_KEYS,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtOpenKey() failed with status 0x%08lx\n", Status);
        goto cleanup;
    }

    pDeviceInformation = RtlAllocateHeap(RtlGetProcessHeap(), 0, DeviceInfoLength);
    if (!pDeviceInformation)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        Status = STATUS_NO_MEMORY;
        goto cleanup;
    }

    pValueInformation = RtlAllocateHeap(RtlGetProcessHeap(), 0, ValueInfoLength);
    if (!pValueInformation)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        Status = STATUS_NO_MEMORY;
        goto cleanup;
    }

    while (TRUE)
    {
        Status = NtEnumerateKey(hDevicesKey,
                                IndexDevice,
                                KeyBasicInformation,
                                pDeviceInformation,
                                DeviceInfoLength,
                                &RequiredSize);
        if (Status == STATUS_NO_MORE_ENTRIES)
            break;
        else if (Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, pDeviceInformation);
            DeviceInfoLength = RequiredSize;
            pDeviceInformation = RtlAllocateHeap(RtlGetProcessHeap(), 0, DeviceInfoLength);
            if (!pDeviceInformation)
            {
                DPRINT("RtlAllocateHeap() failed\n");
                Status = STATUS_NO_MEMORY;
                goto cleanup;
            }
            Status = NtEnumerateKey(hDevicesKey,
                                    IndexDevice,
                                    KeyBasicInformation,
                                    pDeviceInformation,
                                    DeviceInfoLength,
                                    &RequiredSize);
        }
        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtEnumerateKey() failed with status 0x%08lx\n", Status);
            goto cleanup;
        }
        IndexDevice++;

        /* Open device key */
        DeviceName.Length = DeviceName.MaximumLength = pDeviceInformation->NameLength;
        DeviceName.Buffer = pDeviceInformation->Name;
        InitializeObjectAttributes(&ObjectAttributes,
                                   &DeviceName,
                                   OBJ_CASE_INSENSITIVE,
                                   hDevicesKey,
                                   NULL);
        Status = NtOpenKey(&hDeviceKey,
                           KEY_QUERY_VALUE,
                           &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtOpenKey() failed with status 0x%08lx\n", Status);
            goto cleanup;
        }

        /* Read identifier */
        Status = NtQueryValueKey(hDeviceKey,
                                 &IdentifierU,
                                 KeyValuePartialInformation,
                                 pValueInformation,
                                 ValueInfoLength,
                                 &RequiredSize);
        if (Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, pValueInformation);
            ValueInfoLength = RequiredSize;
            pValueInformation = RtlAllocateHeap(RtlGetProcessHeap(), 0, ValueInfoLength);
            if (!pValueInformation)
            {
                DPRINT("RtlAllocateHeap() failed\n");
                Status = STATUS_NO_MEMORY;
                goto cleanup;
            }
            Status = NtQueryValueKey(hDeviceKey,
                                     &IdentifierU,
                                     KeyValuePartialInformation,
                                     pValueInformation,
                                     ValueInfoLength,
                                     &RequiredSize);
        }
        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtQueryValueKey() failed with status 0x%08lx\n", Status);
            goto nextdevice;
        }
        else if (pValueInformation->Type != REG_SZ)
        {
            DPRINT("Wrong registry type: got 0x%lx, expected 0x%lx\n", pValueInformation->Type, REG_SZ);
            goto nextdevice;
        }

        ValueName.Length = ValueName.MaximumLength = pValueInformation->DataLength;
        ValueName.Buffer = (PWCHAR)pValueInformation->Data;
        if (ValueName.Length >= sizeof(WCHAR) && ValueName.Buffer[ValueName.Length / sizeof(WCHAR) - 1] == UNICODE_NULL)
            ValueName.Length -= sizeof(WCHAR);
        if (RtlEqualUnicodeString(&ValueName, &AcpiBiosIdentifier, FALSE))
        {
            DPRINT("Found ACPI BIOS\n");
            ret = TRUE;
            goto cleanup;
        }

nextdevice:
        NtClose(hDeviceKey);
        hDeviceKey = NULL;
    }

cleanup:
    if (pDeviceInformation)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pDeviceInformation);
    if (pValueInformation)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pValueInformation);
    if (hDevicesKey)
        NtClose(hDevicesKey);
    if (hDeviceKey)
        NtClose(hDeviceKey);
    return ret;
}

static
BOOLEAN
GetComputerIdentifier(
    OUT PWSTR Identifier,
    IN ULONG IdentifierLength)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    LPCWSTR ComputerIdentifier;
    HANDLE ProcessorsKey;
    PKEY_FULL_INFORMATION pFullInfo;
    ULONG Size, SizeNeeded;
    NTSTATUS Status;

    DPRINT("GetComputerIdentifier() called\n");

    Size = sizeof(KEY_FULL_INFORMATION);
    pFullInfo = (PKEY_FULL_INFORMATION)RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
    if (!pFullInfo)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        return FALSE;
    }

    /* Open the processors key */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&ProcessorsKey,
                       KEY_QUERY_VALUE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtOpenKey() failed (Status 0x%lx)\n", Status);
        RtlFreeHeap(RtlGetProcessHeap(), 0, pFullInfo);
        return FALSE;
    }

    /* Get number of subkeys */
    Status = NtQueryKey(ProcessorsKey,
                        KeyFullInformation,
                        pFullInfo,
                        Size,
                        &Size);
    NtClose(ProcessorsKey);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
    {
        DPRINT("NtQueryKey() failed (Status 0x%lx)\n", Status);
        RtlFreeHeap(RtlGetProcessHeap(), 0, pFullInfo);
        return FALSE;
    }

    /* Find computer identifier */
    if (pFullInfo->SubKeys == 0)
    {
        /* Something strange happened. No processor detected */
        RtlFreeHeap(RtlGetProcessHeap(), 0, pFullInfo);
        return FALSE;
    }

#ifdef _M_AMD64
    /* On x64 we are l33t and use the MP config by default (except when we use KDBG, which is broken) */
#ifndef KDBG
    ComputerIdentifier = L"X64 MP";
#else
    ComputerIdentifier = L"X64 UP";
#endif
#else
    if (IsAcpiComputer())
    {
        if (pFullInfo->SubKeys == 1)
        {
            /* Computer is mono-CPU */
            ComputerIdentifier = L"ACPI UP";
        }
        else
        {
            /* Computer is multi-CPUs */
            ComputerIdentifier = L"ACPI MP";
        }
    }
    else
    {
        if (pFullInfo->SubKeys == 1)
        {
            /* Computer is mono-CPU */
            ComputerIdentifier = L"PC UP";
        }
        else
        {
            /* Computer is multi-CPUs */
            ComputerIdentifier = L"PC MP";
        }
    }
#endif

    RtlFreeHeap(RtlGetProcessHeap(), 0, pFullInfo);

    /* Copy computer identifier to return buffer */
    SizeNeeded = (wcslen(ComputerIdentifier) + 1) * sizeof(WCHAR);
    if (SizeNeeded > IdentifierLength)
        return FALSE;

    RtlCopyMemory(Identifier, ComputerIdentifier, SizeNeeded);

    return TRUE;
}


/**
 * @brief
 * Enumerate "KeyName=KeyValue" entries from under a specified section
 * from the given TXTSETUP.SIF file.
 *
 * @param[in]   InfFile
 * An INF file handle to TXTSETUP.SIF.
 *
 * @param[in]   SectionName
 * The name of the section whose entries are to be enumerated.
 *
 * @param[in]   pContext
 * Pointer to the INF parsing context.
 *
 * @param[in]   DefaultEntry
 * TODO TODO TODO
 *
 * @param[in]   EnumEntryProc
 * User-provided callback invoked for each enumerated entry.
 *
 * @param[in]   Parameter
 * Optional parameter for the callback.
 *
 * @return
 * The total count of entries found in the section, or zero if none.
 * Returns -1 in case of failure.
 **/
static LONG
EnumEntriesFromInfSection(
    _In_ HINF InfFile,
    _In_ PCWSTR SectionName,
    _In_ PINFCONTEXT pContext,
    /**/_In_opt_ ULONG_PTR DefaultEntry,/**/ // PCWSTR DefaultKeyName
    _In_ PENUM_ENTRY_PROC EnumEntryProc,
    _In_opt_ PVOID Parameter)
{
    LONG TotalCount = 0;
    PCWSTR KeyName;
    PCWSTR KeyValue;
    UCHAR RetVal;

    if (!SpInfFindFirstLine(InfFile, SectionName, NULL, pContext))
        return -1;

    do
    {
        /*
         * NOTE: Do not use INF_GetData() as it expects INF entries of exactly
         * two fields ("key = value"); however we expect to be able to deal with
         * entries having more than two fields, the only requirement being that
         * the second field (field number 1) contains the field description.
         */
        if (!INF_GetDataField(pContext, 0, &KeyName))
        {
            DPRINT("INF_GetDataField() failed\n");
            return -1;
        }

        if (!INF_GetDataField(pContext, 1, &KeyValue))
        {
            DPRINT("INF_GetDataField() failed\n");
            INF_FreeData(KeyName);
            return -1;
        }

        RetVal = EnumEntryProc(KeyName, KeyValue, DefaultEntry, Parameter);

        INF_FreeData(KeyName);
        INF_FreeData(KeyValue);

        if (RetVal == 0)
        {
            DPRINT("EnumEntryProc() failed\n");
            return -1;
        }
        else if (RetVal == 1)
        {
            ++TotalCount;
        }
        // else if (RetVal == 2), skip the entry.

    } while (SpInfFindNextLine(pContext, pContext));

    return TotalCount;
}


static BOOLEAN
AddComputerTypeEntries(
    _In_ HINF InfFile,
    _In_ PCWSTR SectionName,
    _Inout_opt_ PCWSTR* CurrentComputerType,
    _In_ PENUM_ENTRY_PROC EnumEntryProc,
    _In_opt_ PVOID Parameter)
{
    INFCONTEXT Context;
    WCHAR ComputerIdentifier[128];
    WCHAR ComputerKey[32];
    ULONG Count1, Count2;

    /* Get the computer identification */
    if (!GetComputerIdentifier(ComputerIdentifier, _countof(ComputerIdentifier)))
        ComputerIdentifier[0] = 0;

    DPRINT("Computer identifier: '%S'\n", ComputerIdentifier);

    if (!CurrentComputerType || !*CurrentComputerType)
    {
        /* Search for matching device identifier (that should be the default one) */
        if (!SpInfFindFirstLine(InfFile, SectionName, NULL, &Context))
            return FALSE;

        do
        {
            PCWSTR KeyName;
            PCWSTR KeyValue;
            BOOLEAN FoundId;

            if (!INF_GetDataField(&Context, 1, &KeyValue))
            {
                /* FIXME: Handle error! */
                DPRINT("INF_GetDataField() failed\n");
                return FALSE;
            }

            DPRINT("KeyValue: %S\n", KeyValue);
            FoundId = !!wcsstr(ComputerIdentifier, KeyValue);
            INF_FreeData(KeyValue);

            if (!FoundId)
                continue;

            if (!INF_GetDataField(&Context, 0, &KeyName))
            {
                /* FIXME: Handle error! */
                DPRINT("INF_GetDataField() failed\n");
                return FALSE;
            }

            DPRINT("Computer key: %S\n", KeyName);
            RtlStringCchCopyW(ComputerKey, ARRAYSIZE(ComputerKey), KeyName);
            INF_FreeData(KeyName);
        } while (SpInfFindNextLine(&Context, &Context));

        CurrentComputerType = &ComputerKey;
    }

    Count1 = EnumEntriesFromInfSection(InfFile,
                                       L"Computer",
                                       &Context,
                                       (ULONG_PTR)ComputerKey,
                                       EnumEntryProc,
                                       Parameter);
    Count2 = EnumEntriesFromInfSection(InfFile,
                                       L"Computer.NT" INF_ARCH,
                                       &Context,
                                       (ULONG_PTR)ComputerKey,
                                       EnumEntryProc,
                                       Parameter);
    if ((Count1 == -1) && (Count2 == -1))
    {
        return FALSE;
    }

    // *CurrentComputerType;

    return TRUE;
}

/**
 * @brief
 * Enumerate computer type entries from the given TXTSETUP.SIF file.
 * The examined sections are: "Map.Computer", "Computer", as well as
 * the platform-specific "Map.Computer.NT**" and "Computer.NT**".
 *
 * @param[in]   InfFile
 * An INF file handle to TXTSETUP.SIF.
 *
 * @param[in,out]   CurrentComputerType
 * Optional parameter, pointing to a string pointer which:
 * - in input, provides the computer type to look for (if it is NULL,
 *   the current computer type is determined at runtime);
 * - in output, receives the current computer type, or NULL if it is
 *   not described in the TXTSETUP.SIF file.
 *
 * @param[in]   EnumEntryProc
 * User-provided callback invoked for each computer type listed in TXTSETUP.SIF
 *
 * @param[in]   Parameter
 * Optional parameter for the callback.
 *
 * @return  TRUE if entries have been successfully enumerated; FALSE if not.
 **/
BOOLEAN
EnumComputerTypeEntries(
    _In_ HINF InfFile,
    _Inout_opt_ PCWSTR* CurrentComputerType,
    _In_ PENUM_ENTRY_PROC EnumEntryProc,
    _In_opt_ PVOID Parameter)
{
    BOOLEAN Success;

    Success =  AddComputerTypeEntries(InfFile, L"Map.Computer",
                                      CurrentComputerType,
                                      EnumEntryProc, Parameter);
    Success |= AddComputerTypeEntries(InfFile, L"Map.Computer.NT" INF_ARCH,
                                      CurrentComputerType,
                                      EnumEntryProc, Parameter);
    return Success;
}


static
BOOLEAN
GetDisplayIdentifier(
    OUT PWSTR Identifier,
    IN ULONG IdentifierLength)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    WCHAR Buffer[32];
    HANDLE BusKey;
    HANDLE BusInstanceKey;
    HANDLE ControllerKey;
    HANDLE ControllerInstanceKey;
    ULONG BusInstance;
    ULONG ControllerInstance;
    ULONG BufferLength;
    ULONG ReturnedLength;
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    NTSTATUS Status;

    DPRINT("GetDisplayIdentifier() called\n");

    /* Open the bus key */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\HARDWARE\\Description\\System\\MultifunctionAdapter");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&BusKey,
                       KEY_ENUMERATE_SUB_KEYS,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtOpenKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    BusInstance = 0;
    while (TRUE)
    {
        RtlStringCchPrintfW(Buffer, ARRAYSIZE(Buffer), L"%lu", BusInstance);
        RtlInitUnicodeString(&KeyName, Buffer);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   BusKey,
                                   NULL);

        Status = NtOpenKey(&BusInstanceKey,
                           KEY_ENUMERATE_SUB_KEYS,
                           &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtOpenKey() failed (Status %lx)\n", Status);
            NtClose(BusKey);
            return FALSE;
        }

        /* Open the controller type key */
        RtlInitUnicodeString(&KeyName, L"DisplayController");
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   BusInstanceKey,
                                   NULL);

        Status = NtOpenKey(&ControllerKey,
                           KEY_ENUMERATE_SUB_KEYS,
                           &ObjectAttributes);
        if (NT_SUCCESS(Status))
        {
            ControllerInstance = 0;

            while (TRUE)
            {
                /* Open the pointer controller instance key */
                RtlStringCchPrintfW(Buffer, ARRAYSIZE(Buffer), L"%lu", ControllerInstance);
                RtlInitUnicodeString(&KeyName, Buffer);
                InitializeObjectAttributes(&ObjectAttributes,
                                           &KeyName,
                                           OBJ_CASE_INSENSITIVE,
                                           ControllerKey,
                                           NULL);

                Status = NtOpenKey(&ControllerInstanceKey,
                                   KEY_QUERY_VALUE,
                                   &ObjectAttributes);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT("NtOpenKey() failed (Status %lx)\n", Status);
                    NtClose(ControllerKey);
                    NtClose(BusInstanceKey);
                    NtClose(BusKey);
                    return FALSE;
                }

                /* Get controller identifier */
                RtlInitUnicodeString(&KeyName, L"Identifier");

                BufferLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                               256 * sizeof(WCHAR);
                ValueInfo = (KEY_VALUE_PARTIAL_INFORMATION*) RtlAllocateHeap(RtlGetProcessHeap(),
                                                                             0,
                                                                             BufferLength);
                if (ValueInfo == NULL)
                {
                    DPRINT("RtlAllocateHeap() failed\n");
                    NtClose(ControllerInstanceKey);
                    NtClose(ControllerKey);
                    NtClose(BusInstanceKey);
                    NtClose(BusKey);
                    return FALSE;
                }

                Status = NtQueryValueKey(ControllerInstanceKey,
                                         &KeyName,
                                         KeyValuePartialInformation,
                                         ValueInfo,
                                         BufferLength,
                                         &ReturnedLength);
                if (NT_SUCCESS(Status))
                {
                    DPRINT("Identifier: %S\n", (PWSTR)ValueInfo->Data);

                    BufferLength = min(ValueInfo->DataLength / sizeof(WCHAR), IdentifierLength);
                    RtlCopyMemory(Identifier,
                                  ValueInfo->Data,
                                  BufferLength * sizeof(WCHAR));
                    Identifier[BufferLength] = 0;

                    RtlFreeHeap(RtlGetProcessHeap(),
                                0,
                                ValueInfo);

                    NtClose(ControllerInstanceKey);
                    NtClose(ControllerKey);
                    NtClose(BusInstanceKey);
                    NtClose(BusKey);
                    return TRUE;
                }

                NtClose(ControllerInstanceKey);

                ControllerInstance++;
            }

            NtClose(ControllerKey);
        }

        NtClose(BusInstanceKey);

        BusInstance++;
    }

    NtClose(BusKey);

    return FALSE;
}

/**
 * @brief
 * Enumerate display driver entries from the given TXTSETUP.SIF file.
 * The examined sections are: "Map.Display" and "Display".
 *
 * @param[in]   InfFile
 * An INF file handle to TXTSETUP.SIF.
 *
 * @param[in,out]   CurrentDisplayType
 * Optional parameter, pointing to a string pointer which:
 * - in input, provides the display type to look for (if it is NULL,
 *   the current display type is determined at runtime);
 * - in output, receives the current display type, or NULL if it is
 *   not described in the TXTSETUP.SIF file.
 *
 * @param[in]   EnumEntryProc
 * User-provided callback invoked for each display type listed in TXTSETUP.SIF
 *
 * @param[in]   Parameter
 * Optional parameter for the callback.
 *
 * @return  TRUE if entries have been successfully enumerated; FALSE if not.
 **/
BOOLEAN
EnumDisplayDriverEntries(
    _In_ HINF InfFile,
    _Inout_opt_ PCWSTR* CurrentDisplayType,
    _In_ PENUM_ENTRY_PROC EnumEntryProc,
    _In_opt_ PVOID Parameter)
{
    INFCONTEXT Context;
    WCHAR DisplayIdentifier[128];
    WCHAR DisplayKey[32];

    /* Get the display identification */
    if (!GetDisplayIdentifier(DisplayIdentifier, _countof(DisplayIdentifier)))
        DisplayIdentifier[0] = 0;

    DPRINT("Display identifier: '%S'\n", DisplayIdentifier);

    if (!CurrentDisplayType || !*CurrentDisplayType)
    {
        /* Search for matching device identifier (that should be the default one) */
        if (!SpInfFindFirstLine(InfFile, L"Map.Display", NULL, &Context))
            return FALSE;

        do
        {
            PCWSTR KeyName;
            PCWSTR KeyValue;
            BOOLEAN FoundId;

            if (!INF_GetDataField(&Context, 1, &KeyValue))
            {
                /* FIXME: Handle error! */
                DPRINT("INF_GetDataField() failed\n");
                return FALSE;
            }

            DPRINT("KeyValue: %S\n", KeyValue);
            FoundId = !!wcsstr(DisplayIdentifier, KeyValue);
            INF_FreeData(KeyValue);

            if (!FoundId)
                continue;

            if (!INF_GetDataField(&Context, 0, &KeyName))
            {
                /* FIXME: Handle error! */
                DPRINT("INF_GetDataField() failed\n");
                return FALSE;
            }

            DPRINT("Display key: %S\n", KeyName);
            RtlStringCchCopyW(DisplayKey, ARRAYSIZE(DisplayKey), KeyName);
            INF_FreeData(KeyName);
        } while (SpInfFindNextLine(&Context, &Context));

        CurrentDisplayType = &DisplayKey;
    }

    if (EnumEntriesFromInfSection(InfFile,
                                  L"Display",
                                  &Context,
                                  (ULONG_PTR)DisplayKey,
                                  EnumEntryProc,
                                  Parameter) == -1)
    {
        return FALSE;
    }

#if 0
    // FIXME: How to express that this one should be the default one?
    EnumEntryProc(L"Other display driver", NULL, /*DisplayKey*/NULL, Parameter);
    // AppendGenericListEntry(List, L"Other display driver", NULL, TRUE);
#endif

    return TRUE;
}


/**
 * @brief
 * Enumerate keyboard driver entries from the given TXTSETUP.SIF file.
 * The examined sections are: "Map.Keyboard" and "Keyboard".
 *
 * @param[in]   InfFile
 * An INF file handle to TXTSETUP.SIF.
 *
 * @param[in,out]   CurrentKeyboardType
 * Optional parameter, pointing to a string pointer which:
 * - in input, provides the keyboard type to look for (if it is NULL,
 *   the current keyboard type is determined at runtime);
 * - in output, receives the current keyboard type, or NULL if it is
 *   not described in the TXTSETUP.SIF file.
 *
 * @param[in]   EnumEntryProc
 * User-provided callback invoked for each keyboard type listed in TXTSETUP.SIF
 *
 * @param[in]   Parameter
 * Optional parameter for the callback.
 *
 * @return  TRUE if entries have been successfully enumerated; FALSE if not.
 **/
BOOLEAN
EnumKeyboardDriverEntries(
    _In_ HINF InfFile,
    _In_ PENUM_ENTRY_PROC EnumEntryProc,
    _In_opt_ PVOID Parameter)
{
    INFCONTEXT Context;

    // TODO: Handle "Map.Keyboard"

    return (EnumEntriesFromInfSection(InfFile,
                                      L"Keyboard",
                                      &Context,
                                      (ULONG_PTR)NULL,
                                      EnumEntryProc,
                                      Parameter) != -1);
}


/**
 * @brief
 * Enumerate language entries from the given TXTSETUP.SIF file.
 * The examined values are: "NLS\DefaultLanguage" (ReactOS-specific)
 * and "NLS\DefaultLayout", and the examined sections are: "Language".
 *
 * @param[in]   InfFile
 * An INF file handle to TXTSETUP.SIF.
 *
 * @param[in,out]   CurrentKeyboardType
 * Optional parameter, pointing to a string pointer which:
 * - in input, provides the language to look for (if it is NULL,
 *   the current language is determined at runtime);
 * - in output, receives the current language, or NULL if it is
 *   not described in the TXTSETUP.SIF file.
 *
 * @param[in]   EnumEntryProc
 * User-provided callback invoked for each language listed in TXTSETUP.SIF
 *
 * @param[in]   Parameter
 * Optional parameter for the callback.
 *
 * @return  TRUE if entries have been successfully enumerated; FALSE if not.
 **/
BOOLEAN
EnumLanguageEntries(
    _In_ HINF InfFile,
    _Out_opt_ LANGID* DefaultLanguage,
    _In_ PENUM_ENTRY_PROC EnumEntryProc,
    _In_opt_ PVOID Parameter)
{
    INFCONTEXT Context;

    if (DefaultLanguage)
    {
        /* Get default language ID (ReactOS-specific);
         * if none, use the one from "DefaultLayout" */
        if (SpInfFindFirstLine(InfFile, L"NLS", L"DefaultLanguage", &Context) ||
            SpInfFindFirstLine(InfFile, L"NLS", L"DefaultLayout", &Context))
        {
            /* This is a <= 8-character long hexadecimal string */
            PCWSTR KeyValue;
            if (!INF_GetData(&Context, NULL, &KeyValue))
                return FALSE;
            *DefaultLanguage = (LANGID)(wcstoul(KeyValue, NULL, 16) & 0xFFFF);
        }
        else
        {
            /* None, default to en-US */
            DPRINT1("No NLS\\DefaultLayout value, default to en-US\n");
            *DefaultLanguage = 0x0409;
        }
    }

    if (EnumEntriesFromInfSection(InfFile,
                                  L"Language",
                                  &Context,
                                  (ULONG_PTR)*DefaultLanguage,
                                  EnumEntryProc,
                                  Parameter) == -1)
    {
        // FIXME: Just fall back to en-US
        return FALSE;
    }

    return TRUE;
}


/**
 * @brief
 * Enumerate keyboard layout entries from the given TXTSETUP.SIF file.
 * The examined values are: "NLS\DefaultLayout", and the examined sections
 * are: "KeyboardLayout" (NOTE: ReactOS-specific; on Windows it is named
 * "Keyboard Layout" instead).
 *
 * @param[in]   InfFile
 * An INF file handle to TXTSETUP.SIF.
 *
 * @param[in]   LanguageId
 * The language ID for which to enumerate corresponding keyboard layouts.
 *
 * @param[in,out]   CurrentKeyboardType
 * Optional parameter, pointing to a string pointer which:
 * - in input, provides the keyboard layout to look for (if it is NULL,
 *   the current keyboard layout is determined at runtime);
 * - in output, receives the current keyboard layout, or NULL if it is
 *   not described in the TXTSETUP.SIF file.
 *
 * @param[in]   EnumEntryProc
 * User-provided callback invoked for each keyboard layout listed in TXTSETUP.SIF
 *
 * @param[in]   Parameter
 * Optional parameter for the callback.
 *
 * @return  TRUE if entries have been successfully enumerated; FALSE if not.
 **/
BOOLEAN
EnumKeyboardLayoutEntries(
    _In_ HINF InfFile,
    _In_ LANGID LanguageId,
    _Out_ KLID* DefaultKBLayout,
    _In_ PENUM_ENTRY_PROC EnumEntryProc,
    _In_opt_ PVOID Parameter)
{
    INFCONTEXT Context;
    const MUI_LAYOUTS* LayoutsList;
    ULONG uIndex;

    /* Get default layout ID */
    if (SpInfFindFirstLine(InfFile, L"NLS", L"DefaultLayout", &Context))
    {
        /* This is a <= 8-character long hexadecimal string */
        PCWSTR KeyValue;
        if (!INF_GetData(&Context, NULL, &KeyValue))
            return FALSE;
        *DefaultKBLayout = (KLID)wcstoul(KeyValue, NULL, 16);
    }
    else
    {
        /* None, default to en-US */
        DPRINT1("No NLS\\DefaultLayout value, default to en-US\n");
        *DefaultKBLayout = 0x00000409;
    }

    LayoutsList = MUIGetLayoutsList(LanguageId);
    for (uIndex = 0; LayoutsList[uIndex].LangID != 0; ++uIndex)
    {
        // NOTE: See https://git.reactos.org/?p=reactos.git;a=commit;h=785b2eb8b8c6222bb961f5368d51f1a50640dc96
        // "Make the keyboard layouts selection not dependent on the selected language." r68354 CORE-9630
        if (EnumEntriesFromInfSection(InfFile,
                                      L"KeyboardLayout", // NOTE: Windows TXTSETUP.SIF names this: "Keyboard Layout"
                                      &Context,
                                      (ULONG_PTR)DefaultKBLayout,
                                      EnumEntryProc,
                                      Parameter) == -1)
        {
            return FALSE;
        }
    }

    return TRUE;
}


ULONG
GetDefaultLanguageIndex(VOID)
{
    return DefaultLanguageIndex;
}




//
// Callback to add an entry to a PGENERIC_LIST -- For USETUP only
//
static UCHAR
NTAPI
DefaultProcessEntry(
    _In_ PCWSTR KeyName,
    _In_opt_ PCWSTR KeyValue,
    /**/_In_ ULONG_PTR DefaultEntry,/**/ // PCWSTR DefaultKeyName
    _In_opt_ PVOID Parameter)
{
    PCWSTR CompareKey = (PCWSTR)DefaultEntry;
    PGENERIC_LIST List = (PGENERIC_LIST)Parameter;
    PGENENTRY GenEntry;
    SIZE_T IdSize, ValueSize;
    BOOLEAN Current;

    IdSize    = (wcslen(KeyName)  + 1) * sizeof(WCHAR);
    ValueSize = (wcslen(KeyValue) + 1) * sizeof(WCHAR);

    GenEntry = RtlAllocateHeap(ProcessHeap, 0,
                               sizeof(*GenEntry) + IdSize + ValueSize);
    if (!GenEntry)
    {
        /* Failure, stop enumeration */
        DPRINT1("RtlAllocateHeap() failed\n");
        return 0;
    }

    GenEntry->Id.Str = (PCWSTR)((ULONG_PTR)GenEntry + sizeof(*GenEntry));
    GenEntry->Value  = (PCWSTR)((ULONG_PTR)GenEntry + sizeof(*GenEntry) + IdSize);
    RtlStringCbCopyW((PWSTR)GenEntry->Id.Str, IdSize, KeyName);
    RtlStringCbCopyW((PWSTR)GenEntry->Value, ValueSize, KeyValue);

    Current = (CompareKey ? !_wcsicmp(KeyName, CompareKey) : FALSE);

    /* Add the entry */
    if (AppendGenericListEntry(List, GenEntry, Current))
    {
        return 1;
    }
    else
    {
        /* Failure, stop enumeration */
        RtlFreeHeap(ProcessHeap, 0, GenEntry);
        return 0;
    }
}

//
// This is for USETUP. GUI setup can enumerate within a combolist.
//
PGENERIC_LIST
CreateComputerTypeList(
    _In_ HINF InfFile)
{
    PGENERIC_LIST List;

    List = CreateGenericList();
    if (!List)
        return NULL;

    if (!EnumComputerTypeEntries(InfFile, DefaultProcessEntry, List))
    {
        DestroyGenericList(List, TRUE);
        return NULL;
    }

    return List;
}

//
// This is for USETUP. GUI setup can enumerate within a combolist.
//
PGENERIC_LIST
CreateDisplayDriverList(
    _In_ HINF InfFile)
{
    PGENERIC_LIST List;

    List = CreateGenericList();
    if (!List)
        return NULL;

    if (!EnumDisplayDriverEntries(InfFile, DefaultProcessEntry, List))
    {
        DestroyGenericList(List, TRUE);
        return NULL;
    }

    return List;
}

//
// This is for USETUP. GUI setup can enumerate within a combolist.
//
PGENERIC_LIST
CreateKeyboardDriverList(
    _In_ HINF InfFile)
{
    PGENERIC_LIST List;

    List = CreateGenericList();
    if (!List)
        return NULL;

    if (!EnumKeyboardDriverEntries(InfFile, DefaultProcessEntry, List))
    {
        DestroyGenericList(List, TRUE);
        return NULL;
    }

    return List;
}


typedef struct _LANG_ENTRY_PARAM
{
    PGENERIC_LIST List;
    ULONG uIndex;
    LANGID DefaultLanguage;
} LANG_ENTRY_PARAM, *PLANG_ENTRY_PARAM;

static UCHAR
NTAPI
ProcessLangEntry(
    _In_ PCWSTR KeyName,
    _In_opt_ PCWSTR KeyValue,
    /**/_In_ ULONG_PTR DefaultEntry,/**/ // PCWSTR DefaultKeyName
    _In_opt_ PVOID Parameter)
{
    PLANG_ENTRY_PARAM LangEntryParam = (PLANG_ENTRY_PARAM)Parameter;
    PGENERIC_LIST List = LangEntryParam->List;
    PGENENTRY GenEntry;
    SIZE_T ValueSize;
    LANGID LangId;
    BOOLEAN Current;

    LangId = (LANGID)(wcstoul(KeyName, NULL, 16) & 0xFFFF);
    if (!IsLanguageAvailable(LangId))
    {
        /* The specified language is unavailable, skip the entry */
        return 2;
    }

    ValueSize = (wcslen(KeyValue) + 1) * sizeof(WCHAR);

    GenEntry = RtlAllocateHeap(ProcessHeap, 0,
                               sizeof(*GenEntry) + ValueSize);
    if (!GenEntry)
    {
        /* Failure, stop enumeration */
        DPRINT1("RtlAllocateHeap() failed\n");
        return 0;
    }

    GenEntry->Id.Ul = (ULONG_PTR)LangId;
    GenEntry->Value = (PCWSTR)((ULONG_PTR)GenEntry + sizeof(*GenEntry));
    RtlStringCbCopyW((PWSTR)GenEntry->Value, ValueSize, KeyValue);

    Current = FALSE;

    if (LangId == LangEntryParam->DefaultLanguage)
    {
        DefaultLanguageIndex = LangEntryParam->uIndex;
        Current = TRUE;
    }

    LangEntryParam->uIndex++;

    /* Add the entry */
    if (AppendGenericListEntry(List, GenEntry, Current))
    {
        return 1;
    }
    else
    {
        /* Failure, stop enumeration */
        RtlFreeHeap(ProcessHeap, 0, GenEntry);
        return 0;
    }
}

//
// This is for USETUP. GUI setup can enumerate within a combolist.
//
PGENERIC_LIST
CreateLanguageList(
    _In_ HINF InfFile,
    _Inout_ LANGID* DefaultLanguage)
{
    PGENERIC_LIST List;
    LANG_ENTRY_PARAM LangEntryParam;
#if 0
    LANGID OrigDefaultLanguage = *DefaultLanguage;
#endif

    List = CreateGenericList();
    if (!List)
        return NULL;

    LangEntryParam.List = List;
    LangEntryParam.uIndex = 0;
    LangEntryParam.DefaultLanguage = *DefaultLanguage;

    if (!EnumLanguageEntries(InfFile, DefaultLanguage,
                             ProcessLangEntry/*DefaultProcessEntry*/,
                             &LangEntryParam))
    {
        // FIXME: Just fall back to en-US
        DestroyGenericList(List, TRUE);
        return NULL;
    }

    /* Only one language available, make it the default one */
    if (LangEntryParam.uIndex == 1)
    {
        DefaultLanguageIndex = 0;
        SetCurrentListEntry(List, GetFirstListEntry(List));
        *DefaultLanguage = (LANGID)(((PGENENTRY)GetListEntryData(GetFirstListEntry(List)))->Id.Ul);
    }


//
// FIXME: Check whether such a hack (inspired from
// the IsUnattendedSetup thing) is still necessary...
//
#if 0
    if (OrigDefaultLanguage != 0)
    {
        PGENERIC_LIST_ENTRY ListEntry;
        LCID LocaleId;

        /* first we hack LanguageList */
        for (ListEntry = GetFirstListEntry(List); ListEntry;
             ListEntry = GetNextListEntry(ListEntry))
        {
            LocaleId = (LCID)(((PGENENTRY)GetListEntryData(ListEntry))->Id.Ul);
            if (OrigDefaultLanguage == (LANGID)LocaleId)
            {
                DPRINT("Found 0x%08lx in LanguageList\n", LocaleId);
                SetCurrentListEntry(List, ListEntry);
                break;
            }
        }
    }
#endif


    return List;
}


static UCHAR
NTAPI
ProcessLayoutEntry(
    _In_ PCWSTR KeyName,
    _In_opt_ PCWSTR KeyValue,
    /**/_In_ ULONG_PTR DefaultEntry,/**/ // PCWSTR DefaultKeyName
    _In_opt_ PVOID Parameter)
{
    PGENERIC_LIST List = (PGENERIC_LIST)Parameter;
    PGENENTRY GenEntry;
    SIZE_T ValueSize;
    KLID LayoutId;
    BOOLEAN Current;

    LayoutId = (KLID)wcstoul(KeyName, NULL, 16);

    ValueSize = (wcslen(KeyValue) + 1) * sizeof(WCHAR);

    GenEntry = RtlAllocateHeap(ProcessHeap, 0,
                               sizeof(*GenEntry) + ValueSize);
    if (!GenEntry)
    {
        /* Failure, stop enumeration */
        DPRINT1("RtlAllocateHeap() failed\n");
        return 0;
    }

    GenEntry->Id.Ul = (ULONG_PTR)LayoutId;
    GenEntry->Value = (PCWSTR)((ULONG_PTR)GenEntry + sizeof(*GenEntry));
    RtlStringCbCopyW((PWSTR)GenEntry->Value, ValueSize, KeyValue);

    Current = FALSE; // (LayoutId == (KLID)DefaultEntry);

    /* Add the entry */
    if (AppendGenericListEntry(List, GenEntry, Current))
    {
        return 1;
    }
    else
    {
        /* Failure, stop enumeration */
        RtlFreeHeap(ProcessHeap, 0, GenEntry);
        return 0;
    }
}

//
// This is for USETUP. GUI setup can enumerate within a combolist.
//
PGENERIC_LIST
CreateKeyboardLayoutList(
    _In_ HINF InfFile,
    _In_ LANGID LanguageId,
    _Out_ KLID* DefaultKBLayout) // FIXME: Investigate whether this could be Inout (see usetup.c!UpdateKBLayout)
{
    PGENERIC_LIST List;

    List = CreateGenericList();
    if (!List)
        return NULL;

    if (!EnumKeyboardLayoutEntries(InfFile,
                                   LanguageId,
                                   DefaultKBLayout,
                                   ProcessLayoutEntry,
                                   List))
    {
        // FIXME: Just fall back to en-US
        DestroyGenericList(List, TRUE);
        return NULL;
    }

    /* Check whether some keyboard layouts have been found */
    /* FIXME: Handle this case */
#if 0 // Actually this code is useless.
    if (GetNumberOfListEntries(List) == 0)
    {
        DPRINT1("No keyboard layouts have been found\n");
        DestroyGenericList(List, TRUE);
        return NULL;
    }
#endif

    return List;
}





BOOLEAN
ProcessComputerFiles(
    _In_ HINF InfFile,
    _In_ PCWSTR ComputerType,
    _Out_ PWSTR* AdditionalSectionName)
{
    static WCHAR SectionName[128];

    DPRINT("ProcessComputerFiles(%S) called\n", ComputerType);

    RtlStringCchPrintfW(SectionName, _countof(SectionName),
                        L"Files.%s", ComputerType);
    *AdditionalSectionName = SectionName;

    // TODO: More things to do?

    return TRUE;
}

BOOLEAN
ProcessDisplayRegistry(
    _In_ HINF InfFile,
    _In_ PCWSTR DisplayType)
{
    NTSTATUS Status;
    INFCONTEXT Context;
    PCWSTR Buffer;
    PCWSTR ServiceName;
    ULONG StartValue;
    ULONG Width, Height, Bpp;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE KeyHandle;
    WCHAR RegPath[255];

    DPRINT("ProcessDisplayRegistry(%S) called\n", DisplayType);

    if (!SpInfFindFirstLine(InfFile, L"Display", DisplayType, &Context))
    {
        DPRINT1("SpInfFindFirstLine() failed\n");
        return FALSE;
    }

    /* Enable the correct driver */
    if (!INF_GetDataField(&Context, 3, &ServiceName))
    {
        DPRINT1("INF_GetDataField() failed\n");
        return FALSE;
    }

    ASSERT(wcslen(ServiceName) < 10);
    DPRINT("Service name: '%S'\n", ServiceName);

    RtlStringCchPrintfW(RegPath, ARRAYSIZE(RegPath),
                        L"System\\CurrentControlSet\\Services\\%s",
                        ServiceName);
    RtlInitUnicodeString(&KeyName, RegPath);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               GetRootKeyByPredefKey(HKEY_LOCAL_MACHINE, NULL),
                               NULL);
    Status = NtOpenKey(&KeyHandle,
                       KEY_SET_VALUE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    StartValue = 1;
    Status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE, KeyHandle,
                                   L"Start",
                                   REG_DWORD,
                                   &StartValue,
                                   sizeof(StartValue));
    NtClose(KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlWriteRegistryValue() failed (Status %lx)\n", Status);
        return FALSE;
    }

    /* Set the resolution */

    if (!INF_GetDataField(&Context, 4, &Buffer))
    {
        DPRINT1("INF_GetDataField() failed\n");
        return FALSE;
    }

    RtlStringCchPrintfW(RegPath, ARRAYSIZE(RegPath),
                        L"System\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\%s\\Device0",
                        ServiceName);
    DPRINT("RegPath: '%S'\n", RegPath);
    RtlInitUnicodeString(&KeyName, RegPath);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               GetRootKeyByPredefKey(HKEY_LOCAL_MACHINE, NULL),
                               NULL);
    Status = NtOpenKey(&KeyHandle,
                       KEY_SET_VALUE,
                       &ObjectAttributes);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        /* Try without Hardware Profile part */
        RtlStringCchPrintfW(RegPath, ARRAYSIZE(RegPath),
                            L"System\\CurrentControlSet\\Services\\%s\\Device0",
                            ServiceName);
        RtlInitUnicodeString(&KeyName, RegPath);
        Status = NtOpenKey(&KeyHandle,
                           KEY_SET_VALUE,
                           &ObjectAttributes);
    }
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    Width = wcstoul(Buffer, NULL, 10);
    Status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE, KeyHandle,
                                   L"DefaultSettings.XResolution",
                                   REG_DWORD,
                                   &Width,
                                   sizeof(Width));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlWriteRegistryValue() failed (Status %lx)\n", Status);
        NtClose(KeyHandle);
        return FALSE;
    }

    if (!INF_GetDataField(&Context, 5, &Buffer))
    {
        DPRINT1("INF_GetDataField() failed\n");
        NtClose(KeyHandle);
        return FALSE;
    }

    Height = wcstoul(Buffer, NULL, 10);
    Status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE, KeyHandle,
                                   L"DefaultSettings.YResolution",
                                   REG_DWORD,
                                   &Height,
                                   sizeof(Height));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlWriteRegistryValue() failed (Status %lx)\n", Status);
        NtClose(KeyHandle);
        return FALSE;
    }

    if (!INF_GetDataField(&Context, 6, &Buffer))
    {
        DPRINT1("INF_GetDataField() failed\n");
        NtClose(KeyHandle);
        return FALSE;
    }

    Bpp = wcstoul(Buffer, NULL, 10);
    Status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE, KeyHandle,
                                   L"DefaultSettings.BitsPerPel",
                                   REG_DWORD,
                                   &Bpp,
                                   sizeof(Bpp));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlWriteRegistryValue() failed (Status %lx)\n", Status);
        NtClose(KeyHandle);
        return FALSE;
    }

    NtClose(KeyHandle);

    DPRINT("ProcessDisplayRegistry() done\n");

    return TRUE;
}

BOOLEAN
ProcessLocaleRegistry(
    _In_ LCID LocaleId)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    NTSTATUS Status;
    WCHAR Value[sizeof("FFFFFFFF")];

    Status = RtlStringCchPrintfW(Value, _countof(Value), L"%08lx", LocaleId);
    ASSERT(NT_SUCCESS(Status));

    DPRINT("LocaleId: %S\n", Value);

    /* Open the default users locale key */
    RtlInitUnicodeString(&KeyName,
                         L".DEFAULT\\Control Panel\\International");

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               GetRootKeyByPredefKey(HKEY_USERS, NULL),
                               NULL);

    Status = NtOpenKey(&KeyHandle,
                       KEY_SET_VALUE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    /* Set default user locale */
    RtlInitUnicodeString(&ValueName, L"Locale");
    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_SZ,
                           (PVOID)Value,
                           (wcslen(Value) + 1) * sizeof(WCHAR));
    NtClose(KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    /* Just get the language ID */
    Status = RtlStringCchPrintfW(Value, _countof(Value), L"%04lx", LANGIDFROMLCID(LocaleId));
    ASSERT(NT_SUCCESS(Status));

    /* Open the NLS language key */
    RtlInitUnicodeString(&KeyName,
                         L"SYSTEM\\CurrentControlSet\\Control\\NLS\\Language");

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               GetRootKeyByPredefKey(HKEY_LOCAL_MACHINE, NULL),
                               NULL);

    Status = NtOpenKey(&KeyHandle,
                       KEY_SET_VALUE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    /* Set default language */
    RtlInitUnicodeString(&ValueName, L"Default");
    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_SZ,
                           (PVOID)Value,
                           (wcslen(Value) + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
        NtClose(KeyHandle);
        return FALSE;
    }

    /* Set install language */
    RtlInitUnicodeString(&ValueName, L"InstallLanguage");
    Status = NtSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_SZ,
                           (PVOID)Value,
                           (wcslen(Value) + 1) * sizeof(WCHAR));
    NtClose(KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
ProcessKeyboardLayoutRegistry(
    _In_ KLID LayoutId,
    _In_ LANGID LanguageId)
{
    const MUI_LAYOUTS* LayoutsList;
    MUI_LAYOUTS NewLayoutsList[20]; // HACK: Hardcoded fixed size "20" is a hack. Please verify against lang/*.h
    ULONG uIndex;
    ULONG uOldPos = 0;

    LayoutsList = MUIGetLayoutsList(LanguageId);

    /* If the keyboard layout is already at the top of the list, we are done */
    if (LayoutsList[0].LayoutID == LayoutId)
        return TRUE;
    // TODO: Shouldn't we add it to the registry as well?
    // Note that AddKeyboardLayouts() also calls AddKbLayoutsToRegistry().

    /* Otherwise, move it up to the top of the list */
    for (uIndex = 1; LayoutsList[uIndex].LangID != 0; ++uIndex)
    {
        if (LayoutsList[uIndex].LayoutID == LayoutId)
        {
            uOldPos = uIndex;
            continue;
        }

        NewLayoutsList[uIndex].LangID   = LayoutsList[uIndex].LangID;
        NewLayoutsList[uIndex].LayoutID = LayoutsList[uIndex].LayoutID;
    }

    NewLayoutsList[uIndex].LangID    = 0;
    NewLayoutsList[uIndex].LayoutID  = 0;
    NewLayoutsList[uOldPos].LangID   = LayoutsList[0].LangID;
    NewLayoutsList[uOldPos].LayoutID = LayoutsList[0].LayoutID;
    NewLayoutsList[0].LangID         = LayoutsList[uOldPos].LangID;
    NewLayoutsList[0].LayoutID       = LayoutsList[uOldPos].LayoutID;

    return AddKbLayoutsToRegistry(NewLayoutsList);
}

#if 0
BOOLEAN
ProcessKeyboardLayoutFiles(
    IN PGENERIC_LIST List) // _In_ PCWSTR KeyboardDriver
{
    return TRUE;
}
#endif

BOOLEAN
SetGeoID(
    _In_ GEOID GeoId)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;
    HANDLE KeyHandle;
    /*
     * Buffer big enough to hold the NULL-terminated string L"4294967295",
     * corresponding to the literal 0xFFFFFFFF (MAXULONG) in decimal.
     */
    WCHAR Value[sizeof("4294967295")];

    Status = RtlStringCchPrintfW(Value, _countof(Value), L"%lu", GeoId);
    ASSERT(NT_SUCCESS(Status));

    RtlInitUnicodeString(&Name,
                         L".DEFAULT\\Control Panel\\International\\Geo");
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               GetRootKeyByPredefKey(HKEY_USERS, NULL),
                               NULL);
    Status = NtOpenKey(&KeyHandle,
                       KEY_SET_VALUE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    RtlInitUnicodeString(&Name, L"Nation");
    Status = NtSetValueKey(KeyHandle,
                           &Name,
                           0,
                           REG_SZ,
                           (PVOID)Value,
                           (wcslen(Value) + 1) * sizeof(WCHAR));
    NtClose(KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
SetDefaultPagefile(
    _In_ WCHAR Drive)
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management");
    UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"PagingFiles");
    WCHAR ValueBuffer[] = L"?:\\pagefile.sys 0 0\0";

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               GetRootKeyByPredefKey(HKEY_LOCAL_MACHINE, NULL),
                               NULL);
    Status = NtOpenKey(&KeyHandle,
                       KEY_ALL_ACCESS,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
        return FALSE;

    ValueBuffer[0] = Drive;

    NtSetValueKey(KeyHandle,
                  &ValueName,
                  0,
                  REG_MULTI_SZ,
                  (PVOID)&ValueBuffer,
                  sizeof(ValueBuffer));

    NtClose(KeyHandle);
    return TRUE;
}

/* EOF */
