/*
 *  FreeLoader
 *
 *  Copyright (C) 2014  Timo Kreuzer <timo.kreuzer@reactos.org>
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

#include <freeldr.h>
#include <cmlib.h>
#include "registry.h"

#include <debug.h>
DBG_DEFAULT_CHANNEL(REGISTRY);

static PCMHIVE CmSystemHive;
static HCELL_INDEX SystemRootCell;

PHHIVE SystemHive = NULL;
HKEY CurrentControlSetKey = NULL;

#define GET_HHIVE(CmHive)               (&((CmHive)->Hive))
#define GET_HHIVE_FROM_HKEY(hKey)       GET_HHIVE(CmSystemHive)
#define GET_CM_KEY_NODE(hHive, hKey)    ((PCM_KEY_NODE)HvGetCell(hHive, (HCELL_INDEX)hKey))

PVOID
NTAPI
CmpAllocate(
    IN SIZE_T Size,
    IN BOOLEAN Paged,
    IN ULONG Tag)
{
    UNREFERENCED_PARAMETER(Paged);
    return FrLdrHeapAlloc(Size, Tag);
}

VOID
NTAPI
CmpFree(
    IN PVOID Ptr,
    IN ULONG Quota)
{
    UNREFERENCED_PARAMETER(Quota);
    FrLdrHeapFree(Ptr, 0);
}

BOOLEAN
RegImportBinaryHive(
    _In_ PVOID ChunkBase,
    _In_ ULONG ChunkSize)
{
    NTSTATUS Status;
    PCM_KEY_NODE KeyNode;

    TRACE("RegImportBinaryHive(%p, 0x%lx)\n", ChunkBase, ChunkSize);

    /* Allocate and initialize the hive */
    CmSystemHive = FrLdrTempAlloc(sizeof(CMHIVE), 'eviH');
    Status = HvInitialize(GET_HHIVE(CmSystemHive),
                          HINIT_FLAT, // HINIT_MEMORY_INPLACE
                          0,
                          0,
                          ChunkBase,
                          CmpAllocate,
                          CmpFree,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          1,
                          NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("Corrupted hive %p!\n", ChunkBase);
        FrLdrTempFree(CmSystemHive, 'eviH');
        return FALSE;
    }

    /* Save the root key node */
    SystemHive = GET_HHIVE(CmSystemHive);
    SystemRootCell = SystemHive->BaseBlock->RootCell;
    ASSERT(SystemRootCell != HCELL_NIL);

    /* Verify it is accessible */
    KeyNode = (PCM_KEY_NODE)HvGetCell(SystemHive, SystemRootCell);
    ASSERT(KeyNode);
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);
    HvReleaseCell(SystemHive, SystemRootCell);

    TRACE("RegImportBinaryHive done\n");
    return TRUE;
}

