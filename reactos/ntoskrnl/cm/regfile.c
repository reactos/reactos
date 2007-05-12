/* $Id$
 *
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/regfile.c
 * PURPOSE:         Registry file manipulation routines
 *
 * PROGRAMMERS:     Casper Hornstrup
 *                  Eric Kohl
 *                  Filip Navara
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#include "cm.h"
#include "../config/cm.h"

/* LOCAL MACROS *************************************************************/

#define ABS_VALUE(V) (((V) < 0) ? -(V) : (V))

/* FUNCTIONS ****************************************************************/

NTSTATUS
CmiLoadHive(IN POBJECT_ATTRIBUTES KeyObjectAttributes,
            IN PUNICODE_STRING FileName,
            IN ULONG Flags)
{
    PEREGISTRY_HIVE Hive;
    NTSTATUS Status;
    BOOLEAN Allocate = TRUE;

    DPRINT ("CmiLoadHive(Filename %wZ)\n", FileName);

    if (Flags & ~REG_NO_LAZY_FLUSH) return STATUS_INVALID_PARAMETER;

    Status = CmpInitHiveFromFile(FileName,
                                 (Flags & REG_NO_LAZY_FLUSH) ? HIVE_NO_SYNCH : 0,
                                 &Hive,
                                 &Allocate,
                                 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmpInitHiveFromFile() failed (Status %lx)\n", Status);
        ExFreePool(Hive);
        return Status;
    }

    Status = CmiConnectHive(KeyObjectAttributes, Hive);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1 ("CmiConnectHive() failed (Status %lx)\n", Status);
        //      CmiRemoveRegistryHive (Hive);
    }

    DPRINT ("CmiLoadHive() done\n");
    return Status;
}


NTSTATUS
CmiRemoveRegistaryHive(PEREGISTRY_HIVE RegistryHive)
{
  /* Remove hive from hive list */
  RemoveEntryList (&RegistryHive->HiveList);

  /* Release file names */
  RtlFreeUnicodeString (&RegistryHive->HiveFileName);

  /* Release hive */
  HvFree (&RegistryHive->Hive);

  return STATUS_SUCCESS;
}

VOID
CmCloseHiveFiles(PEREGISTRY_HIVE RegistryHive)
{
  ZwClose(RegistryHive->HiveHandle);
  ZwClose(RegistryHive->LogHandle);
}


