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
 * PROGRAMMERS:     Eric Kohl
 *                  Colin Finck
 */

/* INCLUDES *****************************************************************/

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

ULONG DefaultLanguageIndex = 0;

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
    PWSTR Identifier,
    ULONG IdentifierLength)
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

    RtlFreeHeap(RtlGetProcessHeap(), 0, pFullInfo);

    /* Copy computer identifier to return buffer */
    SizeNeeded = (wcslen(ComputerIdentifier) + 1) * sizeof(WCHAR);
    if (SizeNeeded > IdentifierLength)
        return FALSE;

    RtlCopyMemory(Identifier, ComputerIdentifier, SizeNeeded);

    return TRUE;
}


PGENERIC_LIST
CreateComputerTypeList(
    HINF InfFile)
{
    CHAR Buffer[128];
    PGENERIC_LIST List;
    INFCONTEXT Context;
    PWCHAR KeyName;
    PWCHAR KeyValue;
    PWCHAR UserData;
    WCHAR ComputerIdentifier[128];
    WCHAR ComputerKey[32];

    /* Get the computer identification */
    if (!GetComputerIdentifier(ComputerIdentifier, 128))
    {
        ComputerIdentifier[0] = 0;
    }

    DPRINT("Computer identifier: '%S'\n", ComputerIdentifier);

    /* Search for matching device identifier */
    if (!SetupFindFirstLineW(InfFile, L"Map.Computer", NULL, &Context))
    {
        /* FIXME: error message */
        return NULL;
    }

    do
    {
        if (!INF_GetDataField(&Context, 1, &KeyValue))
        {
            /* FIXME: Handle error! */
            DPRINT("INF_GetDataField() failed\n");
            return NULL;
        }

        DPRINT("KeyValue: %S\n", KeyValue);
        if (wcsstr(ComputerIdentifier, KeyValue))
        {
            INF_FreeData(KeyValue);
            if (!INF_GetDataField(&Context, 0, &KeyName))
            {
                /* FIXME: Handle error! */
                DPRINT("INF_GetDataField() failed\n");
                return NULL;
            }

            DPRINT("Computer key: %S\n", KeyName);
            wcscpy(ComputerKey, KeyName);
            INF_FreeData(KeyName);
        }
        INF_FreeData(KeyValue);
    } while (SetupFindNextLine(&Context, &Context));

    List = CreateGenericList();
    if (List == NULL)
        return NULL;

    if (!SetupFindFirstLineW(InfFile, L"Computer", NULL, &Context))
    {
        DestroyGenericList(List, FALSE);
        return NULL;
    }

    do
    {
        if (!INF_GetData(&Context, &KeyName, &KeyValue))
        {
            /* FIXME: Handle error! */
            DPRINT("INF_GetData() failed\n");
            break;
        }

        UserData = (WCHAR*) RtlAllocateHeap(ProcessHeap,
                                            0,
                                            (wcslen(KeyName) + 1) * sizeof(WCHAR));
        if (UserData == NULL)
        {
            /* FIXME: Handle error! */
        }

        wcscpy(UserData, KeyName);
        INF_FreeData(KeyName);

        sprintf(Buffer, "%S", KeyValue);
        INF_FreeData(KeyValue);

        AppendGenericListEntry(List, Buffer, UserData,
                               _wcsicmp(UserData, ComputerKey) ? FALSE : TRUE);
    } while (SetupFindNextLine(&Context, &Context));

    return List;
}

