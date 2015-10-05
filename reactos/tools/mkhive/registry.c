/*
 *  ReactOS kernel
 *  Copyright (C) 2006 ReactOS Team
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
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/registry.c
 * PURPOSE:         Registry code
 * PROGRAMMER:      Hervé Poussineau
 */

/*
 * TODO:
 *   - Implement RegDeleteKeyW()
 *   - Implement RegEnumValue()
 *   - Implement RegQueryValueExW()
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define NDEBUG
#include "mkhive.h"

#define REG_DATA_SIZE_MASK                 0x7FFFFFFF
#define REG_DATA_IN_OFFSET                 0x80000000

static CMHIVE RootHive;
static PMEMKEY RootKey;
CMHIVE DefaultHive;  /* \Registry\User\.DEFAULT */
CMHIVE SamHive;      /* \Registry\Machine\SAM */
CMHIVE SecurityHive; /* \Registry\Machine\SECURITY */
CMHIVE SoftwareHive; /* \Registry\Machine\SOFTWARE */
CMHIVE SystemHive;   /* \Registry\Machine\SYSTEM */
CMHIVE BcdHive;      /* \Registry\Machine\BCD00000000 */

static PMEMKEY
CreateInMemoryStructure(
    IN PCMHIVE RegistryHive,
    IN HCELL_INDEX KeyCellOffset)
{
    PMEMKEY Key;

    Key = (PMEMKEY) malloc (sizeof(MEMKEY));
    if (!Key)
        return NULL;

    Key->RegistryHive = RegistryHive;
    Key->KeyCellOffset = KeyCellOffset;
    return Key;
}

LIST_ENTRY CmiReparsePointsHead;

static LONG
RegpOpenOrCreateKey(
    IN HKEY hParentKey,
    IN PCWSTR KeyName,
    IN BOOL AllowCreation,
    IN BOOL Volatile,
    OUT PHKEY Key)
{
    PWSTR LocalKeyName;
    PWSTR End;
    UNICODE_STRING KeyString;
    NTSTATUS Status;
    PREPARSE_POINT CurrentReparsePoint;
    PMEMKEY CurrentKey;
    PCMHIVE ParentRegistryHive;
    HCELL_INDEX ParentCellOffset;
    PLIST_ENTRY Ptr;
    PCM_KEY_NODE SubKeyCell;
    HCELL_INDEX BlockOffset;

    DPRINT("RegpCreateOpenKey('%S')\n", KeyName);

    if (*KeyName == L'\\')
    {
        KeyName++;
        ParentRegistryHive = RootKey->RegistryHive;
        ParentCellOffset = RootKey->KeyCellOffset;
    }
    else if (hParentKey == NULL)
    {
        ParentRegistryHive = RootKey->RegistryHive;
        ParentCellOffset = RootKey->KeyCellOffset;
    }
    else
    {
        ParentRegistryHive = HKEY_TO_MEMKEY(RootKey)->RegistryHive;
        ParentCellOffset = HKEY_TO_MEMKEY(RootKey)->KeyCellOffset;
    }

    LocalKeyName = (PWSTR)KeyName;
    for (;;)
    {
        End = (PWSTR)strchrW(LocalKeyName, '\\');
        if (End)
        {
            KeyString.Buffer = LocalKeyName;
            KeyString.Length = KeyString.MaximumLength =
                (USHORT)((ULONG_PTR)End - (ULONG_PTR)LocalKeyName);
        }
        else
        {
            RtlInitUnicodeString(&KeyString, LocalKeyName);
            if (KeyString.Length == 0)
            {
                /* Trailing backslash char; we're done */
                break;
            }
        }

        Status = CmiScanForSubKey(ParentRegistryHive,
                                  ParentCellOffset,
                                  &KeyString,
                                  OBJ_CASE_INSENSITIVE,
                                  &SubKeyCell,
                                  &BlockOffset);
        if (NT_SUCCESS(Status))
        {
            /* Search for a possible reparse point */
            Ptr = CmiReparsePointsHead.Flink;
            while (Ptr != &CmiReparsePointsHead)
            {
                CurrentReparsePoint = CONTAINING_RECORD(Ptr, REPARSE_POINT, ListEntry);
                if (CurrentReparsePoint->SourceHive == ParentRegistryHive &&
                    CurrentReparsePoint->SourceKeyCellOffset == BlockOffset)
                {
                    ParentRegistryHive = CurrentReparsePoint->DestinationHive;
                    BlockOffset = CurrentReparsePoint->DestinationKeyCellOffset;
                    break;
                }
                Ptr = Ptr->Flink;
            }
        }
        else if (Status == STATUS_OBJECT_NAME_NOT_FOUND && AllowCreation)
        {
            Status = CmiAddSubKey(ParentRegistryHive,
                                  ParentCellOffset,
                                  &KeyString,
                                  Volatile ? REG_OPTION_VOLATILE : 0,
                                  &SubKeyCell,
                                  &BlockOffset);
        }
        if (!NT_SUCCESS(Status))
            return ERROR_UNSUCCESSFUL;

nextsubkey:
        ParentCellOffset = BlockOffset;
        if (End)
            LocalKeyName = End + 1;
        else
            break;
    }

    CurrentKey = CreateInMemoryStructure(ParentRegistryHive, ParentCellOffset);
    if (!CurrentKey)
        return ERROR_OUTOFMEMORY;
    *Key = MEMKEY_TO_HKEY(CurrentKey);

    return ERROR_SUCCESS;
}