NTSTATUS
CmiFlushRegistryHive(PEREGISTRY_HIVE RegistryHive)
{
  BOOLEAN Success;
  NTSTATUS Status;
  ULONG Disposition;

  ASSERT(!IsNoFileHive(RegistryHive));

  if (RtlFindSetBits(&RegistryHive->Hive.DirtyVector, 1, 0) == ~0)
    {
      return(STATUS_SUCCESS);
    }

  Status = CmpOpenHiveFiles(&RegistryHive->HiveFileName,
                            L".LOG",
                            &RegistryHive->HiveHandle,
                            &RegistryHive->LogHandle,
                            &Disposition,
                            &Disposition,
                            FALSE,
                            FALSE,
                            TRUE,
                            NULL);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  Success = HvSyncHive(&RegistryHive->Hive);

  CmCloseHiveFiles(RegistryHive);

  return Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


ULONG
CmiGetNumberOfSubKeys(PKEY_OBJECT KeyObject)
{
  PCM_KEY_NODE KeyCell;
  ULONG SubKeyCount;

  VERIFY_KEY_OBJECT(KeyObject);

  KeyCell = KeyObject->KeyCell;
  VERIFY_KEY_CELL(KeyCell);

  SubKeyCount = (KeyCell == NULL) ? 0 :
                KeyCell->SubKeyCounts[HvStable] +
                KeyCell->SubKeyCounts[HvVolatile];

  return SubKeyCount;
}


ULONG
CmiGetMaxNameLength(PKEY_OBJECT KeyObject)
{
  PHASH_TABLE_CELL HashBlock;
  PCM_KEY_NODE CurSubKeyCell;
  PCM_KEY_NODE KeyCell;
  ULONG MaxName;
  ULONG NameSize;
  ULONG i;
  ULONG Storage;

  VERIFY_KEY_OBJECT(KeyObject);

  KeyCell = KeyObject->KeyCell;
  VERIFY_KEY_CELL(KeyCell);

  MaxName = 0;
  for (Storage = HvStable; Storage < HvMaxStorageType; Storage++)
    {
      if (KeyCell->SubKeyLists[Storage] != HCELL_NULL)
        {
          HashBlock = HvGetCell (&KeyObject->RegistryHive->Hive, KeyCell->SubKeyLists[Storage]);
          ASSERT(HashBlock->Id == REG_HASH_TABLE_CELL_ID);

          for (i = 0; i < KeyCell->SubKeyCounts[Storage]; i++)
            {
              CurSubKeyCell = HvGetCell (&KeyObject->RegistryHive->Hive,
                                         HashBlock->Table[i].KeyOffset);
              NameSize = CurSubKeyCell->NameSize;
              if (CurSubKeyCell->Flags & REG_KEY_NAME_PACKED)
                 NameSize *= sizeof(WCHAR);
              if (NameSize > MaxName)
                 MaxName = NameSize;
            }
        }
    }

  DPRINT ("MaxName %lu\n", MaxName);

  return MaxName;
}


ULONG
CmiGetMaxClassLength(PKEY_OBJECT  KeyObject)
{
  PHASH_TABLE_CELL HashBlock;
  PCM_KEY_NODE CurSubKeyCell;
  PCM_KEY_NODE KeyCell;
  ULONG MaxClass;
  ULONG i;
  ULONG Storage;

  VERIFY_KEY_OBJECT(KeyObject);

  KeyCell = KeyObject->KeyCell;
  VERIFY_KEY_CELL(KeyCell);

  MaxClass = 0;
  for (Storage = HvStable; Storage < HvMaxStorageType; Storage++)
    {
      if (KeyCell->SubKeyLists[Storage] != HCELL_NULL)
        {
          HashBlock = HvGetCell (&KeyObject->RegistryHive->Hive,
                                 KeyCell->SubKeyLists[Storage]);
          ASSERT(HashBlock->Id == REG_HASH_TABLE_CELL_ID);

          for (i = 0; i < KeyCell->SubKeyCounts[Storage]; i++)
            {
              CurSubKeyCell = HvGetCell (&KeyObject->RegistryHive->Hive,
                                         HashBlock->Table[i].KeyOffset);

              if (MaxClass < CurSubKeyCell->ClassSize)
                {
                  MaxClass = CurSubKeyCell->ClassSize;
                }
            }
        }
    }

  return MaxClass;
}


ULONG
CmiGetMaxValueNameLength(PEREGISTRY_HIVE RegistryHive,
			 PCM_KEY_NODE KeyCell)
{
  PVALUE_LIST_CELL ValueListCell;
  PCM_KEY_VALUE CurValueCell;
  ULONG MaxValueName;
  ULONG Size;
  ULONG i;

  VERIFY_KEY_CELL(KeyCell);

  if (KeyCell->ValueList.List == HCELL_NULL)
    {
      return 0;
    }

  MaxValueName = 0;
  ValueListCell = HvGetCell (&RegistryHive->Hive,
			     KeyCell->ValueList.List);

  for (i = 0; i < KeyCell->ValueList.Count; i++)
    {
      CurValueCell = HvGetCell (&RegistryHive->Hive,
				ValueListCell->ValueOffset[i]);
      if (CurValueCell == NULL)
	{
	  DPRINT("CmiGetBlock() failed\n");
	}

      if (CurValueCell != NULL)
	{
	  Size = CurValueCell->NameSize;
	  if (CurValueCell->Flags & REG_VALUE_NAME_PACKED)
	    {
	      Size *= sizeof(WCHAR);
	    }
	  if (MaxValueName < Size)
	    {
	      MaxValueName = Size;
	    }
        }
    }

  return MaxValueName;
}


ULONG
CmiGetMaxValueDataLength(PEREGISTRY_HIVE RegistryHive,
			 PCM_KEY_NODE KeyCell)
{
  PVALUE_LIST_CELL ValueListCell;
  PCM_KEY_VALUE CurValueCell;
  LONG MaxValueData;
  ULONG i;

  VERIFY_KEY_CELL(KeyCell);

  if (KeyCell->ValueList.List == HCELL_NULL)
    {
      return 0;
    }

  MaxValueData = 0;
  ValueListCell = HvGetCell (&RegistryHive->Hive, KeyCell->ValueList.List);

  for (i = 0; i < KeyCell->ValueList.Count; i++)
    {
      CurValueCell = HvGetCell (&RegistryHive->Hive,
                                ValueListCell->ValueOffset[i]);
      if ((MaxValueData < (LONG)(CurValueCell->DataSize & REG_DATA_SIZE_MASK)))
        {
          MaxValueData = CurValueCell->DataSize & REG_DATA_SIZE_MASK;
        }
    }

  return MaxValueData;
}

NTSTATUS
CmiScanKeyForValue(IN PEREGISTRY_HIVE RegistryHive,
                   IN PCM_KEY_NODE KeyCell,
                   IN PUNICODE_STRING ValueName,
                   OUT PCM_KEY_VALUE *ValueCell,
                   OUT HCELL_INDEX *ValueCellOffset)
{
    HCELL_INDEX CellIndex;

    /* Assume failure */
    *ValueCell = NULL;
    if (ValueCellOffset) *ValueCellOffset = HCELL_NIL;

    /* Call newer Cm API */
    CellIndex = CmpFindValueByName(&RegistryHive->Hive, KeyCell, ValueName);
    if (CellIndex == HCELL_NIL) return STATUS_OBJECT_NAME_NOT_FOUND;

    /* Otherwise, get the cell data back too */
    if (ValueCellOffset) *ValueCellOffset = CellIndex;
    *ValueCell =  HvGetCell(&RegistryHive->Hive, CellIndex);
    return STATUS_SUCCESS;
}

NTSTATUS
CmiScanForSubKey(IN PEREGISTRY_HIVE RegistryHive,
                 IN PCM_KEY_NODE KeyCell,
                 OUT PCM_KEY_NODE *SubKeyCell,
                 OUT HCELL_INDEX *BlockOffset,
                 IN PUNICODE_STRING KeyName,
                 IN ACCESS_MASK DesiredAccess,
                 IN ULONG Attributes)
{
    HCELL_INDEX CellIndex;

    /* Assume failure */
    *SubKeyCell = NULL;

    /* Call newer Cm API */
    CellIndex = CmpFindSubKeyByName(&RegistryHive->Hive, KeyCell, KeyName);
    if (CellIndex == HCELL_NIL) return STATUS_OBJECT_NAME_NOT_FOUND;

    /* Otherwise, get the cell data back too */
    *BlockOffset = CellIndex;
    *SubKeyCell = HvGetCell(&RegistryHive->Hive, CellIndex);
    return STATUS_SUCCESS;
}

NTSTATUS
CmiAddSubKey(PEREGISTRY_HIVE RegistryHive,
	     PKEY_OBJECT ParentKey,
	     PKEY_OBJECT SubKey,
	     PUNICODE_STRING SubKeyName,
	     ULONG TitleIndex,
	     PUNICODE_STRING Class,
	     ULONG CreateOptions)
{
  PHASH_TABLE_CELL HashBlock;
  HCELL_INDEX NKBOffset;
  PCM_KEY_NODE NewKeyCell;
  ULONG NewBlockSize;
  PCM_KEY_NODE ParentKeyCell;
  PVOID ClassCell;
  NTSTATUS Status;
  USHORT NameSize;
  PWSTR NamePtr;
  BOOLEAN Packable;
  HV_STORAGE_TYPE Storage;
  ULONG i;

  ParentKeyCell = ParentKey->KeyCell;

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

  DPRINT("Key %S  Length %lu  %s\n", NamePtr, NameSize, (Packable)?"True":"False");

  Status = STATUS_SUCCESS;

  Storage = (CreateOptions & REG_OPTION_VOLATILE) ? HvVolatile : HvStable;
  NewBlockSize = sizeof(CM_KEY_NODE) + NameSize;
  NKBOffset = HvAllocateCell (&RegistryHive->Hive, NewBlockSize, Storage);
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
	  RtlCopyMemory(NewKeyCell->Name,
			NamePtr,
			NameSize);
	}

      VERIFY_KEY_CELL(NewKeyCell);

      if (Class != NULL && Class->Length)
	{
	  NewKeyCell->ClassSize = Class->Length;
	  NewKeyCell->ClassNameOffset = HvAllocateCell(
	    &RegistryHive->Hive, NewKeyCell->ClassSize, HvStable);
	  ASSERT(NewKeyCell->ClassNameOffset != HCELL_NULL); /* FIXME */

	  ClassCell = HvGetCell(&RegistryHive->Hive, NewKeyCell->ClassNameOffset);
	  RtlCopyMemory (ClassCell,
			 Class->Buffer,
			 Class->Length);
	}
    }

  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  SubKey->KeyCell = NewKeyCell;
  SubKey->KeyCellOffset = NKBOffset;

  if (ParentKeyCell->SubKeyLists[Storage] == HCELL_NULL)
    {
      Status = CmiAllocateHashTableCell (RegistryHive,
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
      HashBlock = HvGetCell (&RegistryHive->Hive,
			     ParentKeyCell->SubKeyLists[Storage]);
      ASSERT(HashBlock->Id == REG_HASH_TABLE_CELL_ID);

      if (((ParentKeyCell->SubKeyCounts[Storage] + 1) >= HashBlock->HashTableSize))
	{
	  PHASH_TABLE_CELL NewHashBlock;
	  HCELL_INDEX HTOffset;

	  /* Reallocate the hash table cell */
	  Status = CmiAllocateHashTableCell (RegistryHive,
					     &NewHashBlock,
					     &HTOffset,
					     HashBlock->HashTableSize +
					       REG_EXTEND_HASH_TABLE_SIZE,
					     Storage);
	  if (!NT_SUCCESS(Status))
	    {
	      return Status;
	    }

	  RtlZeroMemory(&NewHashBlock->Table[0],
			sizeof(NewHashBlock->Table[0]) * NewHashBlock->HashTableSize);
	  RtlCopyMemory(&NewHashBlock->Table[0],
			&HashBlock->Table[0],
			sizeof(NewHashBlock->Table[0]) * HashBlock->HashTableSize);
	  HvFreeCell (&RegistryHive->Hive, ParentKeyCell->SubKeyLists[Storage]);
	  ParentKeyCell->SubKeyLists[Storage] = HTOffset;
	  HashBlock = NewHashBlock;
	}
    }

  Status = CmiAddKeyToHashTable(RegistryHive,
				HashBlock,
				ParentKeyCell,
				Storage,
				NewKeyCell,
				NKBOffset);
  if (NT_SUCCESS(Status))
    {
      ParentKeyCell->SubKeyCounts[Storage]++;
    }

  KeQuerySystemTime (&ParentKeyCell->LastWriteTime);
  HvMarkCellDirty (&RegistryHive->Hive, ParentKey->KeyCellOffset);

  return(Status);
}


