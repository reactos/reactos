/*
 * PROJECT:   registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#include "cmlib.h"

BOOLEAN CMAPI
CmCreateRootNode(
   PHHIVE Hive,
   PCWSTR Name)
{
   PKEY_CELL KeyCell;
   HCELL_INDEX RootCellIndex;
   ULONG NameSize;

   NameSize = wcslen(Name) * sizeof(WCHAR);
   RootCellIndex = HvAllocateCell(Hive, sizeof(KEY_CELL) + NameSize, HvStable);
   if (RootCellIndex == HCELL_NULL)
      return FALSE;

   Hive->HiveHeader->RootCell = RootCellIndex;
   Hive->HiveHeader->Checksum = HvpHiveHeaderChecksum(Hive->HiveHeader);

   KeyCell = (PKEY_CELL)HvGetCell(Hive, RootCellIndex);
   KeyCell->Id = REG_KEY_CELL_ID;
   KeyCell->Flags = REG_KEY_ROOT_CELL;
   KeyCell->LastWriteTime.QuadPart = 0;
   KeyCell->ParentKeyOffset = HCELL_NULL;
   KeyCell->NumberOfSubKeys[0] = 0;
   KeyCell->NumberOfSubKeys[1] = 0;
   KeyCell->HashTableOffset[0] = HCELL_NULL;
   KeyCell->HashTableOffset[1] = HCELL_NULL;
   KeyCell->NumberOfValues = 0;
   KeyCell->ValueListOffset = HCELL_NULL;
   KeyCell->SecurityKeyOffset = HCELL_NULL;
   KeyCell->ClassNameOffset = HCELL_NULL; 
   KeyCell->NameSize = NameSize;
   KeyCell->ClassSize = 0;
   memcpy(KeyCell->Name, Name, NameSize);

   return TRUE;
}

static VOID CMAPI
CmpPrepareKey(
   PHHIVE RegistryHive,
   PKEY_CELL KeyCell)
{
   PKEY_CELL SubKeyCell;
   PHASH_TABLE_CELL HashCell;
   ULONG i;

   ASSERT(KeyCell->Id == REG_KEY_CELL_ID);

   KeyCell->HashTableOffset[HvVolatile] = HCELL_NULL;
   KeyCell->NumberOfSubKeys[HvVolatile] = 0;

   /* Enumerate and add subkeys */
   if (KeyCell->NumberOfSubKeys[HvStable] > 0)
   {
      HashCell = HvGetCell(RegistryHive, KeyCell->HashTableOffset[HvStable]);

      for (i = 0; i < KeyCell->NumberOfSubKeys[HvStable]; i++)
      {
         SubKeyCell = HvGetCell(RegistryHive, HashCell->Table[i].KeyOffset);
         CmpPrepareKey(RegistryHive, SubKeyCell);
      }
   }
}

VOID CMAPI
CmPrepareHive(
   PHHIVE RegistryHive)
{ 
   PKEY_CELL RootCell;

   RootCell = HvGetCell(RegistryHive, RegistryHive->HiveHeader->RootCell);
   CmpPrepareKey(RegistryHive, RootCell);
}