static
BOOLEAN
GetDisplayIdentifier(
    PWSTR Identifier,
    ULONG IdentifierLength)
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
        swprintf(Buffer, L"%lu", BusInstance);
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
                swprintf(Buffer, L"%lu", ControllerInstance);
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
                    RtlCopyMemory (Identifier,
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

PGENERIC_LIST
CreateDisplayDriverList(
    HINF InfFile)
{
    CHAR Buffer[128];
    PGENERIC_LIST List;
    INFCONTEXT Context;
    PWCHAR KeyName;
    PWCHAR KeyValue;
    PWCHAR UserData;
    WCHAR DisplayIdentifier[128];
    WCHAR DisplayKey[32];

    /* Get the display identification */
    if (!GetDisplayIdentifier(DisplayIdentifier, 128))
    {
        DisplayIdentifier[0] = 0;
    }

    DPRINT("Display identifier: '%S'\n", DisplayIdentifier);

    /* Search for matching device identifier */
    if (!SetupFindFirstLineW(InfFile, L"Map.Display", NULL, &Context))
    {
        /* FIXME: error message */
        return NULL;
    }

    do
    {
        if (!INF_GetDataField(&Context, 1, &KeyValue))
        {
            /* FIXME: Handle error! */
            DPRINT("INF_GetDataField() failed\n");
            return NULL;
        }

        DPRINT("KeyValue: %S\n", KeyValue);
        if (wcsstr(DisplayIdentifier, KeyValue))
        {
            INF_FreeData(KeyValue);
            if (!INF_GetDataField(&Context, 0, &KeyName))
            {
                /* FIXME: Handle error! */
                DPRINT("INF_GetDataField() failed\n");
                return NULL;
            }

            DPRINT("Display key: %S\n", KeyName);
            wcscpy(DisplayKey, KeyName);
            INF_FreeData(KeyName);
        }
        INF_FreeData(KeyValue);
    } while (SetupFindNextLine(&Context, &Context));

    List = CreateGenericList();
    if (List == NULL)
        return NULL;

    if (!SetupFindFirstLineW(InfFile, L"Display", NULL, &Context))
    {
        DestroyGenericList(List, FALSE);
        return NULL;
    }

    do
    {
        if (!INF_GetDataField(&Context, 0, &KeyName))
        {
            DPRINT1("INF_GetDataField() failed\n");
            break;
        }

        if (!INF_GetDataField(&Context, 1, &KeyValue))
        {
            DPRINT1("INF_GetDataField() failed\n");
            INF_FreeData(KeyName);
            break;
        }

        UserData = (WCHAR*) RtlAllocateHeap(ProcessHeap,
                                            0,
                                            (wcslen(KeyName) + 1) * sizeof(WCHAR));
        if (UserData == NULL)
        {
            DPRINT1("RtlAllocateHeap() failed\n");
            DestroyGenericList(List, TRUE);
            INF_FreeData(KeyValue);
            INF_FreeData(KeyName);
            return NULL;
        }

        wcscpy(UserData, KeyName);
        INF_FreeData(KeyName);

        sprintf(Buffer, "%S", KeyValue);
        INF_FreeData(KeyValue);

        AppendGenericListEntry(List, Buffer, UserData,
                               _wcsicmp(UserData, DisplayKey) ? FALSE : TRUE);
    } while (SetupFindNextLine(&Context, &Context));

#if 0
    AppendGenericListEntry(List, "Other display driver", NULL, TRUE);
#endif

    return List;
}


BOOLEAN
ProcessComputerFiles(
    HINF InfFile,
    PGENERIC_LIST List,
    PWCHAR *AdditionalSectionName)
{
    PGENERIC_LIST_ENTRY Entry;
    static WCHAR SectionName[128];

    DPRINT("ProcessComputerFiles() called\n");

    Entry = GetCurrentListEntry(List);
    if (Entry == NULL)
    {
        DPRINT("GetCurrentListEntry() failed\n");
        return FALSE;
    }

    wcscpy(SectionName, L"Files.");
    wcscat(SectionName, (const wchar_t*)GetListEntryUserData(Entry));
    *AdditionalSectionName = SectionName;

    return TRUE;
}

BOOLEAN
ProcessDisplayRegistry(
    HINF InfFile,
    PGENERIC_LIST List)
{
    NTSTATUS Status;
    PGENERIC_LIST_ENTRY Entry;
    INFCONTEXT Context;
    PWCHAR Buffer;
    PWCHAR ServiceName;
    ULONG StartValue;
    ULONG Width, Height, Bpp;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE KeyHandle;
    WCHAR RegPath[255];

    DPRINT("ProcessDisplayRegistry() called\n");

    Entry = GetCurrentListEntry(List);
    if (Entry == NULL)
    {
        DPRINT1("GetCurrentListEntry() failed\n");
        return FALSE;
    }

    if (!SetupFindFirstLineW(InfFile, L"Display", (WCHAR*)GetListEntryUserData(Entry), &Context))
    {
        DPRINT1("SetupFindFirstLineW() failed\n");
        return FALSE;
    }

    /* Enable the correct driver */
    if (!INF_GetDataField(&Context, 3, &ServiceName))
    {
        DPRINT1("INF_GetDataField() failed\n");
        return FALSE;
    }

    ASSERT(wcslen(ServiceName) < 10);
    DPRINT1("Service name: '%S'\n", ServiceName);

    swprintf(RegPath, L"System\\CurrentControlSet\\Services\\%s", ServiceName);
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

    swprintf(RegPath,
             L"System\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\%s\\Device0",
             ServiceName);
    DPRINT1("RegPath: '%S'\n", RegPath);
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

    Height = wcstoul(Buffer, 0, 0);
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

    Bpp = wcstoul(Buffer, 0, 0);
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
    PGENERIC_LIST List)
{
    PGENERIC_LIST_ENTRY Entry;
    PWCHAR LanguageId;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;

    HANDLE KeyHandle;
    NTSTATUS Status;

    Entry = GetCurrentListEntry(List);
    if (Entry == NULL)
        return FALSE;

    LanguageId = (PWCHAR)GetListEntryUserData(Entry);
    if (LanguageId == NULL)
        return FALSE;

    DPRINT("LanguageId: %S\n", LanguageId);

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
                           (PVOID)LanguageId,
                           (wcslen(LanguageId) + 1) * sizeof(WCHAR));
    NtClose(KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    /* Skip first 4 zeroes */
    if (wcslen(LanguageId) >= 4)
        LanguageId += 4;

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
                           (PVOID)LanguageId,
                           (wcslen(LanguageId) + 1) * sizeof(WCHAR));
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
                           (PVOID)LanguageId,
                           (wcslen(LanguageId) + 1) * sizeof(WCHAR));
    NtClose(KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
        return FALSE;
    }

    return TRUE;
}