NTSTATUS
CmiRemoveSubKey(PEREGISTRY_HIVE RegistryHive,
		PKEY_OBJECT ParentKey,
		PKEY_OBJECT SubKey)
{
  PHASH_TABLE_CELL HashBlock;
  PVALUE_LIST_CELL ValueList;
  PCM_KEY_VALUE ValueCell;
  HV_STORAGE_TYPE Storage;
  ULONG i;

  DPRINT("CmiRemoveSubKey() called\n");

  Storage = (SubKey->KeyCell->Flags & REG_KEY_VOLATILE_CELL) ? HvVolatile : HvStable;

  /* Remove all values */
  if (SubKey->KeyCell->ValueList.Count != 0)
    {
      /* Get pointer to the value list cell */
      ValueList = HvGetCell (&RegistryHive->Hive, SubKey->KeyCell->ValueList.List);

      /* Enumerate all values */
      for (i = 0; i < SubKey->KeyCell->ValueList.Count; i++)
	{
	  /* Get pointer to value cell */
	  ValueCell = HvGetCell(&RegistryHive->Hive,
				ValueList->ValueOffset[i]);

	  if (!(ValueCell->DataSize & REG_DATA_IN_OFFSET)
              && ValueCell->DataSize > sizeof(HCELL_INDEX)
              && ValueCell->DataOffset != HCELL_NULL)
	    {
	      /* Destroy data cell */
	      HvFreeCell (&RegistryHive->Hive, ValueCell->DataOffset);
	    }

	  /* Destroy value cell */
	  HvFreeCell (&RegistryHive->Hive, ValueList->ValueOffset[i]);
	}

      /* Destroy value list cell */
      HvFreeCell (&RegistryHive->Hive, SubKey->KeyCell->ValueList.List);

      SubKey->KeyCell->ValueList.Count = 0;
      SubKey->KeyCell->ValueList.List = (HCELL_INDEX)-1;

      HvMarkCellDirty(&RegistryHive->Hive, SubKey->KeyCellOffset);
    }

  /* Remove the key from the parent key's hash block */
  if (ParentKey->KeyCell->SubKeyLists[Storage] != HCELL_NULL)
    {
      DPRINT("ParentKey SubKeyLists %lx\n", ParentKey->KeyCell->SubKeyLists[Storage]);
      HashBlock = HvGetCell (&ParentKey->RegistryHive->Hive,
			     ParentKey->KeyCell->SubKeyLists[Storage]);
      ASSERT(HashBlock->Id == REG_HASH_TABLE_CELL_ID);
      DPRINT("ParentKey HashBlock %p\n", HashBlock);
      CmiRemoveKeyFromHashTable(ParentKey->RegistryHive,
				HashBlock,
				SubKey->KeyCellOffset);
      HvMarkCellDirty(&ParentKey->RegistryHive->Hive,
                      ParentKey->KeyCell->SubKeyLists[Storage]);
    }

  /* Remove the key's hash block */
  if (SubKey->KeyCell->SubKeyLists[Storage] != HCELL_NULL)
    {
      DPRINT("SubKey SubKeyLists %lx\n", SubKey->KeyCell->SubKeyLists[Storage]);
      HvFreeCell (&RegistryHive->Hive, SubKey->KeyCell->SubKeyLists[Storage]);
      SubKey->KeyCell->SubKeyLists[Storage] = HCELL_NULL;
    }

  /* Decrement the number of the parent key's sub keys */
  if (ParentKey != NULL)
    {
      DPRINT("ParentKey %p\n", ParentKey);
      ParentKey->KeyCell->SubKeyCounts[Storage]--;

      /* Remove the parent key's hash table */
      if (ParentKey->KeyCell->SubKeyCounts[Storage] == 0 &&
          ParentKey->KeyCell->SubKeyLists[Storage] != HCELL_NULL)
	{
	  DPRINT("ParentKey SubKeyLists %lx\n", ParentKey->KeyCell->SubKeyLists);
	  HvFreeCell (&ParentKey->RegistryHive->Hive,
		      ParentKey->KeyCell->SubKeyLists[Storage]);
	  ParentKey->KeyCell->SubKeyLists[Storage] = HCELL_NULL;
	}

      KeQuerySystemTime(&ParentKey->KeyCell->LastWriteTime);
      HvMarkCellDirty(&ParentKey->RegistryHive->Hive,
                      ParentKey->KeyCellOffset);
    }

  /* Destroy key cell */
  HvFreeCell (&RegistryHive->Hive, SubKey->KeyCellOffset);
  SubKey->KeyCell = NULL;
  SubKey->KeyCellOffset = (HCELL_INDEX)-1;

  DPRINT("CmiRemoveSubKey() done\n");

  return STATUS_SUCCESS;
}

