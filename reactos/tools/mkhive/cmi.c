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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/cmi.c
 * PURPOSE:         Registry file manipulation routines
 * PROGRAMMER:      Hervé Poussineau
 */

#define NDEBUG
#include "mkhive.h"

static PVOID
NTAPI
CmpAllocate(
	IN SIZE_T Size,
	IN BOOLEAN Paged)
{
	return (PVOID) malloc((size_t)Size);
}

static VOID
NTAPI
CmpFree(
	IN PVOID Ptr)
{
	free(Ptr);
}

static BOOLEAN
NTAPI
CmpFileRead(
	IN PHHIVE RegistryHive,
	IN ULONG FileType,
	IN ULONGLONG FileOffset,
	OUT PVOID Buffer,
	IN SIZE_T BufferLength)
{
	DPRINT1("CmpFileRead() unimplemented\n");
	return FALSE;
}

static BOOLEAN
NTAPI
CmpFileWrite(
	IN PHHIVE RegistryHive,
	IN ULONG FileType,
	IN ULONGLONG FileOffset,
	IN PVOID Buffer,
	IN SIZE_T BufferLength)
{
	PEREGISTRY_HIVE CmHive = (PEREGISTRY_HIVE)RegistryHive;
	FILE *File = CmHive->HiveHandle;
	if (0 != fseek (File, (long)FileOffset, SEEK_SET))
		return FALSE;
	return BufferLength == fwrite (Buffer, 1, BufferLength, File);
}

static BOOLEAN
NTAPI
CmpFileSetSize(
	IN PHHIVE RegistryHive,
	IN ULONG FileType,
	IN ULONGLONG FileSize)
{
	DPRINT1("CmpFileSetSize() unimplemented\n");
	return FALSE;
}

static BOOLEAN
NTAPI
CmpFileFlush(
	IN PHHIVE RegistryHive,
	IN ULONG FileType)
{
	PEREGISTRY_HIVE CmHive = (PEREGISTRY_HIVE)RegistryHive;
	FILE *File = CmHive->HiveHandle;
	return 0 == fflush (File);
}

NTSTATUS
CmiInitializeTempHive(
	IN OUT PEREGISTRY_HIVE Hive)
{
	NTSTATUS Status;

	RtlZeroMemory (
		Hive,
		sizeof(EREGISTRY_HIVE));

	DPRINT("Hive 0x%p\n", Hive);

	Status = HvInitialize(
		&Hive->Hive,
		HV_OPERATION_CREATE_HIVE, 0, 0, 0, 0,
		CmpAllocate, CmpFree,
		CmpFileRead, CmpFileWrite, CmpFileSetSize,
		CmpFileFlush, NULL);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	if (!CmCreateRootNode (&Hive->Hive, L""))
	{
		HvFree (&Hive->Hive);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	Hive->Flags = HIVE_NO_FILE;

	/* Add the new hive to the hive list */
	InsertTailList (
		&CmiHiveListHead,
		&Hive->HiveList);

	VERIFY_REGISTRY_HIVE (Hive);

	return STATUS_SUCCESS;
}

static NTSTATUS
CmiAddKeyToHashTable(
	IN PEREGISTRY_HIVE RegistryHive,
	IN OUT PHASH_TABLE_CELL HashCell,
	IN PCM_KEY_NODE KeyCell,
	IN HV_STORAGE_TYPE StorageType,
	IN PCM_KEY_NODE NewKeyCell,
	IN HCELL_INDEX NKBOffset)
{
	ULONG i = KeyCell->SubKeyCounts[StorageType];
	ULONG HashValue;

	if (NewKeyCell->Flags & REG_KEY_NAME_PACKED)
	{
		RtlCopyMemory(
			&HashValue,
			NewKeyCell->Name,
			min(NewKeyCell->NameSize, sizeof(ULONG)));
	}

	for (i = 0; i < KeyCell->SubKeyCounts[StorageType]; i++)
	{
		if (HashCell->Table[i].HashValue > HashValue)
			break;
	}

	if (i < KeyCell->SubKeyCounts[StorageType])
	{
		RtlMoveMemory(HashCell->Table + i + 1,
		              HashCell->Table + i,
		              (HashCell->HashTableSize - 1 - i) *
		              sizeof(HashCell->Table[0]));
	}

	HashCell->Table[i].KeyOffset = NKBOffset;
	HashCell->Table[i].HashValue = HashValue;
	HvMarkCellDirty(&RegistryHive->Hive, KeyCell->SubKeyLists[StorageType]);
	return STATUS_SUCCESS;
}

static NTSTATUS
CmiAllocateHashTableCell (
	IN PEREGISTRY_HIVE RegistryHive,
	OUT PHASH_TABLE_CELL *HashBlock,
	OUT HCELL_INDEX *HBOffset,
	IN USHORT SubKeyCount,
	IN HV_STORAGE_TYPE Storage)
{
	PHASH_TABLE_CELL NewHashBlock;
	ULONG NewHashSize;
	NTSTATUS Status;

	Status = STATUS_SUCCESS;
	*HashBlock = NULL;
	NewHashSize = sizeof(HASH_TABLE_CELL) +
		(SubKeyCount * sizeof(HASH_RECORD));
	*HBOffset = HvAllocateCell(&RegistryHive->Hive, NewHashSize, Storage);

	if (*HBOffset == HCELL_NULL)
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;
	}
	else
	{
		ASSERT(SubKeyCount <= USHORT_MAX);
		NewHashBlock = HvGetCell (&RegistryHive->Hive, *HBOffset);
		NewHashBlock->Id = REG_HASH_TABLE_CELL_ID;
		NewHashBlock->HashTableSize = SubKeyCount;
		*HashBlock = NewHashBlock;
	}

	return Status;
}