LONG WINAPI
RegCreateKeyW(
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    OUT PHKEY phkResult)
{
    return RegpOpenOrCreateKey(hKey, lpSubKey, TRUE, FALSE, phkResult);
}

static PWSTR
MultiByteToWideChar(
    IN PCSTR MultiByteString)
{
    ANSI_STRING Source;
    UNICODE_STRING Destination;
    NTSTATUS Status;

    RtlInitAnsiString(&Source, MultiByteString);
    Status = RtlAnsiStringToUnicodeString(&Destination, &Source, TRUE);
    if (!NT_SUCCESS(Status))
        return NULL;
    return Destination.Buffer;
}

LONG WINAPI
RegDeleteKeyW(
    IN HKEY hKey,
    IN LPCWSTR lpSubKey)
{
    DPRINT1("FIXME: implement RegDeleteKeyW!\n");
    return ERROR_SUCCESS;
}

LONG WINAPI
RegDeleteKeyA(
    IN HKEY hKey,
    IN LPCSTR lpSubKey)
{
    PWSTR lpSubKeyW = NULL;
    LONG rc;

    if (lpSubKey != NULL && strchr(lpSubKey, '\\') != NULL)
        return ERROR_INVALID_PARAMETER;

    if (lpSubKey)
    {
        lpSubKeyW = MultiByteToWideChar(lpSubKey);
        if (!lpSubKeyW)
            return ERROR_OUTOFMEMORY;
    }

    rc = RegDeleteKeyW(hKey, lpSubKeyW);

    if (lpSubKey)
        free(lpSubKeyW);

    return rc;
}

LONG WINAPI
RegOpenKeyW(
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    OUT PHKEY phkResult)
{
    return RegpOpenOrCreateKey(hKey, lpSubKey, FALSE, FALSE, phkResult);
}

LONG WINAPI
RegCreateKeyExW(
    IN HKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD Reserved,
    IN LPWSTR lpClass OPTIONAL,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes OPTIONAL,
    OUT PHKEY phkResult,
    OUT LPDWORD lpdwDisposition OPTIONAL)
{
    return RegpOpenOrCreateKey(hKey,
                               lpSubKey,
                               TRUE,
                               (dwOptions & REG_OPTION_VOLATILE) != 0,
                               phkResult);
}

LONG WINAPI
RegOpenKeyA(
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    OUT PHKEY phkResult)
{
    PWSTR lpSubKeyW;
    LONG rc;

    lpSubKeyW = MultiByteToWideChar(lpSubKey);
    if (!lpSubKeyW)
        return ERROR_OUTOFMEMORY;

    rc = RegOpenKeyW(hKey, lpSubKeyW, phkResult);
    free(lpSubKeyW);
    return rc;
}

static LONG
RegpOpenOrCreateValue(
    IN HKEY hKey,
    IN LPCWSTR ValueName,
    IN BOOL AllowCreation,
    OUT PCM_KEY_VALUE *ValueCell,
    OUT PHCELL_INDEX ValueCellOffset)
{
    PMEMKEY ParentKey;
    UNICODE_STRING ValueString;
    NTSTATUS Status;

    ParentKey = HKEY_TO_MEMKEY(hKey);
    RtlInitUnicodeString(&ValueString, ValueName);

    Status = CmiScanForValueKey(ParentKey->RegistryHive,
                                ParentKey->KeyCellOffset,
                                &ValueString,
                                ValueCell,
                                ValueCellOffset);
    if (AllowCreation && Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        Status = CmiAddValueKey(ParentKey->RegistryHive,
                                ParentKey->KeyCellOffset,
                                &ValueString,
                                ValueCell,
                                ValueCellOffset);
    }
    if (!NT_SUCCESS(Status))
        return ERROR_UNSUCCESSFUL;
    return ERROR_SUCCESS;
}