NTSTATUS
CmiGetValueFromKeyByIndex(IN PEREGISTRY_HIVE RegistryHive,
			  IN PCM_KEY_NODE KeyCell,
			  IN ULONG Index,
			  OUT PCM_KEY_VALUE *ValueCell)
{
  PVALUE_LIST_CELL ValueListCell;
  PCM_KEY_VALUE CurValueCell;

  *ValueCell = NULL;

  if (KeyCell->ValueList.List == (HCELL_INDEX)-1)
    {
      return STATUS_NO_MORE_ENTRIES;
    }

  if (Index >= KeyCell->ValueList.Count)
    {
      return STATUS_NO_MORE_ENTRIES;
    }


  ValueListCell = HvGetCell (&RegistryHive->Hive, KeyCell->ValueList.List);

  VERIFY_VALUE_LIST_CELL(ValueListCell);

  CurValueCell = HvGetCell (&RegistryHive->Hive, ValueListCell->ValueOffset[Index]);

  *ValueCell = CurValueCell;

  return STATUS_SUCCESS;
}


NTSTATUS
CmiAddValueToKey(IN PEREGISTRY_HIVE RegistryHive,
		 IN PCM_KEY_NODE KeyCell,
		 IN HCELL_INDEX KeyCellOffset,
		 IN PUNICODE_STRING ValueName,
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

  DPRINT("KeyCell->ValuesOffset %lu\n", (ULONG)KeyCell->ValueList.List);

  Storage = (KeyCell->Flags & REG_KEY_VOLATILE_CELL) ? HvVolatile : HvStable;
  if (KeyCell->ValueList.List == HCELL_NULL)
    {
      CellSize = sizeof(VALUE_LIST_CELL) +
		 (3 * sizeof(HCELL_INDEX));
      ValueListCellOffset = HvAllocateCell (&RegistryHive->Hive, CellSize, Storage);
      if (ValueListCellOffset == HCELL_NULL)
	{
	  return STATUS_INSUFFICIENT_RESOURCES;
	}

      ValueListCell = HvGetCell (&RegistryHive->Hive, ValueListCellOffset);
      KeyCell->ValueList.List = ValueListCellOffset;
      HvMarkCellDirty(&RegistryHive->Hive, KeyCellOffset);
    }
  else
    {
      ValueListCell = (PVALUE_LIST_CELL) HvGetCell (&RegistryHive->Hive, KeyCell->ValueList.List);
      CellSize = ABS_VALUE(HvGetCellSize(&RegistryHive->Hive, ValueListCell));

      if (KeyCell->ValueList.Count >=
	   (CellSize / sizeof(HCELL_INDEX)))
        {
          CellSize *= 2;
          ValueListCellOffset = HvReallocateCell (&RegistryHive->Hive, KeyCell->ValueList.List, CellSize);
          if (ValueListCellOffset == HCELL_NULL)
            {
              return STATUS_INSUFFICIENT_RESOURCES;
            }

          ValueListCell = HvGetCell (&RegistryHive->Hive, ValueListCellOffset);
          KeyCell->ValueList.List = ValueListCellOffset;
          HvMarkCellDirty (&RegistryHive->Hive, KeyCellOffset);
        }
    }

#if 0
  DPRINT("KeyCell->ValueList.Count %lu, ValueListCell->Size %lu (%lu %lx)\n",
	 KeyCell->ValueList.Count,
	 (ULONG)ABS_VALUE(ValueListCell->Size),
	 ((ULONG)ABS_VALUE(ValueListCell->Size) - sizeof(HCELL)) / sizeof(HCELL_INDEX),
	 ((ULONG)ABS_VALUE(ValueListCell->Size) - sizeof(HCELL)) / sizeof(HCELL_INDEX));
#endif

  Status = CmiAllocateValueCell(RegistryHive,
				&NewValueCell,
				&NewValueCellOffset,
				ValueName,
				Storage);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  ValueListCell->ValueOffset[KeyCell->ValueList.Count] = NewValueCellOffset;
  KeyCell->ValueList.Count++;

  HvMarkCellDirty(&RegistryHive->Hive, KeyCellOffset);
  HvMarkCellDirty(&RegistryHive->Hive, KeyCell->ValueList.List);
  HvMarkCellDirty(&RegistryHive->Hive, NewValueCellOffset);

  *pValueCell = NewValueCell;
  *pValueCellOffset = NewValueCellOffset;

  return STATUS_SUCCESS;
}