NTSTATUS
CmiAddSubKey(
	IN PEREGISTRY_HIVE RegistryHive,
	IN PCM_KEY_NODE ParentKeyCell,
	IN HCELL_INDEX ParentKeyCellOffset,
	IN PCUNICODE_STRING SubKeyName,
	IN ULONG CreateOptions,
	OUT PCM_KEY_NODE *pSubKeyCell,
	OUT HCELL_INDEX *pBlockOffset)
{
	PHASH_TABLE_CELL HashBlock;
	HCELL_INDEX NKBOffset;
	PCM_KEY_NODE NewKeyCell;
	ULONG NewBlockSize;
	NTSTATUS Status;
	USHORT NameSize;
	PWSTR NamePtr;
	BOOLEAN Packable;
	HV_STORAGE_TYPE Storage;
	ULONG i;

	DPRINT("CmiAddSubKey(%p '%wZ')\n", RegistryHive, SubKeyName);

	VERIFY_KEY_CELL(ParentKeyCell);

	/* Skip leading backslash */
	if (SubKeyName->Buffer[0] == L'\\')
	{
		NamePtr = &SubKeyName->Buffer[1];
		NameSize = SubKeyName->Length - sizeof(WCHAR);
	}
	else
	{
		NamePtr = SubKeyName->Buffer;
		NameSize = SubKeyName->Length;
	}

	/* Check whether key name can be packed */
	Packable = TRUE;
	for (i = 0; i < NameSize / sizeof(WCHAR); i++)
	{
		if (NamePtr[i] & 0xFF00)
		{
			Packable = FALSE;
			break;
		}
	}

	/* Adjust name size */
	if (Packable)
	{
		NameSize = NameSize / sizeof(WCHAR);
	}

	Status = STATUS_SUCCESS;

	Storage = (CreateOptions & REG_OPTION_VOLATILE) ? HvVolatile : HvStable;
	NewBlockSize = sizeof(CM_KEY_NODE) + NameSize;
	NKBOffset = HvAllocateCell(&RegistryHive->Hive, NewBlockSize, Storage);
	if (NKBOffset == HCELL_NULL)
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;
	}
	else
	{
		NewKeyCell = HvGetCell (&RegistryHive->Hive, NKBOffset);
		NewKeyCell->Id = REG_KEY_CELL_ID;
		if (CreateOptions & REG_OPTION_VOLATILE)
		{
			NewKeyCell->Flags = REG_KEY_VOLATILE_CELL;
		}
		else
		{
			NewKeyCell->Flags = 0;
		}
		KeQuerySystemTime(&NewKeyCell->LastWriteTime);
		NewKeyCell->Parent = HCELL_NULL;
		NewKeyCell->SubKeyCounts[HvStable] = 0;
		NewKeyCell->SubKeyCounts[HvVolatile] = 0;
		NewKeyCell->SubKeyLists[HvStable] = HCELL_NULL;
		NewKeyCell->SubKeyLists[HvVolatile] = HCELL_NULL;
		NewKeyCell->ValueList.Count = 0;
		NewKeyCell->ValueList.List = HCELL_NULL;
		NewKeyCell->SecurityKeyOffset = HCELL_NULL;
		NewKeyCell->ClassNameOffset = HCELL_NULL;

		/* Pack the key name */
		NewKeyCell->NameSize = NameSize;
		if (Packable)
		{
			NewKeyCell->Flags |= REG_KEY_NAME_PACKED;
			for (i = 0; i < NameSize; i++)
			{
				NewKeyCell->Name[i] = (CHAR)(NamePtr[i] & 0x00FF);
			}
		}
		else
		{
			RtlCopyMemory(
				NewKeyCell->Name,
				NamePtr,
				NameSize);
		}

		VERIFY_KEY_CELL(NewKeyCell);
	}

	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	if (ParentKeyCell->SubKeyLists[Storage] == HCELL_NULL)
	{
		Status = CmiAllocateHashTableCell (
			RegistryHive,
			&HashBlock,
			&ParentKeyCell->SubKeyLists[Storage],
			REG_INIT_HASH_TABLE_SIZE,
			Storage);
		if (!NT_SUCCESS(Status))
		{
			return(Status);
		}
	}
	else
	{
		HashBlock = HvGetCell (
			&RegistryHive->Hive,
			ParentKeyCell->SubKeyLists[Storage]);
		ASSERT(HashBlock->Id == REG_HASH_TABLE_CELL_ID);

		if (((ParentKeyCell->SubKeyCounts[Storage] + 1) >= HashBlock->HashTableSize))
		{
			PHASH_TABLE_CELL NewHashBlock;
			HCELL_INDEX HTOffset;

			/* Reallocate the hash table cell */
			Status = CmiAllocateHashTableCell (
				RegistryHive,
				&NewHashBlock,
				&HTOffset,
				HashBlock->HashTableSize +
				REG_EXTEND_HASH_TABLE_SIZE,
				Storage);
			if (!NT_SUCCESS(Status))
			{
				return Status;
			}

			RtlZeroMemory(
				&NewHashBlock->Table[0],
				sizeof(NewHashBlock->Table[0]) * NewHashBlock->HashTableSize);
			RtlCopyMemory(
				&NewHashBlock->Table[0],
				&HashBlock->Table[0],
				sizeof(NewHashBlock->Table[0]) * HashBlock->HashTableSize);
			HvFreeCell (&RegistryHive->Hive, ParentKeyCell->SubKeyLists[Storage]);
			ParentKeyCell->SubKeyLists[Storage] = HTOffset;
			HashBlock = NewHashBlock;
		}
	}

	Status = CmiAddKeyToHashTable(
		RegistryHive,
		HashBlock,
		ParentKeyCell,
		Storage,
		NewKeyCell,
		NKBOffset);
	if (NT_SUCCESS(Status))
	{
		ParentKeyCell->SubKeyCounts[Storage]++;
		*pSubKeyCell = NewKeyCell;
		*pBlockOffset = NKBOffset;
	}

	KeQuerySystemTime(&ParentKeyCell->LastWriteTime);
	HvMarkCellDirty(&RegistryHive->Hive, ParentKeyCellOffset);

	return Status;
}

