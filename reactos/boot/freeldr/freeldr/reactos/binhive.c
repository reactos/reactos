/*
 *  FreeLoader
 *
 *  Copyright (C) 2001  Rex Jolliff
 *  Copyright (C) 2001  Eric Kohl
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

#include <freeldr.h>
#include <cmlib.h>
#include <debug.h>

#define REG_DATA_SIZE_MASK                 0x7FFFFFFF
#define REG_DATA_IN_OFFSET                 0x80000000

/* FUNCTIONS ****************************************************************/

static PVOID
NTAPI
CmpAllocate (SIZE_T Size, BOOLEAN Paged, ULONG Tag)
{
  return MmAllocateMemory(Size);
}


static VOID
NTAPI
CmpFree (PVOID Ptr, IN ULONG Quota)
{
  return MmFreeMemory(Ptr);
}

static BOOLEAN
RegImportValue (PHHIVE Hive,
		PCM_KEY_VALUE ValueCell,
		FRLDRHKEY Key)
{
  PVOID DataCell;
  PWCHAR wName;
  LONG Error;
  ULONG DataLength;
  ULONG i;

  if (ValueCell->Signature != CM_KEY_VALUE_SIGNATURE)
    {
      DbgPrint((DPRINT_REGISTRY, "Invalid key cell!\n"));
      return FALSE;
    }

  if (ValueCell->Flags & VALUE_COMP_NAME)
    {
      wName = MmAllocateMemory ((ValueCell->NameLength + 1)*sizeof(WCHAR));
      for (i = 0; i < ValueCell->NameLength; i++)
        {
          wName[i] = ((PCHAR)ValueCell->Name)[i];
        }
      wName[ValueCell->NameLength] = 0;
    }
  else
    {
      wName = MmAllocateMemory (ValueCell->NameLength + sizeof(WCHAR));
      memcpy (wName,
	      ValueCell->Name,
	      ValueCell->NameLength);
      wName[ValueCell->NameLength / sizeof(WCHAR)] = 0;
    }

  DataLength = ValueCell->DataLength & REG_DATA_SIZE_MASK;

  DbgPrint((DPRINT_REGISTRY, "ValueName: '%S'\n", wName));
  DbgPrint((DPRINT_REGISTRY, "DataLength: %u\n", DataLength));

  if (DataLength <= sizeof(HCELL_INDEX) && (ValueCell->DataLength & REG_DATA_IN_OFFSET))
    {
      Error = RegSetValue(Key,
			  wName,
			  ValueCell->Type,
			  (PCHAR)&ValueCell->Data,
			  DataLength);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_REGISTRY, "RegSetValue() failed!\n"));
	  MmFreeMemory (wName);
	  return FALSE;
	}
    }
  else
    {
      DataCell = (PVOID)HvGetCell (Hive, ValueCell->Data);
      DbgPrint((DPRINT_REGISTRY, "DataCell: %x\n", DataCell));

      Error = RegSetValue (Key,
			   wName,
			   ValueCell->Type,
			   DataCell,
			   DataLength);

      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_REGISTRY, "RegSetValue() failed!\n"));
	  MmFreeMemory (wName);
	  return FALSE;
	}
    }

  MmFreeMemory (wName);

  return TRUE;
}