NTSTATUS
CmiDeleteValueFromKey(IN PEREGISTRY_HIVE RegistryHive,
		      IN PCM_KEY_NODE KeyCell,
		      IN HCELL_INDEX KeyCellOffset,
		      IN PUNICODE_STRING ValueName)
{
  PVALUE_LIST_CELL ValueListCell;
  PCM_KEY_VALUE CurValueCell;
  ULONG i;
  NTSTATUS Status;

  if (KeyCell->ValueList.List == -1)
    {
      return STATUS_OBJECT_NAME_NOT_FOUND;
    }

  ValueListCell = HvGetCell (&RegistryHive->Hive, KeyCell->ValueList.List);

  VERIFY_VALUE_LIST_CELL(ValueListCell);

  for (i = 0; i < KeyCell->ValueList.Count; i++)
    {
      CurValueCell = HvGetCell (&RegistryHive->Hive, ValueListCell->ValueOffset[i]);

      if (CmiComparePackedNames(ValueName,
				CurValueCell->Name,
				CurValueCell->NameSize,
				(BOOLEAN)((CurValueCell->Flags & REG_VALUE_NAME_PACKED) ? TRUE : FALSE)))
	{
	  Status = CmiDestroyValueCell(RegistryHive,
				       CurValueCell,
				       ValueListCell->ValueOffset[i]);
	  if (CurValueCell == NULL)
	    {
	      DPRINT1("CmiDestroyValueCell() failed\n");
	      return Status;
	    }

	  if (i < (KeyCell->ValueList.Count - 1))
	    {
	      RtlMoveMemory(&ValueListCell->ValueOffset[i],
			    &ValueListCell->ValueOffset[i + 1],
			    sizeof(HCELL_INDEX) * (KeyCell->ValueList.Count - 1 - i));
	    }
	  ValueListCell->ValueOffset[KeyCell->ValueList.Count - 1] = 0;


	  KeyCell->ValueList.Count--;

	  if (KeyCell->ValueList.Count == 0)
	    {
	      HvFreeCell(&RegistryHive->Hive, KeyCell->ValueList.List);
	      KeyCell->ValueList.List = -1;
	    }
	  else
	    {
	      HvMarkCellDirty(&RegistryHive->Hive,
		              KeyCell->ValueList.List);
	    }

	  HvMarkCellDirty(&RegistryHive->Hive,
			  KeyCellOffset);

	  return STATUS_SUCCESS;
	}
    }

  DPRINT("Couldn't find the desired value\n");

  return STATUS_OBJECT_NAME_NOT_FOUND;
}