static BOOLEAN
CmiCompareHash(
	IN PCUNICODE_STRING KeyName,
	IN PCHAR HashString)
{
	CHAR Buffer[4];

	Buffer[0] = (KeyName->Length >= 2) ? (CHAR)KeyName->Buffer[0] : 0;
	Buffer[1] = (KeyName->Length >= 4) ? (CHAR)KeyName->Buffer[1] : 0;
	Buffer[2] = (KeyName->Length >= 6) ? (CHAR)KeyName->Buffer[2] : 0;
	Buffer[3] = (KeyName->Length >= 8) ? (CHAR)KeyName->Buffer[3] : 0;

	return (strncmp(Buffer, HashString, 4) == 0);
}


BOOLEAN
CmiCompareHashI(
	IN PCUNICODE_STRING KeyName,
	IN PCHAR HashString)
{
	CHAR Buffer[4];

	Buffer[0] = (KeyName->Length >= 2) ? (CHAR)KeyName->Buffer[0] : 0;
	Buffer[1] = (KeyName->Length >= 4) ? (CHAR)KeyName->Buffer[1] : 0;
	Buffer[2] = (KeyName->Length >= 6) ? (CHAR)KeyName->Buffer[2] : 0;
	Buffer[3] = (KeyName->Length >= 8) ? (CHAR)KeyName->Buffer[3] : 0;

	return (strncasecmp(Buffer, HashString, 4) == 0);
}