PGENERIC_LIST
CreateKeyboardDriverList(
    HINF InfFile)
{
    CHAR Buffer[128];
    PGENERIC_LIST List;
    INFCONTEXT Context;
    PWCHAR KeyName;
    PWCHAR KeyValue;
    PWCHAR UserData;

    List = CreateGenericList();
    if (List == NULL)
        return NULL;

    if (!SetupFindFirstLineW(InfFile, L"Keyboard", NULL, &Context))
    {
        DestroyGenericList(List, FALSE);
        return NULL;
    }

    do
    {
        if (!INF_GetData(&Context, &KeyName, &KeyValue))
        {
            /* FIXME: Handle error! */
            DPRINT("INF_GetData() failed\n");
            break;
        }

        UserData = (WCHAR*)RtlAllocateHeap(ProcessHeap,
                                           0,
                                           (wcslen(KeyName) + 1) * sizeof(WCHAR));
        if (UserData == NULL)
        {
            /* FIXME: Handle error! */
        }

        wcscpy(UserData, KeyName);

        sprintf(Buffer, "%S", KeyValue);
        AppendGenericListEntry(List, Buffer, UserData, FALSE);
    } while (SetupFindNextLine(&Context, &Context));

    return List;
}

ULONG
GetDefaultLanguageIndex(VOID)
{
    return DefaultLanguageIndex;
}

PGENERIC_LIST
CreateLanguageList(
    HINF InfFile,
    WCHAR *DefaultLanguage)
{
    CHAR Buffer[128];
    PGENERIC_LIST List;
    INFCONTEXT Context;
    PWCHAR KeyName;
    PWCHAR KeyValue;
    PWCHAR UserData = NULL;
    ULONG uIndex = 0;

    /* Get default language id */
    if (!SetupFindFirstLineW(InfFile, L"NLS", L"DefaultLanguage", &Context))
        return NULL;

    if (!INF_GetData(&Context, NULL, &KeyValue))
        return NULL;

    wcscpy(DefaultLanguage, KeyValue);

    SelectedLanguageId = KeyValue;

    List = CreateGenericList();
    if (List == NULL)
        return NULL;

    if (!SetupFindFirstLineW(InfFile, L"Language", NULL, &Context))
    {
        DestroyGenericList(List, FALSE);
        return NULL;
    }

    do
    {
        if (!INF_GetData(&Context, &KeyName, &KeyValue))
        {
            /* FIXME: Handle error! */
            DPRINT("INF_GetData() failed\n");
            break;
        }

        if (IsLanguageAvailable(KeyName))
        {

            UserData = (WCHAR*) RtlAllocateHeap(ProcessHeap,
                                                0,
                                                (wcslen(KeyName) + 1) * sizeof(WCHAR));
            if (UserData == NULL)
            {
                /* FIXME: Handle error! */
            }

            wcscpy(UserData, KeyName);

            if (!_wcsicmp(KeyName, DefaultLanguage))
                DefaultLanguageIndex = uIndex;

            sprintf(Buffer, "%S", KeyValue);
            AppendGenericListEntry(List,
                                   Buffer,
                                   UserData,
                                   FALSE);
            uIndex++;
        }
    } while (SetupFindNextLine(&Context, &Context));

    /* Only one language available, make it the default one */
    if (uIndex == 1 && UserData != NULL)
    {
        DefaultLanguageIndex = 0;
        wcscpy(DefaultLanguage, UserData);
    }

    return List;
}

