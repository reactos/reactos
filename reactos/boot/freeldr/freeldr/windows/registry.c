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

static PCMHIVE CmHive;
static PCM_KEY_NODE RootKeyNode;
static FRLDRHKEY CurrentControlSetKey;

BOOLEAN
RegImportBinaryHive(
    _In_ PCHAR ChunkBase,
    _In_ ULONG ChunkSize)
{
    NTSTATUS Status;
    TRACE("RegImportBinaryHive(%p, 0x%lx)\n", ChunkBase, ChunkSize);

    /* Allocate and initialize the hive */
    CmHive = FrLdrTempAlloc(sizeof(CMHIVE), 'eviH');
    Status = HvInitialize(&CmHive->Hive,
                          HINIT_FLAT,
                          0,
                          0,
                          ChunkBase,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          1,
                          NULL);
    if (!NT_SUCCESS(Status))
    {
        FrLdrTempFree(CmHive, 'eviH');
        ERR("Invalid hive Signature!\n");
        return FALSE;
    }

    /* Save the root key node */
    RootKeyNode = HvGetCell(&CmHive->Hive, CmHive->Hive.BaseBlock->RootCell);

    TRACE("RegImportBinaryHive done\n");
    return TRUE;
}

VOID
RegInitializeRegistry(VOID)
{
    /* Nothing to do */
}

