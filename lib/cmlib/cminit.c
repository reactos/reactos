/*
 * PROJECT:   registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#include "cmlib.h"
#define NDEBUG
#include <debug.h>

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
   NameSize = utf16_wcslen(Name) * sizeof(WCHAR);
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
   PCM_KEY_NODE KeyCell);

static VOID CMAPI
CmpPrepareIndexOfKeys(
   PHHIVE RegistryHive,
   PCM_KEY_INDEX IndexCell)
{
   ULONG i;

   if (IndexCell->Signature == CM_KEY_INDEX_ROOT ||
       IndexCell->Signature == CM_KEY_INDEX_LEAF)
   {
      for (i = 0; i < IndexCell->Count; i++)
      {
         PCM_KEY_INDEX SubIndexCell = HvGetCell(RegistryHive, IndexCell->List[i]);
         CmpPrepareIndexOfKeys(RegistryHive, SubIndexCell);
      }
   }
   else if (IndexCell->Signature == CM_KEY_FAST_LEAF ||
            IndexCell->Signature == CM_KEY_HASH_LEAF)
   {
      PCM_KEY_FAST_INDEX HashCell = (PCM_KEY_FAST_INDEX)IndexCell;
      for (i = 0; i < HashCell->Count; i++)
      {
         PCM_KEY_NODE SubKeyCell = HvGetCell(RegistryHive, HashCell->List[i].Cell);
         CmpPrepareKey(RegistryHive, SubKeyCell);
      }
   }
   else
   {
      DPRINT1("IndexCell->Signature %x\n", IndexCell->Signature);
      ASSERT(FALSE);
   }
}

static VOID CMAPI
CmpPrepareKey(
   PHHIVE RegistryHive,
   PCM_KEY_NODE KeyCell)
{
   PCM_KEY_INDEX IndexCell;

   ASSERT(KeyCell->Signature == CM_KEY_NODE_SIGNATURE);

   KeyCell->SubKeyLists[Volatile] = HCELL_NIL;
   KeyCell->SubKeyCounts[Volatile] = 0;

   /* Enumerate and add subkeys */
   if (KeyCell->SubKeyCounts[Stable] > 0)
   {
      IndexCell = HvGetCell(RegistryHive, KeyCell->SubKeyLists[Stable]);
      CmpPrepareIndexOfKeys(RegistryHive, IndexCell);
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