static BOOLEAN
RegImportSubKey(PHHIVE Hive,
		PCM_KEY_NODE KeyCell,
		FRLDRHKEY ParentKey)
{
  PCM_KEY_FAST_INDEX HashCell;
  PCM_KEY_NODE SubKeyCell;
  PVALUE_LIST_CELL ValueListCell;
  PCM_KEY_VALUE ValueCell = NULL;
  PWCHAR wName;
  FRLDRHKEY SubKey;
  LONG Error;
  ULONG i;


  DbgPrint((DPRINT_REGISTRY, "KeyCell: %x\n", KeyCell));
  DbgPrint((DPRINT_REGISTRY, "KeyCell->Signature: %x\n", KeyCell->Signature));
  if (KeyCell->Signature != CM_KEY_NODE_SIGNATURE)
    {
      DbgPrint((DPRINT_REGISTRY, "Invalid key cell Signature!\n"));
      return FALSE;
    }

  if (KeyCell->Flags & KEY_COMP_NAME)
    {
      wName = MmAllocateMemory ((KeyCell->NameLength + 1) * sizeof(WCHAR));
      for (i = 0; i < KeyCell->NameLength; i++)
        {
          wName[i] = ((PCHAR)KeyCell->Name)[i];
        }
      wName[KeyCell->NameLength] = 0;
    }
  else
    {
      wName = MmAllocateMemory (KeyCell->NameLength + sizeof(WCHAR));
      memcpy (wName,
	      KeyCell->Name,
	      KeyCell->NameLength);
      wName[KeyCell->NameLength/sizeof(WCHAR)] = 0;
    }

  DbgPrint((DPRINT_REGISTRY, "KeyName: '%S'\n", wName));

  /* Create new sub key */
  Error = RegCreateKey (ParentKey,
			wName,
			&SubKey);
  MmFreeMemory (wName);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REGISTRY, "RegCreateKey() failed!\n"));
      return FALSE;
    }
  DbgPrint((DPRINT_REGISTRY, "Subkeys: %u\n", KeyCell->SubKeyCounts));
  DbgPrint((DPRINT_REGISTRY, "Values: %u\n", KeyCell->ValueList.Count));

  /* Enumerate and add values */
  if (KeyCell->ValueList.Count > 0)
    {
      ValueListCell = (PVALUE_LIST_CELL) HvGetCell (Hive, KeyCell->ValueList.List);
      DbgPrint((DPRINT_REGISTRY, "ValueListCell: %x\n", ValueListCell));

      for (i = 0; i < KeyCell->ValueList.Count; i++)
	{
	  DbgPrint((DPRINT_REGISTRY, "ValueOffset[%d]: %x\n", i, ValueListCell->ValueOffset[i]));

	  ValueCell = (PCM_KEY_VALUE) HvGetCell (Hive, ValueListCell->ValueOffset[i]);

	  DbgPrint((DPRINT_REGISTRY, "ValueCell[%d]: %x\n", i, ValueCell));

	  if (!RegImportValue(Hive, ValueCell, SubKey))
	    return FALSE;
	}
    }

  /* Enumerate and add subkeys */
  if (KeyCell->SubKeyCounts[Stable] > 0)
    {
      HashCell = (PCM_KEY_FAST_INDEX) HvGetCell (Hive, KeyCell->SubKeyLists[Stable]);
      DbgPrint((DPRINT_REGISTRY, "HashCell: %x\n", HashCell));
      DbgPrint((DPRINT_REGISTRY, "SubKeyCounts: %x\n", KeyCell->SubKeyCounts));

      for (i = 0; i < KeyCell->SubKeyCounts[Stable]; i++)
	{
	  DbgPrint((DPRINT_REGISTRY, "Cell[%d]: %x\n", i, HashCell->List[i].Cell));

	  SubKeyCell = (PCM_KEY_NODE) HvGetCell (Hive, HashCell->List[i].Cell);

	  DbgPrint((DPRINT_REGISTRY, "SubKeyCell[%d]: %x\n", i, SubKeyCell));

	  if (!RegImportSubKey(Hive, SubKeyCell, SubKey))
	    return FALSE;
	}
    }

  return TRUE;
}


BOOLEAN
RegImportBinaryHive(PCHAR ChunkBase,
		    ULONG ChunkSize)
{
  PCM_KEY_NODE KeyCell;
  PCM_KEY_FAST_INDEX HashCell;
  PCM_KEY_NODE SubKeyCell;
  FRLDRHKEY SystemKey;
  ULONG i;
  LONG Error;
  PCMHIVE CmHive;
  PHHIVE Hive;
  NTSTATUS Status;

  DbgPrint((DPRINT_REGISTRY, "RegImportBinaryHive(%x, %u) called\n",ChunkBase,ChunkSize));

  CmHive = CmpAllocate(sizeof(CMHIVE), TRUE, 0);
  Status = HvInitialize (&CmHive->Hive,
                         HINIT_FLAT,
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
      DbgPrint((DPRINT_REGISTRY, "Invalid hive Signature!\n"));
      return FALSE;
    }

  Hive = &CmHive->Hive;
  KeyCell = (PCM_KEY_NODE)HvGetCell (Hive, Hive->BaseBlock->RootCell);
  DbgPrint((DPRINT_REGISTRY, "KeyCell: %x\n", KeyCell));
  DbgPrint((DPRINT_REGISTRY, "KeyCell->Signature: %x\n", KeyCell->Signature));
  if (KeyCell->Signature != CM_KEY_NODE_SIGNATURE)
    {
      DbgPrint((DPRINT_REGISTRY, "Invalid key cell Signature!\n"));
      return FALSE;
    }

  DbgPrint((DPRINT_REGISTRY, "Subkeys: %u\n", KeyCell->SubKeyCounts));
  DbgPrint((DPRINT_REGISTRY, "Values: %u\n", KeyCell->ValueList.Count));

  /* Open 'System' key */
  Error = RegOpenKey(NULL,
		     L"\\Registry\\Machine\\SYSTEM",
		     &SystemKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REGISTRY, "Failed to open 'system' key!\n"));
      return FALSE;
    }

  /* Enumerate and add subkeys */
  if (KeyCell->SubKeyCounts[Stable] > 0)
    {
      HashCell = (PCM_KEY_FAST_INDEX)HvGetCell (Hive, KeyCell->SubKeyLists[Stable]);
      DbgPrint((DPRINT_REGISTRY, "HashCell: %x\n", HashCell));
      DbgPrint((DPRINT_REGISTRY, "SubKeyCounts: %x\n", KeyCell->SubKeyCounts[Stable]));

      for (i = 0; i < KeyCell->SubKeyCounts[Stable]; i++)
	{
	  DbgPrint((DPRINT_REGISTRY, "Cell[%d]: %x\n", i, HashCell->List[i].Cell));

	  SubKeyCell = (PCM_KEY_NODE)HvGetCell (Hive, HashCell->List[i].Cell);

	  DbgPrint((DPRINT_REGISTRY, "SubKeyCell[%d]: %x\n", i, SubKeyCell));

	  if (!RegImportSubKey(Hive, SubKeyCell, SystemKey))
	    return FALSE;
	}
    }

  return TRUE;
}

/* EOF */