PGENERIC_LIST
CreateKeyboardLayoutList(
    HINF InfFile,
    WCHAR *DefaultKBLayout)
{
    CHAR Buffer[128];
    PGENERIC_LIST List;
    INFCONTEXT Context;
    PWCHAR KeyName;
    PWCHAR KeyValue;
    PWCHAR UserData;
    const MUI_LAYOUTS * LayoutsList;
    ULONG uIndex = 0;
    BOOL KeyboardLayoutsFound = FALSE;

    /* Get default layout id */
    if (!SetupFindFirstLineW(InfFile, L"NLS", L"DefaultLayout", &Context))
        return NULL;

    if (!INF_GetData(&Context, NULL, &KeyValue))
        return NULL;

    wcscpy(DefaultKBLayout, KeyValue);

    List = CreateGenericList();
    if (List == NULL)
        return NULL;

    LayoutsList = MUIGetLayoutsList();

    do
    {
        if (!SetupFindFirstLineW(InfFile, L"KeyboardLayout", NULL, &Context))
        {
            DestroyGenericList(List, FALSE);
            return NULL;
        }

        do
        {
            if (!INF_GetData(&Context, &KeyName, &KeyValue))
            {
                /* FIXME: Handle error! */
                DPRINT("INF_GetData() failed\n");
                DestroyGenericList(List, FALSE);
                return NULL;
            }

            {
                UserData = (WCHAR*)RtlAllocateHeap(ProcessHeap,
                                                   0,
                                                   (wcslen(KeyName) + 1) * sizeof(WCHAR));
                if (UserData == NULL)
                {
                    /* FIXME: Handle error! */
                    DPRINT("RtlAllocateHeap() failed\n");
                    DestroyGenericList(List, FALSE);
                    return NULL;
                }

                wcscpy(UserData, KeyName);

                sprintf(Buffer, "%S", KeyValue);
                AppendGenericListEntry(List,
                                       Buffer,
                                       UserData,
                                       _wcsicmp(KeyName, DefaultKBLayout) ? FALSE : TRUE);
                KeyboardLayoutsFound = TRUE;
            }

        } while (SetupFindNextLine(&Context, &Context));

        uIndex++;

    } while (LayoutsList[uIndex].LangID != NULL);

    /* FIXME: Handle this case */
    if (!KeyboardLayoutsFound)
    {
        DPRINT1("No keyboard layouts have been found\n");
        DestroyGenericList(List, FALSE);
        return NULL;
    }

    return List;
}


BOOLEAN
ProcessKeyboardLayoutRegistry(
    PGENERIC_LIST List)
{
    PGENERIC_LIST_ENTRY Entry;
    PWCHAR LayoutId;
    const MUI_LAYOUTS * LayoutsList;
    MUI_LAYOUTS NewLayoutsList[20];
    ULONG uIndex;
    ULONG uOldPos = 0;

    Entry = GetCurrentListEntry(List);
    if (Entry == NULL)
        return FALSE;

    LayoutId = (PWCHAR)GetListEntryUserData(Entry);
    if (LayoutId == NULL)
        return FALSE;

    LayoutsList = MUIGetLayoutsList();

    if (_wcsicmp(LayoutsList[0].LayoutID, LayoutId) != 0)
    {
        for (uIndex = 1; LayoutsList[uIndex].LangID != NULL; uIndex++)
        {
            if (_wcsicmp(LayoutsList[uIndex].LayoutID, LayoutId) == 0)
            {
                uOldPos = uIndex;
                continue;
            }

            NewLayoutsList[uIndex].LangID   = LayoutsList[uIndex].LangID;
            NewLayoutsList[uIndex].LayoutID = LayoutsList[uIndex].LayoutID;
        }

        NewLayoutsList[uIndex].LangID    = NULL;
        NewLayoutsList[uIndex].LayoutID  = NULL;
        NewLayoutsList[uOldPos].LangID   = LayoutsList[0].LangID;
        NewLayoutsList[uOldPos].LayoutID = LayoutsList[0].LayoutID;
        NewLayoutsList[0].LangID         = LayoutsList[uOldPos].LangID;
        NewLayoutsList[0].LayoutID       = LayoutsList[uOldPos].LayoutID;

        return AddKbLayoutsToRegistry(NewLayoutsList);
    }

    return TRUE;
}

#if 0
BOOLEAN
ProcessKeyboardLayoutFiles(
    PGENERIC_LIST List)
{
    return TRUE;
}
#endif

BOOLEAN
SetGeoID(
    PWCHAR Id)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;
    HANDLE KeyHandle;

    RtlInitUnicodeString(&Name,
                         L".DEFAULT\\Control Panel\\International\\Geo");
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               GetRootKeyByPredefKey(HKEY_USERS, NULL),
                               NULL);
    Status =  NtOpenKey(&KeyHandle,
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
                           (PVOID)Id,
                           (wcslen(Id) + 1) * sizeof(WCHAR));
    NtClose(KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetValueKey() failed (Status = %lx)\n", Status);
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
SetDefaultPagefile(
    WCHAR Drive)
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
