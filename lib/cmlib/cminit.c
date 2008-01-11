/*
 * PROJECT:   registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#include "cmlib.h"

ULONG CmlibTraceLevel = 0;

BOOLEAN CMAPI
CmCreateRootNode(
   PHHIVE Hive,
   PCWSTR Name)
{
   PCM_KEY_NODE KeyCell;
   HCELL_INDEX RootCellIndex;
   SIZE_T NameSize;

   /* Allocate the cell */
   NameSize = wcslen(Name) * sizeof(WCHAR);
   RootCellIndex = HvAllocateCell(Hive,
                                  FIELD_OFFSET(CM_KEY_NODE, Name) + NameSize,
                                  Stable,
                                  HCELL_NIL);
   if (RootCellIndex == HCELL_NIL) return FALSE;

   /* Seutp the base block */
   Hive->BaseBlock->RootCell = RootCellIndex;
   Hive->BaseBlock->CheckSum = HvpHiveHeaderChecksum(Hive->BaseBlock);

   /* Get the key cell */
   KeyCell = (PCM_KEY_NODE)HvGetCell(Hive, RootCellIndex);
   if (!KeyCell) return FALSE;

   /* Setup the cell */
   KeyCell->Signature = (USHORT)CM_KEY_NODE_SIGNATURE;
   KeyCell->Flags = KEY_HIVE_ENTRY | KEY_NO_DELETE;
   KeyCell->LastWriteTime.QuadPart = 0;
   KeyCell->Parent = HCELL_NIL;
   KeyCell->SubKeyCounts[Stable] = 0;
   KeyCell->SubKeyCounts[Volatile] = 0;
   KeyCell->SubKeyLists[Stable] = HCELL_NIL;
   KeyCell->SubKeyLists[Volatile] = HCELL_NIL;
   KeyCell->ValueList.Count = 0;
   KeyCell->ValueList.List = HCELL_NIL;
   KeyCell->Security = HCELL_NIL;
   KeyCell->Class = HCELL_NIL;
   KeyCell->ClassLength = 0;
   KeyCell->MaxNameLen = 0;
   KeyCell->MaxClassLen = 0;
   KeyCell->MaxValueNameLen = 0;
   KeyCell->MaxValueDataLen = 0;
   
   /* Write the name */
   KeyCell->NameLength = (USHORT)NameSize;
   memcpy(KeyCell->Name, Name, NameSize);

   /* Return success */
   HvReleaseCell(Hive, RootCellIndex);
   return TRUE;
}

static VOID CMAPI
CmpPrepareKey(
   PHHIVE RegistryHive,
   PCM_KEY_NODE KeyCell)
{
   PCM_KEY_NODE SubKeyCell;
   PCM_KEY_FAST_INDEX HashCell;
   ULONG i;

   ASSERT(KeyCell->Signature == CM_KEY_NODE_SIGNATURE);

   KeyCell->SubKeyLists[Volatile] = HCELL_NIL;
   KeyCell->SubKeyCounts[Volatile] = 0;

   /* Enumerate and add subkeys */
   if (KeyCell->SubKeyCounts[Stable] > 0)
   {
      HashCell = HvGetCell(RegistryHive, KeyCell->SubKeyLists[Stable]);

      for (i = 0; i < KeyCell->SubKeyCounts[Stable]; i++)
      {
         SubKeyCell = HvGetCell(RegistryHive, HashCell->List[i].Cell);
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