static BOOLEAN
CmiCompareKeyNames(
	IN PCUNICODE_STRING KeyName,
	IN PCM_KEY_NODE KeyCell)
{
	PWCHAR UnicodeName;
	USHORT i;

	if (KeyCell->Flags & REG_KEY_NAME_PACKED)
	{
		if (KeyName->Length != KeyCell->NameSize * sizeof(WCHAR))
			return FALSE;

		for (i = 0; i < KeyCell->NameSize; i++)
		{
			if (KeyName->Buffer[i] != (WCHAR)KeyCell->Name[i])
				return FALSE;
		}
	}
	else
	{
		if (KeyName->Length != KeyCell->NameSize)
			return FALSE;

		UnicodeName = (PWCHAR)KeyCell->Name;
		for (i = 0; i < KeyCell->NameSize / sizeof(WCHAR); i++)
		{
			if (KeyName->Buffer[i] != UnicodeName[i])
				return FALSE;
		}
	}

	return TRUE;
}

static BOOLEAN
CmiCompareKeyNamesI(
	IN PCUNICODE_STRING KeyName,
	IN PCM_KEY_NODE KeyCell)
{
	PWCHAR UnicodeName;
	USHORT i;

	DPRINT("Flags: %hx\n", KeyCell->Flags);

	if (KeyCell->Flags & REG_KEY_NAME_PACKED)
	{
		if (KeyName->Length != KeyCell->NameSize * sizeof(WCHAR))
			return FALSE;

		/* FIXME: use _strnicmp */
		for (i = 0; i < KeyCell->NameSize; i++)
		{
			if (RtlUpcaseUnicodeChar(KeyName->Buffer[i]) !=
				RtlUpcaseUnicodeChar((WCHAR)KeyCell->Name[i]))
			return FALSE;
		}
	}
	else
	{
		if (KeyName->Length != KeyCell->NameSize)
			return FALSE;

		UnicodeName = (PWCHAR)KeyCell->Name;
		/* FIXME: use _strnicmp */
		for (i = 0; i < KeyCell->NameSize / sizeof(WCHAR); i++)
		{
			if (RtlUpcaseUnicodeChar(KeyName->Buffer[i]) !=
				RtlUpcaseUnicodeChar(UnicodeName[i]))
				return FALSE;
		}
	}

	return TRUE;
}

NTSTATUS
CmiScanForSubKey(
	IN PEREGISTRY_HIVE RegistryHive,
	IN PCM_KEY_NODE KeyCell,
	IN PCUNICODE_STRING SubKeyName,
	IN ULONG Attributes,
	OUT PCM_KEY_NODE *pSubKeyCell,
	OUT HCELL_INDEX *pBlockOffset)
{
	PHASH_TABLE_CELL HashBlock;
	PCM_KEY_NODE CurSubKeyCell;
	ULONG Storage;
	ULONG i;

	VERIFY_KEY_CELL(KeyCell);

	DPRINT("CmiScanForSubKey('%wZ')\n", SubKeyName);

	ASSERT(RegistryHive);

	*pSubKeyCell = NULL;

	for (Storage = HvStable; Storage < HvMaxStorageType; Storage++)
	{
		if (KeyCell->SubKeyLists[Storage] == HCELL_NULL)
		{
			/* The key does not have any subkeys */
			continue;
		}

		/* Get hash table */
		HashBlock = HvGetCell (&RegistryHive->Hive, KeyCell->SubKeyLists[Storage]);
		if (!HashBlock || HashBlock->Id != REG_HASH_TABLE_CELL_ID)
			return STATUS_UNSUCCESSFUL;

		for (i = 0; i < KeyCell->SubKeyCounts[Storage]; i++)
		{
			if (Attributes & OBJ_CASE_INSENSITIVE)
			{
				if ((HashBlock->Table[i].HashValue == 0
				 || CmiCompareHashI(SubKeyName, (PCHAR)&HashBlock->Table[i].HashValue)))
				{
					CurSubKeyCell = HvGetCell (
						&RegistryHive->Hive,
						HashBlock->Table[i].KeyOffset);

					if (CmiCompareKeyNamesI(SubKeyName, CurSubKeyCell))
					{
						*pSubKeyCell = CurSubKeyCell;
						*pBlockOffset = HashBlock->Table[i].KeyOffset;
						return STATUS_SUCCESS;
					}
				}
			}
			else
			{
				if ((HashBlock->Table[i].HashValue == 0
				 || CmiCompareHash(SubKeyName, (PCHAR)&HashBlock->Table[i].HashValue)))
				{
					CurSubKeyCell = HvGetCell (
						&RegistryHive->Hive,
						HashBlock->Table[i].KeyOffset);

					if (CmiCompareKeyNames(SubKeyName, CurSubKeyCell))
					{
						*pSubKeyCell = CurSubKeyCell;
						*pBlockOffset = HashBlock->Table[i].KeyOffset;
						return STATUS_SUCCESS;
					}
				}
			}
		}
	}

	return STATUS_OBJECT_NAME_NOT_FOUND;
}