LONG
RegInitCurrentControlSet(
    _In_ BOOLEAN LastKnownGood)
{
    WCHAR ControlSetKeyName[80];
    HKEY SelectKey;
    HKEY SystemKey;
    ULONG CurrentSet = 0;
    ULONG DefaultSet = 0;
    ULONG LastKnownGoodSet = 0;
    ULONG DataSize;
    LONG Error;

    TRACE("RegInitCurrentControlSet\n");

    Error = RegOpenKey(NULL,
                       L"\\Registry\\Machine\\SYSTEM\\Select",
                       &SelectKey);
    if (Error != ERROR_SUCCESS)
    {
        ERR("RegOpenKey('SYSTEM\\Select') failed (Error %lu)\n", Error);
        return Error;
    }

    DataSize = sizeof(ULONG);
    Error = RegQueryValue(SelectKey,
                          L"Default",
                          NULL,
                          (PUCHAR)&DefaultSet,
                          &DataSize);
    if (Error != ERROR_SUCCESS)
    {
        ERR("RegQueryValue('Default') failed (Error %lu)\n", Error);
        RegCloseKey(SelectKey);
        return Error;
    }

    DataSize = sizeof(ULONG);
    Error = RegQueryValue(SelectKey,
                          L"LastKnownGood",
                          NULL,
                          (PUCHAR)&LastKnownGoodSet,
                          &DataSize);
    if (Error != ERROR_SUCCESS)
    {
        ERR("RegQueryValue('LastKnownGood') failed (Error %lu)\n", Error);
        RegCloseKey(SelectKey);
        return Error;
    }

    RegCloseKey(SelectKey);

    CurrentSet = (LastKnownGood) ? LastKnownGoodSet : DefaultSet;
    wcscpy(ControlSetKeyName, L"ControlSet");
    switch(CurrentSet)
    {
        case 1:
            wcscat(ControlSetKeyName, L"001");
            break;
        case 2:
            wcscat(ControlSetKeyName, L"002");
            break;
        case 3:
            wcscat(ControlSetKeyName, L"003");
            break;
        case 4:
            wcscat(ControlSetKeyName, L"004");
            break;
        case 5:
            wcscat(ControlSetKeyName, L"005");
            break;
    }

    Error = RegOpenKey(NULL,
                       L"\\Registry\\Machine\\SYSTEM",
                       &SystemKey);
    if (Error != ERROR_SUCCESS)
    {
        ERR("RegOpenKey('SYSTEM') failed (Error %lu)\n", Error);
        return Error;
    }

    Error = RegOpenKey(SystemKey,
                       ControlSetKeyName,
                       &CurrentControlSetKey);

    RegCloseKey(SystemKey);

    if (Error != ERROR_SUCCESS)
    {
        ERR("RegOpenKey('%S') failed (Error %lu)\n", ControlSetKeyName, Error);
        return Error;
    }

    TRACE("RegInitCurrentControlSet done\n");
    return ERROR_SUCCESS;
}

static
BOOLEAN
GetNextPathElement(
    _Out_ PUNICODE_STRING NextElement,
    _Inout_ PUNICODE_STRING RemainingPath)
{
    /* Check if there are any characters left */
    if (RemainingPath->Length < sizeof(WCHAR))
    {
        /* Nothing left, bail out early */
        return FALSE;
    }

    /* The next path elements starts with the remaining path */
    NextElement->Buffer = RemainingPath->Buffer;

    /* Loop until the path element ends */
    while ((RemainingPath->Length >= sizeof(WCHAR)) &&
           (RemainingPath->Buffer[0] != '\\'))
    {
        /* Skip this character */
        RemainingPath->Buffer++;
        RemainingPath->Length -= sizeof(WCHAR);
    }

    NextElement->Length = (USHORT)(RemainingPath->Buffer - NextElement->Buffer) * sizeof(WCHAR);
    NextElement->MaximumLength = NextElement->Length;

    /* Check if the path element ended with a path separator */
    if (RemainingPath->Length >= sizeof(WCHAR))
    {
        /* Skip the path separator */
        ASSERT(RemainingPath->Buffer[0] == '\\');
        RemainingPath->Buffer++;
        RemainingPath->Length -= sizeof(WCHAR);
    }

    /* Return whether we got any characters */
    return TRUE;
}