NTSTATUS
CmiAllocateHashTableCell (IN PEREGISTRY_HIVE RegistryHive,
	OUT PHASH_TABLE_CELL *HashBlock,
	OUT HCELL_INDEX *HBOffset,
	IN ULONG SubKeyCount,
	IN HV_STORAGE_TYPE Storage)
{
  PHASH_TABLE_CELL NewHashBlock;
  ULONG NewHashSize;
  NTSTATUS Status;

  Status = STATUS_SUCCESS;
  *HashBlock = NULL;
  NewHashSize = sizeof(HASH_TABLE_CELL) +
		(SubKeyCount * sizeof(HASH_RECORD));
  *HBOffset = HvAllocateCell (&RegistryHive->Hive, NewHashSize, Storage);

  if (*HBOffset == HCELL_NULL)
    {
      Status = STATUS_INSUFFICIENT_RESOURCES;
    }
  else
    {
      ASSERT(SubKeyCount <= 0xffff); /* should really be USHORT_MAX or similar */
      NewHashBlock = HvGetCell (&RegistryHive->Hive, *HBOffset);
      NewHashBlock->Id = REG_HASH_TABLE_CELL_ID;
      NewHashBlock->HashTableSize = (USHORT)SubKeyCount;
      *HashBlock = NewHashBlock;
    }

  return Status;
}