static USHORT
CmiGetPackedNameLength(
	IN PCUNICODE_STRING Name,
	OUT PBOOLEAN pPackable)
{
	USHORT i;

	*pPackable = TRUE;

	for (i = 0; i < Name->Length / sizeof(WCHAR); i++)
	{
		if (Name->Buffer[i] & 0xFF00)
		{
			*pPackable = FALSE;
			return Name->Length;
		}
	}

	return (Name->Length / sizeof(WCHAR));
}

static NTSTATUS
CmiAllocateValueCell(
	IN PEREGISTRY_HIVE RegistryHive,
	OUT PCM_KEY_VALUE *ValueCell,
	OUT HCELL_INDEX *VBOffset,
	IN PCUNICODE_STRING ValueName,
	IN HV_STORAGE_TYPE Storage)
{
	PCM_KEY_VALUE NewValueCell;
	BOOLEAN Packable;
	USHORT NameSize, i;
	NTSTATUS Status;

	Status = STATUS_SUCCESS;

	NameSize = CmiGetPackedNameLength(ValueName, &Packable);

	DPRINT("ValueName->Length %lu  NameSize %lu\n", ValueName->Length, NameSize);

	*VBOffset = HvAllocateCell(&RegistryHive->Hive, sizeof(CM_KEY_VALUE) + NameSize, Storage);
	if (*VBOffset == HCELL_NULL)
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;
	}
	else
	{
		ASSERT(NameSize <= USHORT_MAX);
		NewValueCell = HvGetCell (&RegistryHive->Hive, *VBOffset);
		NewValueCell->Id = REG_VALUE_CELL_ID;
		NewValueCell->NameSize = (USHORT)NameSize;
		if (Packable)
		{
			/* Pack the value name */
			for (i = 0; i < NameSize; i++)
				NewValueCell->Name[i] = (CHAR)ValueName->Buffer[i];
			NewValueCell->Flags |= REG_VALUE_NAME_PACKED;
		}
		else
		{
			/* Copy the value name */
			RtlCopyMemory(
				NewValueCell->Name,
				ValueName->Buffer,
				NameSize);
			NewValueCell->Flags = 0;
		}
		NewValueCell->DataType = 0;
		NewValueCell->DataSize = 0;
		NewValueCell->DataOffset = HCELL_NULL;
		*ValueCell = NewValueCell;
	}

	return Status;
}