#if 0
LONG
RegEnumKey(
    _In_ HKEY Key,
    _In_ ULONG Index,
    _Out_ PWCHAR Name,
    _Inout_ PULONG NameSize,
    _Out_opt_ PHKEY SubKey)
{
    PHHIVE Hive = GET_HHIVE_FROM_HKEY(Key);
    PCM_KEY_NODE KeyNode, SubKeyNode;
    HCELL_INDEX CellIndex;
    USHORT NameLength;

    TRACE("RegEnumKey(%p, %lu, %p, %p->%u)\n",
          Key, Index, Name, NameSize, NameSize ? *NameSize : 0);

    /* Get the key node */
    KeyNode = GET_CM_KEY_NODE(Hive, Key);
    ASSERT(KeyNode);
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    CellIndex = CmpFindSubKeyByNumber(Hive, KeyNode, Index);
    if (CellIndex == HCELL_NIL)
    {
        TRACE("RegEnumKey index out of bounds (%d) in key (%.*s)\n",
              Index, KeyNode->NameLength, KeyNode->Name);
        HvReleaseCell(Hive, (HCELL_INDEX)Key);
        return ERROR_NO_MORE_ITEMS;
    }
    HvReleaseCell(Hive, (HCELL_INDEX)Key);

    /* Get the value cell */
    SubKeyNode = (PCM_KEY_NODE)HvGetCell(Hive, CellIndex);
    ASSERT(SubKeyNode != NULL);
    ASSERT(SubKeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    if (SubKeyNode->Flags & KEY_COMP_NAME)
    {
        NameLength = CmpCompressedNameSize(SubKeyNode->Name, SubKeyNode->NameLength);

        /* Compressed name */
        CmpCopyCompressedName(Name,
                              *NameSize,
                              SubKeyNode->Name,
                              SubKeyNode->NameLength);
    }
    else
    {
        NameLength = SubKeyNode->NameLength;

        /* Normal name */
        RtlCopyMemory(Name, SubKeyNode->Name,
                      min(*NameSize, SubKeyNode->NameLength));
    }

    if (*NameSize >= NameLength + sizeof(WCHAR))
    {
        Name[NameLength / sizeof(WCHAR)] = UNICODE_NULL;
    }

    *NameSize = NameLength + sizeof(WCHAR);

    HvReleaseCell(Hive, CellIndex);

    if (SubKey != NULL)
        *SubKey = (HKEY)CellIndex;

    TRACE("RegEnumKey done -> %u, '%.*S'\n", *NameSize, *NameSize, Name);
    return ERROR_SUCCESS;
}
#endif

LONG
RegOpenKey(
    _In_ HKEY ParentKey,
    _In_z_ PCWSTR KeyName,
    _Out_ PHKEY Key)
{
    UNICODE_STRING RemainingPath, SubKeyName;
    UNICODE_STRING CurrentControlSet = RTL_CONSTANT_STRING(L"CurrentControlSet");
    PHHIVE Hive = (ParentKey ? GET_HHIVE_FROM_HKEY(ParentKey) : GET_HHIVE(CmSystemHive));
    PCM_KEY_NODE KeyNode;
    HCELL_INDEX CellIndex;

    TRACE("RegOpenKey(%p, '%S', %p)\n", ParentKey, KeyName, Key);

    /* Initialize the remaining path name */
    RtlInitUnicodeString(&RemainingPath, KeyName);

    /* Check if we have a parent key */
    if (ParentKey == NULL)
    {
        UNICODE_STRING SubKeyName1, SubKeyName2, SubKeyName3;
        UNICODE_STRING RegistryPath = RTL_CONSTANT_STRING(L"Registry");
        UNICODE_STRING MachinePath = RTL_CONSTANT_STRING(L"MACHINE");
        UNICODE_STRING SystemPath = RTL_CONSTANT_STRING(L"SYSTEM");

        TRACE("RegOpenKey: absolute path\n");

        if ((RemainingPath.Length < sizeof(WCHAR)) ||
            RemainingPath.Buffer[0] != '\\')
        {
            /* The key path is not absolute */
            ERR("RegOpenKey: invalid path '%S' (%wZ)\n", KeyName, &RemainingPath);
            return ERROR_PATH_NOT_FOUND;
        }

        /* Skip initial path separator */
        RemainingPath.Buffer++;
        RemainingPath.Length -= sizeof(WCHAR);

        /* Get the first 3 path elements */
        GetNextPathElement(&SubKeyName1, &RemainingPath);
        GetNextPathElement(&SubKeyName2, &RemainingPath);
        GetNextPathElement(&SubKeyName3, &RemainingPath);
        TRACE("RegOpenKey: %wZ / %wZ / %wZ\n", &SubKeyName1, &SubKeyName2, &SubKeyName3);

        /* Check if we have the correct path */
        if (!RtlEqualUnicodeString(&SubKeyName1, &RegistryPath, TRUE) ||
            !RtlEqualUnicodeString(&SubKeyName2, &MachinePath, TRUE) ||
            !RtlEqualUnicodeString(&SubKeyName3, &SystemPath, TRUE))
        {
            /* The key path is not inside HKLM\Machine\System */
            ERR("RegOpenKey: invalid path '%S' (%wZ)\n", KeyName, &RemainingPath);
            return ERROR_PATH_NOT_FOUND;
        }

        /* Use the root key */
        CellIndex = SystemRootCell;
    }
    else
    {
        /* Use the parent key */
        CellIndex = (HCELL_INDEX)ParentKey;
    }

    /* Check if this is the root key */
    if (CellIndex == SystemRootCell)
    {
        UNICODE_STRING TempPath = RemainingPath;

        /* Get the first path element */
        GetNextPathElement(&SubKeyName, &TempPath);

        /* Check if this is CurrentControlSet */
        if (RtlEqualUnicodeString(&SubKeyName, &CurrentControlSet, TRUE))
        {
            /* Use the CurrentControlSetKey and update the remaining path */
            CellIndex = (HCELL_INDEX)CurrentControlSetKey;
            RemainingPath = TempPath;
        }
    }

    /* Get the key node */
    KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, CellIndex);
    ASSERT(KeyNode);
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    TRACE("RegOpenKey: RemainingPath '%wZ'\n", &RemainingPath);

    /* Loop while there are path elements */
    while (GetNextPathElement(&SubKeyName, &RemainingPath))
    {
        HCELL_INDEX NextCellIndex;

        TRACE("RegOpenKey: next element '%wZ'\n", &SubKeyName);

        /* Get the next sub key */
        NextCellIndex = CmpFindSubKeyByName(Hive, KeyNode, &SubKeyName);
        HvReleaseCell(Hive, CellIndex);
        CellIndex = NextCellIndex;
        if (CellIndex == HCELL_NIL)
        {
            WARN("Did not find sub key '%wZ' (full: %S)\n", &SubKeyName, KeyName);
            return ERROR_PATH_NOT_FOUND;
        }

        /* Get the found key */
        KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, CellIndex);
        ASSERT(KeyNode);
        ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);
    }

    HvReleaseCell(Hive, CellIndex);
    *Key = (HKEY)CellIndex;

    TRACE("RegOpenKey done\n");
    return ERROR_SUCCESS;
}