PCM_KEY_NODE
CmiGetKeyFromHashByIndex(PEREGISTRY_HIVE RegistryHive,
			 PHASH_TABLE_CELL HashBlock,
			 ULONG Index)
{
  HCELL_INDEX KeyOffset;
  PCM_KEY_NODE KeyCell;

  KeyOffset =  HashBlock->Table[Index].KeyOffset;
  KeyCell = HvGetCell (&RegistryHive->Hive, KeyOffset);

  return KeyCell;
}


NTSTATUS
CmiAddKeyToHashTable(PEREGISTRY_HIVE RegistryHive,
		     PHASH_TABLE_CELL HashCell,
		     PCM_KEY_NODE KeyCell,
		     HV_STORAGE_TYPE StorageType,
		     PCM_KEY_NODE NewKeyCell,
		     HCELL_INDEX NKBOffset)
{
  ULONG i = KeyCell->SubKeyCounts[StorageType];

  HashCell->Table[i].KeyOffset = NKBOffset;
  HashCell->Table[i].HashValue = 0;
  if (NewKeyCell->Flags & REG_KEY_NAME_PACKED)
    {
      RtlCopyMemory(&HashCell->Table[i].HashValue,
                    NewKeyCell->Name,
                    min(NewKeyCell->NameSize, sizeof(ULONG)));
    }
  HvMarkCellDirty(&RegistryHive->Hive, KeyCell->SubKeyLists[StorageType]);
  return STATUS_SUCCESS;
}


NTSTATUS
CmiRemoveKeyFromHashTable(PEREGISTRY_HIVE RegistryHive,
			  PHASH_TABLE_CELL HashBlock,
			  HCELL_INDEX NKBOffset)
{
  ULONG i;

  for (i = 0; i < HashBlock->HashTableSize; i++)
    {
      if (HashBlock->Table[i].KeyOffset == NKBOffset)
	{
	  RtlMoveMemory(HashBlock->Table + i,
	                HashBlock->Table + i + 1,
	                (HashBlock->HashTableSize - i - 1) *
	                sizeof(HashBlock->Table[0]));
	  return STATUS_SUCCESS;
	}
    }

  return STATUS_UNSUCCESSFUL;
}


NTSTATUS
CmiAllocateValueCell(PEREGISTRY_HIVE RegistryHive,
		     PCM_KEY_VALUE *ValueCell,
		     HCELL_INDEX *VBOffset,
		     IN PUNICODE_STRING ValueName,
		     IN HV_STORAGE_TYPE Storage)
{
  PCM_KEY_VALUE NewValueCell;
  NTSTATUS Status;
  BOOLEAN Packable;
  ULONG NameSize;
  ULONG i;

  Status = STATUS_SUCCESS;

  NameSize = CmiGetPackedNameLength(ValueName,
				    &Packable);

  DPRINT("ValueName->Length %lu  NameSize %lu\n", ValueName->Length, NameSize);

  *VBOffset = HvAllocateCell (&RegistryHive->Hive, sizeof(CM_KEY_VALUE) + NameSize, Storage);
  if (*VBOffset == HCELL_NULL)
    {
      Status = STATUS_INSUFFICIENT_RESOURCES;
    }
  else
    {
      ASSERT(NameSize <= 0xffff); /* should really be USHORT_MAX or similar */
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
	  RtlCopyMemory(NewValueCell->Name,
			ValueName->Buffer,
			NameSize);
	  NewValueCell->Flags = 0;
	}
      NewValueCell->DataType = 0;
      NewValueCell->DataSize = 0;
      NewValueCell->DataOffset = (HCELL_INDEX)-1;
      *ValueCell = NewValueCell;
    }

  return Status;
}


NTSTATUS
CmiDestroyValueCell(PEREGISTRY_HIVE RegistryHive,
		    PCM_KEY_VALUE ValueCell,
		    HCELL_INDEX ValueCellOffset)
{
  DPRINT("CmiDestroyValueCell(Cell %p  Offset %lx)\n",
	 ValueCell, ValueCellOffset);

  VERIFY_VALUE_CELL(ValueCell);

  /* Destroy the data cell */
  if (!(ValueCell->DataSize & REG_DATA_IN_OFFSET)
      && ValueCell->DataSize > sizeof(HCELL_INDEX)
      && ValueCell->DataOffset != HCELL_NULL)
    {
      HvFreeCell (&RegistryHive->Hive, ValueCell->DataOffset);
    }

  /* Destroy the value cell */
  HvFreeCell (&RegistryHive->Hive, ValueCellOffset);

  return STATUS_SUCCESS;
}