LONG
RegInitCurrentControlSet(
    _In_ BOOLEAN LastKnownGood)
{
    WCHAR ControlSetKeyName[80];
    FRLDRHKEY SelectKey;
    FRLDRHKEY SystemKey;
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
        ERR("RegOpenKey() failed (Error %u)\n", (int)Error);
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
        ERR("RegQueryValue('Default') failed (Error %u)\n", (int)Error);
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
        ERR("RegQueryValue('Default') failed (Error %u)\n", (int)Error);
        return Error;
    }

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
        ERR("RegOpenKey(SystemKey) failed (Error %lu)\n", Error);
        return Error;
    }

    Error = RegOpenKey(SystemKey,
                       ControlSetKeyName,
                       &CurrentControlSetKey);
    if (Error != ERROR_SUCCESS)
    {
        ERR("RegOpenKey(CurrentControlSetKey) failed (Error %lu)\n", Error);
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

    NextElement->Length = (RemainingPath->Buffer - NextElement->Buffer) * sizeof(WCHAR);
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

static
PCM_KEY_NODE
RegpFindSubkeyInIndex(
    _In_ PHHIVE Hive,
    _In_ PCM_KEY_INDEX IndexCell,
    _In_ PUNICODE_STRING SubKeyName)
{
    PCM_KEY_NODE SubKeyNode;
    ULONG i;
    TRACE("RegpFindSubkeyInIndex('%wZ')\n", SubKeyName);

    /* Check the cell type */
    if ((IndexCell->Signature == CM_KEY_INDEX_ROOT) ||
        (IndexCell->Signature == CM_KEY_INDEX_LEAF))
    {
        ASSERT(FALSE);

        /* Enumerate subindex cells */
        for (i = 0; i < IndexCell->Count; i++)
        {
            /* Get the subindex cell and call the function recursively */
            PCM_KEY_INDEX SubIndexCell = HvGetCell(Hive, IndexCell->List[i]);

            SubKeyNode = RegpFindSubkeyInIndex(Hive, SubIndexCell, SubKeyName);
            if (SubKeyNode != NULL)
            {
                return SubKeyNode;
            }
        }
    }
    else if ((IndexCell->Signature == CM_KEY_FAST_LEAF) ||
             (IndexCell->Signature == CM_KEY_HASH_LEAF))
    {
        /* Directly enumerate subkey nodes */
        PCM_KEY_FAST_INDEX HashCell = (PCM_KEY_FAST_INDEX)IndexCell;
        for (i = 0; i < HashCell->Count; i++)
        {
            SubKeyNode = HvGetCell(Hive, HashCell->List[i].Cell);
            ASSERT(SubKeyNode->Signature == CM_KEY_NODE_SIGNATURE);

            TRACE(" RegpFindSubkeyInIndex: checking '%.*s'\n",
                  SubKeyNode->NameLength, SubKeyNode->Name);
            if (CmCompareKeyName(SubKeyNode, SubKeyName, TRUE))
            {
                return SubKeyNode;
            }
        }
    }
    else
    {
        ASSERT(FALSE);
    }

    return NULL;
}

LONG
RegEnumKey(
    _In_ FRLDRHKEY Key,
    _In_ ULONG Index,
    _Out_ PWCHAR Name,
    _Inout_ ULONG* NameSize,
    _Out_opt_ FRLDRHKEY *SubKey)
{
    PHHIVE Hive = &CmHive->Hive;
    PCM_KEY_NODE KeyNode, SubKeyNode;
    PCM_KEY_INDEX IndexCell;
    PCM_KEY_FAST_INDEX HashCell;
    TRACE("RegEnumKey(%p, %lu, %p, %p->%u)\n",
          Key, Index, Name, NameSize, NameSize ? *NameSize : 0);

    /* Get the key node */
    KeyNode = (PCM_KEY_NODE)Key;
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    /* Check if the index is valid */
    if ((KeyNode->SubKeyCounts[Stable] == 0) ||
        (Index >= KeyNode->SubKeyCounts[Stable]))
    {
        TRACE("RegEnumKey index out of bounds\n");
        return ERROR_NO_MORE_ITEMS;
    }

    /* Get the index cell */
    IndexCell = HvGetCell(Hive, KeyNode->SubKeyLists[Stable]);
    TRACE("IndexCell: %x, SubKeyCounts: %x\n", IndexCell, KeyNode->SubKeyCounts[Stable]);

    /* Check the cell type */
    if ((IndexCell->Signature == CM_KEY_FAST_LEAF) ||
        (IndexCell->Signature == CM_KEY_HASH_LEAF))
    {
        /* Get the value cell */
        HashCell = (PCM_KEY_FAST_INDEX)IndexCell;
        SubKeyNode = HvGetCell(Hive, HashCell->List[Index].Cell);
    }
    else
    {
        ASSERT(FALSE);
    }

    *NameSize = CmCopyKeyName(SubKeyNode, Name, *NameSize);

    if (SubKey != NULL)
    {
        *SubKey = (FRLDRHKEY)SubKeyNode;
    }

    TRACE("RegEnumKey done -> %u, '%.*s'\n", *NameSize, *NameSize, Name);
    return STATUS_SUCCESS;
}

LONG
RegOpenKey(
    _In_ FRLDRHKEY ParentKey,
    _In_z_ PCWSTR KeyName,
    _Out_ PFRLDRHKEY Key)
{
    UNICODE_STRING RemainingPath, SubKeyName;
    UNICODE_STRING CurrentControlSet = RTL_CONSTANT_STRING(L"CurrentControlSet");
    PHHIVE Hive = &CmHive->Hive;
    PCM_KEY_NODE KeyNode;
    PCM_KEY_INDEX IndexCell;
    TRACE("RegOpenKey(%p, '%S', %p)\n", ParentKey, KeyName, Key);

    /* Initialize the remaining path name */
    RtlInitUnicodeString(&RemainingPath, KeyName);

    /* Get the parent key node */
    KeyNode = (PCM_KEY_NODE)ParentKey;

    /* Check if we have a parent key */
    if (KeyNode == NULL)
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
        KeyNode = RootKeyNode;
    }

    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    /* Check if this is the root key */
    if (KeyNode == RootKeyNode)
    {
        UNICODE_STRING TempPath = RemainingPath;

        /* Get the first path element */
        GetNextPathElement(&SubKeyName, &TempPath);

        /* Check if this is CurrentControlSet */
        if (RtlEqualUnicodeString(&SubKeyName, &CurrentControlSet, TRUE))
        {
            /* Use the CurrentControlSetKey and update the remaining path */
            KeyNode = (PCM_KEY_NODE)CurrentControlSetKey;
            RemainingPath = TempPath;
        }
    }

    TRACE("RegOpenKey: RemainingPath '%wZ'\n", &RemainingPath);

    /* Loop while there are path elements */
    while (GetNextPathElement(&SubKeyName, &RemainingPath))
    {
        TRACE("RegOpenKey: next element '%wZ'\n", &SubKeyName);

        /* Check if there is any subkey */
        if (KeyNode->SubKeyCounts[Stable] == 0)
        {
            return ERROR_PATH_NOT_FOUND;
        }

        /* Get the top level index cell */
        IndexCell = HvGetCell(Hive, KeyNode->SubKeyLists[Stable]);

        /* Get the next sub key */
        KeyNode = RegpFindSubkeyInIndex(Hive, IndexCell, &SubKeyName);
        if (KeyNode == NULL)
        {

            ERR("Did not find sub key '%wZ' (full %S)\n", &RemainingPath, KeyName);
            return ERROR_PATH_NOT_FOUND;
        }
    }

    TRACE("RegOpenKey done\n");
    *Key = (FRLDRHKEY)KeyNode;
    return ERROR_SUCCESS;
}