static
VOID
RepGetValueData(
    _In_ PHHIVE Hive,
    _In_ PCM_KEY_VALUE ValueCell,
    _Out_opt_ PULONG Type,
    _Out_opt_ PUCHAR Data,
    _Inout_opt_ PULONG DataSize)
{
    ULONG DataLength;
    PVOID DataCell;

    /* Does the caller want the type? */
    if (Type != NULL)
        *Type = ValueCell->Type;

    /* Does the caller provide DataSize? */
    if (DataSize != NULL)
    {
        // NOTE: CmpValueToData doesn't support big data (the function will
        // bugcheck if so), FreeLdr is not supposed to read such data.
        // If big data is needed, use instead CmpGetValueData.
        // CmpGetValueData(Hive, ValueCell, DataSize, &DataCell, ...);
        DataCell = CmpValueToData(Hive, ValueCell, &DataLength);

        /* Does the caller want the data? */
        if ((Data != NULL) && (*DataSize != 0))
        {
            RtlCopyMemory(Data,
                          DataCell,
                          min(*DataSize, DataLength));
        }

        /* Return the actual data length */
        *DataSize = DataLength;
    }
}

LONG
RegQueryValue(
    _In_ HKEY Key,
    _In_z_ PCWSTR ValueName,
    _Out_opt_ PULONG Type,
    _Out_opt_ PUCHAR Data,
    _Inout_opt_ PULONG DataSize)
{
    PHHIVE Hive = GET_HHIVE_FROM_HKEY(Key);
    PCM_KEY_NODE KeyNode;
    PCM_KEY_VALUE ValueCell;
    HCELL_INDEX CellIndex;
    UNICODE_STRING ValueNameString;

    TRACE("RegQueryValue(%p, '%S', %p, %p, %p)\n",
          Key, ValueName, Type, Data, DataSize);

    /* Get the key node */
    KeyNode = GET_CM_KEY_NODE(Hive, Key);
    ASSERT(KeyNode);
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    /* Initialize value name string */
    RtlInitUnicodeString(&ValueNameString, ValueName);
    CellIndex = CmpFindValueByName(Hive, KeyNode, &ValueNameString);
    if (CellIndex == HCELL_NIL)
    {
        TRACE("RegQueryValue value not found in key (%.*s)\n",
              KeyNode->NameLength, KeyNode->Name);
        HvReleaseCell(Hive, (HCELL_INDEX)Key);
        return ERROR_FILE_NOT_FOUND;
    }
    HvReleaseCell(Hive, (HCELL_INDEX)Key);

    /* Get the value cell */
    ValueCell = (PCM_KEY_VALUE)HvGetCell(Hive, CellIndex);
    ASSERT(ValueCell != NULL);

    RepGetValueData(Hive, ValueCell, Type, Data, DataSize);

    HvReleaseCell(Hive, CellIndex);

    TRACE("RegQueryValue success\n");
    return ERROR_SUCCESS;
}