ULONG
CmiGetPackedNameLength(IN PUNICODE_STRING Name,
		       OUT PBOOLEAN Packable)
{
  ULONG i;

  if (Packable != NULL)
    *Packable = TRUE;

  for (i = 0; i < Name->Length / sizeof(WCHAR); i++)
    {
      if (Name->Buffer[i] & 0xFF00)
	{
	  if (Packable != NULL)
	    *Packable = FALSE;
	  return Name->Length;
	}
    }

  return (Name->Length / sizeof(WCHAR));
}


BOOLEAN
CmiComparePackedNames(IN PUNICODE_STRING Name,
		      IN PUCHAR NameBuffer,
		      IN USHORT NameBufferSize,
		      IN BOOLEAN NamePacked)
{
  PWCHAR UNameBuffer;
  ULONG i;

  if (NamePacked == TRUE)
    {
      if (Name->Length != NameBufferSize * sizeof(WCHAR))
	return(FALSE);

      for (i = 0; i < Name->Length / sizeof(WCHAR); i++)
	{
	  if (RtlUpcaseUnicodeChar(Name->Buffer[i]) != RtlUpcaseUnicodeChar((WCHAR)NameBuffer[i]))
	    return(FALSE);
	}
    }
  else
    {
      if (Name->Length != NameBufferSize)
	return(FALSE);

      UNameBuffer = (PWCHAR)NameBuffer;

      for (i = 0; i < Name->Length / sizeof(WCHAR); i++)
	{
	  if (RtlUpcaseUnicodeChar(Name->Buffer[i]) != RtlUpcaseUnicodeChar(UNameBuffer[i]))
	    return(FALSE);
	}
    }

  return(TRUE);
}


VOID
CmiCopyPackedName(PWCHAR NameBuffer,
		  PUCHAR PackedNameBuffer,
		  ULONG PackedNameSize)
{
  ULONG i;

  for (i = 0; i < PackedNameSize; i++)
    NameBuffer[i] = (WCHAR)PackedNameBuffer[i];
}


BOOLEAN
CmiCompareHash(PUNICODE_STRING KeyName,
	       PCHAR HashString)
{
  CHAR Buffer[4];

  Buffer[0] = (KeyName->Length >= 2) ? (CHAR)KeyName->Buffer[0] : 0;
  Buffer[1] = (KeyName->Length >= 4) ? (CHAR)KeyName->Buffer[1] : 0;
  Buffer[2] = (KeyName->Length >= 6) ? (CHAR)KeyName->Buffer[2] : 0;
  Buffer[3] = (KeyName->Length >= 8) ? (CHAR)KeyName->Buffer[3] : 0;

  return (strncmp(Buffer, HashString, 4) == 0);
}


BOOLEAN
CmiCompareHashI(PUNICODE_STRING KeyName,
	        PCHAR HashString)
{
  CHAR Buffer[4];

  Buffer[0] = (KeyName->Length >= 2) ? (CHAR)KeyName->Buffer[0] : 0;
  Buffer[1] = (KeyName->Length >= 4) ? (CHAR)KeyName->Buffer[1] : 0;
  Buffer[2] = (KeyName->Length >= 6) ? (CHAR)KeyName->Buffer[2] : 0;
  Buffer[3] = (KeyName->Length >= 8) ? (CHAR)KeyName->Buffer[3] : 0;

  return (_strnicmp(Buffer, HashString, 4) == 0);
}


BOOLEAN
CmiCompareKeyNames(PUNICODE_STRING KeyName,
		   PCM_KEY_NODE KeyCell)
{
  PWCHAR UnicodeName;
  USHORT i;

  DPRINT("Flags: %hx\n", KeyCell->Flags);

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


BOOLEAN
CmiCompareKeyNamesI(PUNICODE_STRING KeyName,
		    PCM_KEY_NODE KeyCell)
{
  PWCHAR UnicodeName;
  USHORT i;

  DPRINT("Flags: %hx\n", KeyCell->Flags);

  if (KeyCell->Flags & REG_KEY_NAME_PACKED)
    {
      if (KeyName->Length != KeyCell->NameSize * sizeof(WCHAR))
	return FALSE;

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
CmiSaveTempHive (PEREGISTRY_HIVE Hive,
		 HANDLE FileHandle)
{
  Hive->HiveHandle = FileHandle;
  return HvWriteHive(&Hive->Hive) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

/* EOF */