static
VOID
RepGetValueData(
    _In_ PHHIVE Hive,
    _In_ PCM_KEY_VALUE ValueCell,
    _Out_opt_ ULONG* Type,
    _Out_opt_ PUCHAR Data,
    _Inout_opt_ ULONG* DataSize)
{
    ULONG DataLength;

    /* Does the caller want the type? */
    if (Type != NULL)
    {
        *Type = ValueCell->Type;
    }

    /* Does the caller provide DataSize? */
    if (DataSize != NULL)
    {
        /* Get the data length */
        DataLength = ValueCell->DataLength & REG_DATA_SIZE_MASK;

        /* Does the caller want the data? */
        if ((Data != NULL) && (*DataSize != 0))
        {
            /* Check where the data is stored */
            if ((DataLength <= sizeof(HCELL_INDEX)) &&
                 (ValueCell->DataLength & REG_DATA_IN_OFFSET))
            {
                /* The data member contains the data */
                RtlCopyMemory(Data,
                              &ValueCell->Data,
                              min(*DataSize, DataLength));
            }
            else
            {
                /* The data member contains the data cell index */
                PVOID DataCell = HvGetCell(Hive, ValueCell->Data);
                RtlCopyMemory(Data,
                              DataCell,
                              min(*DataSize, ValueCell->DataLength));
            }

        }

        /* Return the actual data length */
        *DataSize = DataLength;
    }
}

LONG
RegQueryValue(
    _In_ FRLDRHKEY Key,
    _In_z_ PCWSTR ValueName,
    _Out_opt_ ULONG* Type,
    _Out_opt_ PUCHAR Data,
    _Inout_opt_ ULONG* DataSize)
{
    PHHIVE Hive = &CmHive->Hive;
    PCM_KEY_NODE KeyNode;
    PCM_KEY_VALUE ValueCell;
    PVALUE_LIST_CELL ValueListCell;
    UNICODE_STRING ValueNameString;
    ULONG i;
    TRACE("RegQueryValue(%p, '%S', %p, %p, %p)\n",
          Key, ValueName, Type, Data, DataSize);

    /* Get the key node */
    KeyNode = (PCM_KEY_NODE)Key;
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    /* Check if there are any values */
    if (KeyNode->ValueList.Count == 0)
    {
        TRACE("RegQueryValue no values in key (%.*s)\n",
              KeyNode->NameLength, KeyNode->Name);
        return ERROR_INVALID_PARAMETER;
    }

    /* Initialize value name string */
    RtlInitUnicodeString(&ValueNameString, ValueName);

    ValueListCell = (PVALUE_LIST_CELL)HvGetCell(Hive, KeyNode->ValueList.List);
    TRACE("ValueListCell: %x\n", ValueListCell);

    /* Loop all values */
    for (i = 0; i < KeyNode->ValueList.Count; i++)
    {
        /* Get the subkey node and check the name */
        ValueCell = HvGetCell(Hive, ValueListCell->ValueOffset[i]);

        /* Compare the value name */
        TRACE("checking %.*s\n", ValueCell->NameLength, ValueCell->Name);
        if (CmCompareKeyValueName(ValueCell, &ValueNameString, TRUE))
        {
            RepGetValueData(Hive, ValueCell, Type, Data, DataSize);
            TRACE("RegQueryValue success\n");
            return STATUS_SUCCESS;
        }
    }

    TRACE("RegQueryValue value not found\n");
    return ERROR_INVALID_PARAMETER;
}

LONG
RegEnumValue(
    _In_ FRLDRHKEY Key,
    _In_ ULONG Index,
    _Out_ PWCHAR ValueName,
    _Inout_ ULONG* NameSize,
    _Out_ ULONG* Type,
    _Out_ PUCHAR Data,
    _Inout_ ULONG* DataSize)
{
    PHHIVE Hive = &CmHive->Hive;
    PCM_KEY_NODE KeyNode;
    PCM_KEY_VALUE ValueCell;
    PVALUE_LIST_CELL ValueListCell;
    TRACE("RegEnumValue(%p, %lu, %S, %p, %p, %p, %p (%lu))\n",
          Key, Index, ValueName, NameSize, Type, Data, DataSize, *DataSize);

    /* Get the key node */
    KeyNode = (PCM_KEY_NODE)Key;
    ASSERT(KeyNode->Signature == CM_KEY_NODE_SIGNATURE);

    /* Check if the index is valid */
    if ((KeyNode->ValueList.Count == 0) ||
        (Index >= KeyNode->ValueList.Count))
    {
        ERR("RegEnumValue: index invalid\n");
        return ERROR_NO_MORE_ITEMS;
    }

    ValueListCell = (PVALUE_LIST_CELL)HvGetCell(Hive, KeyNode->ValueList.List);
    TRACE("ValueListCell: %x\n", ValueListCell);

    /* Get the value cell */
    ValueCell = HvGetCell(Hive, ValueListCell->ValueOffset[Index]);
    ASSERT(ValueCell != NULL);

    if (NameSize != NULL)
    {
        *NameSize = CmCopyKeyValueName(ValueCell, ValueName, *NameSize);
    }

    RepGetValueData(Hive, ValueCell, Type, Data, DataSize);

    if (DataSize != NULL)
    {
        if ((Data != NULL) && (*DataSize != 0))
        {
            RtlCopyMemory(Data,
                          &ValueCell->Data,
                          min(*DataSize, ValueCell->DataLength));
        }

        *DataSize = ValueCell->DataLength;
    }

    TRACE("RegEnumValue done\n");
    return STATUS_SUCCESS;
}

/* EOF */
