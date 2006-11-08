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
   RootCellIndex = HvAllocateCell(Hive, sizeof(CM_KEY_NODE) + NameSize, HvStable);
   if (RootCellIndex == HCELL_NULL)
      return FALSE;

   Hive->HiveHeader->RootCell = RootCellIndex;
   Hive->HiveHeader->Checksum = HvpHiveHeaderChecksum(Hive->HiveHeader);

   KeyCell = (PCM_KEY_NODE)HvGetCell(Hive, RootCellIndex);
   KeyCell->Id = REG_KEY_CELL_ID;
   KeyCell->Flags = REG_KEY_ROOT_CELL;
   KeyCell->LastWriteTime.QuadPart = 0;
   KeyCell->Parent = HCELL_NULL;
   KeyCell->SubKeyCounts[0] = 0;
   KeyCell->SubKeyCounts[1] = 0;
   KeyCell->SubKeyLists[0] = HCELL_NULL;
   KeyCell->SubKeyLists[1] = HCELL_NULL;
   KeyCell->ValueList.Count = 0;
   KeyCell->ValueList.List = HCELL_NULL;
   KeyCell->SecurityKeyOffset = HCELL_NULL;
   KeyCell->ClassNameOffset = HCELL_NULL; 
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

   KeyCell->SubKeyLists[HvVolatile] = HCELL_NULL;
   KeyCell->SubKeyCounts[HvVolatile] = 0;

   /* Enumerate and add subkeys */
   if (KeyCell->SubKeyCounts[HvStable] > 0)
   {
      HashCell = HvGetCell(RegistryHive, KeyCell->SubKeyLists[HvStable]);

      for (i = 0; i < KeyCell->SubKeyCounts[HvStable]; i++)
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

   RootCell = HvGetCell(RegistryHive, RegistryHive->HiveHeader->RootCell);
   CmpPrepareKey(RegistryHive, RootCell);
}