NTSTATUS
CmiAddValueKey(
	IN PEREGISTRY_HIVE RegistryHive,
	IN PCM_KEY_NODE KeyCell,
	IN HCELL_INDEX KeyCellOffset,
	IN PCUNICODE_STRING ValueName,
	OUT PCM_KEY_VALUE *pValueCell,
	OUT HCELL_INDEX *pValueCellOffset)
{
	PVALUE_LIST_CELL ValueListCell;
	PCM_KEY_VALUE NewValueCell;
	HCELL_INDEX ValueListCellOffset;
	HCELL_INDEX NewValueCellOffset;
	ULONG CellSize;
	HV_STORAGE_TYPE Storage;
	NTSTATUS Status;

	Storage = (KeyCell->Flags & REG_KEY_VOLATILE_CELL) ? HvVolatile : HvStable;
	if (KeyCell->ValueList.List == HCELL_NULL)
	{
		/* Allocate some room for the value list */
		CellSize = sizeof(VALUE_LIST_CELL) + (3 * sizeof(HCELL_INDEX));
		ValueListCellOffset = HvAllocateCell(&RegistryHive->Hive, CellSize, Storage);
		if (ValueListCellOffset == HCELL_NULL)
			return STATUS_INSUFFICIENT_RESOURCES;

		ValueListCell = HvGetCell(&RegistryHive->Hive, ValueListCellOffset);
		if (!ValueListCell)
			return STATUS_UNSUCCESSFUL;
		KeyCell->ValueList.List = ValueListCellOffset;
		HvMarkCellDirty(&RegistryHive->Hive, KeyCellOffset);
	}
	else
	{
		ValueListCell = (PVALUE_LIST_CELL)HvGetCell(&RegistryHive->Hive, KeyCell->ValueList.List);
		if (!ValueListCell)
			return STATUS_UNSUCCESSFUL;
		CellSize = ABS_VALUE(HvGetCellSize(&RegistryHive->Hive, ValueListCell));

		if (KeyCell->ValueList.Count >= CellSize / sizeof(HCELL_INDEX))
		{
			CellSize *= 2;
			ValueListCellOffset = HvReallocateCell(&RegistryHive->Hive, KeyCell->ValueList.List, CellSize);
			if (ValueListCellOffset == HCELL_NULL)
				return STATUS_INSUFFICIENT_RESOURCES;

			ValueListCell = HvGetCell(&RegistryHive->Hive, ValueListCellOffset);
			if (!ValueListCell)
				return STATUS_UNSUCCESSFUL;
			KeyCell->ValueList.List = ValueListCellOffset;
			HvMarkCellDirty(&RegistryHive->Hive, KeyCellOffset);
        }
	}

	Status = CmiAllocateValueCell(
		RegistryHive,
		&NewValueCell,
		&NewValueCellOffset,
		ValueName,
		Storage);
	if (!NT_SUCCESS(Status))
		return Status;

	ValueListCell->ValueOffset[KeyCell->ValueList.Count] = NewValueCellOffset;
	KeyCell->ValueList.Count++;

	HvMarkCellDirty(&RegistryHive->Hive, KeyCellOffset);
	HvMarkCellDirty(&RegistryHive->Hive, KeyCell->ValueList.List);
	HvMarkCellDirty(&RegistryHive->Hive, NewValueCellOffset);

	*pValueCell = NewValueCell;
	*pValueCellOffset = NewValueCellOffset;

	return STATUS_SUCCESS;
}

static BOOLEAN
CmiComparePackedNames(
	IN PCUNICODE_STRING Name,
	IN PUCHAR NameBuffer,
	IN USHORT NameBufferSize,
	IN BOOLEAN NamePacked)
{
	PWCHAR UNameBuffer;
	ULONG i;

	if (NamePacked == TRUE)
	{
		if (Name->Length != NameBufferSize * sizeof(WCHAR))
			return FALSE;

		for (i = 0; i < Name->Length / sizeof(WCHAR); i++)
		{
			if (RtlUpcaseUnicodeChar(Name->Buffer[i]) != RtlUpcaseUnicodeChar((WCHAR)NameBuffer[i]))
				return FALSE;
		}
	}
	else
	{
		if (Name->Length != NameBufferSize)
			return FALSE;

		UNameBuffer = (PWCHAR)NameBuffer;

		for (i = 0; i < Name->Length / sizeof(WCHAR); i++)
		{
			if (RtlUpcaseUnicodeChar(Name->Buffer[i]) != RtlUpcaseUnicodeChar(UNameBuffer[i]))
				return FALSE;
		}
	}

	return TRUE;
}

NTSTATUS
CmiScanForValueKey(
	IN PEREGISTRY_HIVE RegistryHive,
	IN PCM_KEY_NODE KeyCell,
	IN PCUNICODE_STRING ValueName,
	OUT PCM_KEY_VALUE *pValueCell,
	OUT HCELL_INDEX *pValueCellOffset)
{
	PVALUE_LIST_CELL ValueListCell;
	PCM_KEY_VALUE CurValueCell;
	ULONG i;

	*pValueCell = NULL;
	*pValueCellOffset = HCELL_NULL;

	/* The key does not have any values */
	if (KeyCell->ValueList.List == HCELL_NULL)
    {
		return STATUS_OBJECT_NAME_NOT_FOUND;
    }

	ValueListCell = HvGetCell(&RegistryHive->Hive, KeyCell->ValueList.List);

	VERIFY_VALUE_LIST_CELL(ValueListCell);

	for (i = 0; i < KeyCell->ValueList.Count; i++)
	{
		CurValueCell = HvGetCell(
			&RegistryHive->Hive,
			ValueListCell->ValueOffset[i]);

		if (CmiComparePackedNames(
			ValueName,
			CurValueCell->Name,
			CurValueCell->NameSize,
			(BOOLEAN)((CurValueCell->Flags & REG_VALUE_NAME_PACKED) ? TRUE : FALSE)))
		{
			*pValueCell = CurValueCell;
			*pValueCellOffset = ValueListCell->ValueOffset[i];
			return STATUS_SUCCESS;
		}
	}

	return STATUS_OBJECT_NAME_NOT_FOUND;
}