/*
 * NOTE: This function is currently unused in FreeLdr; however it is kept here
 * as an implementation reference of RegEnumValue using CMLIB that may be used
 * elsewhere in ReactOS.
 */
#if 0
LONG
RegEnumValue(
    _In_ HKEY Key,
    _In_ ULONG Index,
    _Out_ PWCHAR ValueName,
    _Inout_ PULONG NameSize,
    _Out_opt_ PULONG Type,
    _Out_opt_ PUCHAR Data,
    _Inout_opt_ PULONG DataSize)
{
    PHHIVE Hive = GET_HHIVE_FROM_HKEY(Key);
    PCM_KEY_NODE KeyNode;
    PCELL_DATA ValueListCell;
    PCM_KEY_VALUE ValueCell;
    USHORT NameLength;

    TRACE("RegEnumValue(%p, %lu, %S, %p, %p, %p, %p (%lu))\n",
          Key, Index, ValueName, NameSize, Type, Data, DataSize, *DataSize);

    /* Get the key node */
    KeyNode = GET_CM_KEY_NODE(Hive, Key);
    ASSERT(KeyNode);
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    /* Check if the index is valid */
    if ((KeyNode->ValueList.Count == 0) ||
        (KeyNode->ValueList.List == HCELL_NIL) ||
        (Index >= KeyNode->ValueList.Count))
    {
        ERR("RegEnumValue: index invalid\n");
        HvReleaseCell(Hive, (HCELL_INDEX)Key);
        return ERROR_NO_MORE_ITEMS;
    }

    ValueListCell = (PCELL_DATA)HvGetCell(Hive, KeyNode->ValueList.List);
    ASSERT(ValueListCell != NULL);

    /* Get the value cell */
    ValueCell = (PCM_KEY_VALUE)HvGetCell(Hive, ValueListCell->KeyList[Index]);
    ASSERT(ValueCell != NULL);
    ASSERT(ValueCell->Signature == CM_KEY_VALUE_SIGNATURE);

    if (ValueCell->Flags & VALUE_COMP_NAME)
    {
        NameLength = CmpCompressedNameSize(ValueCell->Name, ValueCell->NameLength);

        /* Compressed name */
        CmpCopyCompressedName(ValueName,
                              *NameSize,
                              ValueCell->Name,
                              ValueCell->NameLength);
    }
    else
    {
        NameLength = ValueCell->NameLength;

        /* Normal name */
        RtlCopyMemory(ValueName, ValueCell->Name,
                      min(*NameSize, ValueCell->NameLength));
    }

    if (*NameSize >= NameLength + sizeof(WCHAR))
    {
        ValueName[NameLength / sizeof(WCHAR)] = UNICODE_NULL;
    }

    *NameSize = NameLength + sizeof(WCHAR);

    RepGetValueData(Hive, ValueCell, Type, Data, DataSize);

    HvReleaseCell(Hive, ValueListCell->KeyList[Index]);
    HvReleaseCell(Hive, KeyNode->ValueList.List);
    HvReleaseCell(Hive, (HCELL_INDEX)Key);

    TRACE("RegEnumValue done -> %u, '%.*S'\n", *NameSize, *NameSize, ValueName);
    return ERROR_SUCCESS;
}
#endif

/* EOF */
