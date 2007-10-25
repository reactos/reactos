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
   PCM_KEY_NODE KeyCell;
   HCELL_INDEX RootCellIndex;
   SIZE_T NameSize;

   NameSize = wcslen(Name) * sizeof(WCHAR);
   RootCellIndex = HvAllocateCell(Hive,
                                  sizeof(CM_KEY_NODE) + NameSize,
                                  Stable,
                                  HCELL_NIL);
   if (RootCellIndex == HCELL_NIL)
      return FALSE;

   Hive->BaseBlock->RootCell = RootCellIndex;
   Hive->BaseBlock->CheckSum = HvpHiveHeaderChecksum(Hive->BaseBlock);

   KeyCell = (PCM_KEY_NODE)HvGetCell(Hive, RootCellIndex);
   KeyCell->Id = REG_KEY_CELL_ID;
   KeyCell->Flags = REG_KEY_ROOT_CELL;
   KeyCell->LastWriteTime.QuadPart = 0;
   KeyCell->Parent = HCELL_NIL;
   KeyCell->SubKeyCounts[0] = 0;
   KeyCell->SubKeyCounts[1] = 0;
   KeyCell->SubKeyLists[0] = HCELL_NIL;
   KeyCell->SubKeyLists[1] = HCELL_NIL;
   KeyCell->ValueList.Count = 0;
   KeyCell->ValueList.List = HCELL_NIL;
   KeyCell->SecurityKeyOffset = HCELL_NIL;
   KeyCell->ClassNameOffset = HCELL_NIL; 
   KeyCell->NameSize = (USHORT)NameSize;
   KeyCell->ClassSize = 0;
   memcpy(KeyCell->Name, Name, NameSize);

   return TRUE;
}

static VOID CMAPI
CmpPrepareKey(
   PHHIVE RegistryHive,
   PCM_KEY_NODE KeyCell)
{
   PCM_KEY_NODE SubKeyCell;
   PHASH_TABLE_CELL HashCell;
   ULONG i;

   ASSERT(KeyCell->Id == REG_KEY_CELL_ID);

   KeyCell->SubKeyLists[Volatile] = HCELL_NIL;
   KeyCell->SubKeyCounts[Volatile] = 0;

   /* Enumerate and add subkeys */
   if (KeyCell->SubKeyCounts[Stable] > 0)
   {
      HashCell = HvGetCell(RegistryHive, KeyCell->SubKeyLists[Stable]);

      for (i = 0; i < KeyCell->SubKeyCounts[Stable]; i++)
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
   PCM_KEY_NODE RootCell;

   RootCell = HvGetCell(RegistryHive, RegistryHive->BaseBlock->RootCell);
   CmpPrepareKey(RegistryHive, RootCell);
}