LONG WINAPI
RegSetValueExW(
    IN HKEY hKey,
    IN LPCWSTR lpValueName OPTIONAL,
    IN ULONG Reserved,
    IN ULONG dwType,
    IN const UCHAR* lpData,
    IN USHORT cbData)
{
    PMEMKEY Key, DestKey;
    PHKEY phKey;
    PCM_KEY_VALUE ValueCell;
    HCELL_INDEX ValueCellOffset;
    PVOID DataCell;
    LONG DataCellSize;
    NTSTATUS Status;

    if (dwType == REG_LINK)
    {
        /* Special handling of registry links */
        if (cbData != sizeof(PVOID))
            return STATUS_INVALID_PARAMETER;

        phKey = (PHKEY)lpData;
        Key = HKEY_TO_MEMKEY(hKey);
        DestKey = HKEY_TO_MEMKEY(*phKey);

        /* Create the link in registry hive (if applicable) */
        if (Key->RegistryHive != DestKey->RegistryHive)
            return STATUS_SUCCESS;
        DPRINT1("Save link to registry\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    if ((cbData & REG_DATA_SIZE_MASK) != cbData)
        return STATUS_UNSUCCESSFUL;

    Key = HKEY_TO_MEMKEY(hKey);

    Status = RegpOpenOrCreateValue(hKey, lpValueName, TRUE, &ValueCell, &ValueCellOffset);
    if (!NT_SUCCESS(Status))
        return ERROR_UNSUCCESSFUL;

    /* Get size of the allocated cellule (if any) */
    if (!(ValueCell->DataLength & REG_DATA_IN_OFFSET) &&
        (ValueCell->DataLength & REG_DATA_SIZE_MASK) != 0)
    {
        DataCell = HvGetCell(&Key->RegistryHive->Hive, ValueCell->Data);
        if (!DataCell)
            return ERROR_UNSUCCESSFUL;

        DataCellSize = -HvGetCellSize(&Key->RegistryHive->Hive, DataCell);
    }
    else
    {
        DataCell = NULL;
        DataCellSize = 0;
    }

    if (cbData <= sizeof(HCELL_INDEX))
    {
        /* If data size <= sizeof(HCELL_INDEX) then store data in the data offset */
        DPRINT("ValueCell->DataLength %u\n", ValueCell->DataLength);
        if (DataCell)
            HvFreeCell(&Key->RegistryHive->Hive, ValueCell->Data);

        RtlCopyMemory(&ValueCell->Data, lpData, cbData);
        ValueCell->DataLength = (ULONG)(cbData | REG_DATA_IN_OFFSET);
        ValueCell->Type = dwType;
        HvMarkCellDirty(&Key->RegistryHive->Hive, ValueCellOffset, FALSE);
    }
    else
    {
        if (cbData > (SIZE_T)DataCellSize)
        {
            /* New data size is larger than the current, destroy current
             * data block and allocate a new one. */
            HCELL_INDEX NewOffset;

            DPRINT("ValueCell->DataLength %u\n", ValueCell->DataLength);

            NewOffset = HvAllocateCell(&Key->RegistryHive->Hive, cbData, Stable, HCELL_NIL);
            if (NewOffset == HCELL_NIL)
            {
                DPRINT("HvAllocateCell() failed with status 0x%08x\n", Status);
                return ERROR_UNSUCCESSFUL;
            }

            if (DataCell)
                HvFreeCell(&Key->RegistryHive->Hive, ValueCell->Data);

            ValueCell->Data = NewOffset;
            DataCell = (PVOID)HvGetCell(&Key->RegistryHive->Hive, NewOffset);
        }

        /* Copy new contents to cellule */
        RtlCopyMemory(DataCell, lpData, cbData);
        ValueCell->DataLength = (ULONG)(cbData & REG_DATA_SIZE_MASK);
        ValueCell->Type = dwType;
        HvMarkCellDirty(&Key->RegistryHive->Hive, ValueCell->Data, FALSE);
        HvMarkCellDirty(&Key->RegistryHive->Hive, ValueCellOffset, FALSE);
    }

    HvMarkCellDirty(&Key->RegistryHive->Hive, Key->KeyCellOffset, FALSE);

    DPRINT("Return status 0x%08x\n", Status);
    return Status;
}

LONG WINAPI
RegQueryValueExW(
    IN HKEY hKey,
    IN LPCWSTR lpValueName,
    IN PULONG lpReserved,
    OUT PULONG lpType,
    OUT PUCHAR lpData,
    OUT PSIZE_T lpcbData)
{
    //ParentKey = HKEY_TO_MEMKEY(RootKey);
    PCM_KEY_VALUE ValueCell;
    HCELL_INDEX ValueCellOffset;
    LONG rc;

    rc = RegpOpenOrCreateValue(hKey,
                               lpValueName,
                               FALSE,
                               &ValueCell,
                               &ValueCellOffset);
    if (rc != ERROR_SUCCESS)
        return rc;

    DPRINT1("RegQueryValueExW(%S) not implemented\n", lpValueName);
    /* ValueCell and ValueCellOffset are valid */

    return ERROR_UNSUCCESSFUL;
}

LONG WINAPI
RegDeleteValueW(
    IN HKEY hKey,
    IN LPCWSTR lpValueName OPTIONAL)
{
    DPRINT1("RegDeleteValueW() unimplemented\n");
    return ERROR_UNSUCCESSFUL;
}

static BOOL
ConnectRegistry(
    IN HKEY RootKey,
    IN PCMHIVE HiveToConnect,
    IN LPCWSTR Path)
{
    NTSTATUS Status;
    PREPARSE_POINT ReparsePoint;
    PMEMKEY NewKey;
    LONG rc;

    ReparsePoint = (PREPARSE_POINT)malloc(sizeof(REPARSE_POINT));
    if (!ReparsePoint)
        return FALSE;

    Status = CmiInitializeTempHive(HiveToConnect);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmiInitializeTempHive() failed with status 0x%08x\n", Status);
        free(ReparsePoint);
        return FALSE;
    }

    /* Create key */
    rc = RegCreateKeyExW(RootKey,
                         Path,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         0,
                         NULL,
                         (PHKEY)&NewKey,
                         NULL);
    if (rc != ERROR_SUCCESS)
    {
        free(ReparsePoint);
        return FALSE;
    }

    ReparsePoint->SourceHive = NewKey->RegistryHive;
    ReparsePoint->SourceKeyCellOffset = NewKey->KeyCellOffset;
    NewKey->RegistryHive = HiveToConnect;
    NewKey->KeyCellOffset = HiveToConnect->Hive.BaseBlock->RootCell;
    ReparsePoint->DestinationHive = NewKey->RegistryHive;
    ReparsePoint->DestinationKeyCellOffset = NewKey->KeyCellOffset;
    InsertTailList(&CmiReparsePointsHead, &ReparsePoint->ListEntry);
    return TRUE;
}

LIST_ENTRY CmiHiveListHead;

VOID
RegInitializeRegistry(VOID)
{
    UNICODE_STRING RootKeyName = RTL_CONSTANT_STRING(L"\\");
    NTSTATUS Status;
    PMEMKEY ControlSetKey, CurrentControlSetKey;
    PREPARSE_POINT ReparsePoint;

    InitializeListHead(&CmiHiveListHead);
    InitializeListHead(&CmiReparsePointsHead);

    Status = CmiInitializeTempHive(&RootHive);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmiInitializeTempHive() failed with status 0x%08x\n", Status);
        return;
    }

    RootKey = CreateInMemoryStructure(&RootHive,
                                      RootHive.Hive.BaseBlock->RootCell);

    /* Create DEFAULT key */
    ConnectRegistry(NULL,
                    &DefaultHive,
                    L"Registry\\User\\.DEFAULT");

    /* Create SAM key */
    ConnectRegistry(NULL,
                    &SamHive,
                    L"Registry\\Machine\\SAM");

    /* Create SECURITY key */
    ConnectRegistry(NULL,
                    &SecurityHive,
                    L"Registry\\Machine\\SECURITY");

    /* Create SOFTWARE key */
    ConnectRegistry(NULL,
                    &SoftwareHive,
                    L"Registry\\Machine\\SOFTWARE");

    /* Create BCD key */
    ConnectRegistry(NULL,
                    &BcdHive,
                    L"Registry\\Machine\\BCD00000000");

    /* Create SYSTEM key */
    ConnectRegistry(NULL,
                    &SystemHive,
                    L"Registry\\Machine\\SYSTEM");

    /* Create 'ControlSet001' key */
    RegCreateKeyW(NULL,
                  L"Registry\\Machine\\SYSTEM\\ControlSet001",
                  (HKEY*)&ControlSetKey);

    /* Create 'CurrentControlSet' key */
    RegCreateKeyExW(NULL,
                    L"Registry\\Machine\\SYSTEM\\CurrentControlSet",
                    0,
                    NULL,
                    REG_OPTION_VOLATILE,
                    0,
                    NULL,
                    (HKEY*)&CurrentControlSetKey,
                    NULL);

    /* Connect 'CurrentControlSet' to 'ControlSet001' */
    ReparsePoint = (PREPARSE_POINT)malloc(sizeof(REPARSE_POINT));
    ReparsePoint->SourceHive = CurrentControlSetKey->RegistryHive;
    ReparsePoint->SourceKeyCellOffset = CurrentControlSetKey->KeyCellOffset;
    ReparsePoint->DestinationHive = ControlSetKey->RegistryHive;
    ReparsePoint->DestinationKeyCellOffset = ControlSetKey->KeyCellOffset;
    InsertTailList(&CmiReparsePointsHead, &ReparsePoint->ListEntry);
}

VOID
RegShutdownRegistry(VOID)
{
    /* FIXME: clean up the complete hive */

    free(RootKey);
}

/* EOF */
