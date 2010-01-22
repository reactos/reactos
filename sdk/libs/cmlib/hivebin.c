/*
 * PROJECT:   registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2005 Hartmut Birr
 *            Copyright 2001 - 2005 Eric Kohl
 */

#include "cmlib.h"

PHBIN CMAPI
HvpAddBin(
   PHHIVE RegistryHive,
   ULONG Size,
   HSTORAGE_TYPE Storage)
{
   PHMAP_ENTRY BlockList;
   PHBIN Bin;
   SIZE_T BinSize;
   ULONG i;
   ULONG BitmapSize;
   ULONG BlockCount;
   ULONG OldBlockListSize;
   PHCELL Block;

   BinSize = ROUND_UP(Size + sizeof(HBIN), HV_BLOCK_SIZE);
   BlockCount = (ULONG)(BinSize / HV_BLOCK_SIZE);

   Bin = RegistryHive->Allocate(BinSize, TRUE, TAG_CM);
   if (Bin == NULL)
      return NULL;
   RtlZeroMemory(Bin, BinSize);

   Bin->Signature = HV_BIN_SIGNATURE;
   Bin->FileOffset = RegistryHive->Storage[Storage].Length *
                    HV_BLOCK_SIZE;
   Bin->Size = (ULONG)BinSize;

   /* Allocate new block list */
   OldBlockListSize = RegistryHive->Storage[Storage].Length;
   BlockList = RegistryHive->Allocate(sizeof(HMAP_ENTRY) *
                                      (OldBlockListSize + BlockCount),
                                      TRUE,
                                      TAG_CM);
   if (BlockList == NULL)
   {
      RegistryHive->Free(Bin, 0);
      return NULL;
   }

   if (OldBlockListSize > 0)
   {
      RtlCopyMemory(BlockList, RegistryHive->Storage[Storage].BlockList,
                    OldBlockListSize * sizeof(HMAP_ENTRY));
      RegistryHive->Free(RegistryHive->Storage[Storage].BlockList, 0);
   }

   RegistryHive->Storage[Storage].BlockList = BlockList;
   RegistryHive->Storage[Storage].Length += BlockCount;

   for (i = 0; i < BlockCount; i++)
   {
      RegistryHive->Storage[Storage].BlockList[OldBlockListSize + i].BlockAddress =
         ((ULONG_PTR)Bin + (i * HV_BLOCK_SIZE));
      RegistryHive->Storage[Storage].BlockList[OldBlockListSize + i].BinAddress = (ULONG_PTR)Bin;
   }

   /* Initialize a free block in this heap. */
   Block = (PHCELL)(Bin + 1);
   Block->Size = (LONG)(BinSize - sizeof(HBIN));

   if (Storage == Stable)
   {
      /* Calculate bitmap size in bytes (always a multiple of 32 bits). */
      BitmapSize = ROUND_UP(RegistryHive->Storage[Stable].Length,
                            sizeof(ULONG) * 8) / 8;

      /* Grow bitmap if necessary. */
      if (BitmapSize > RegistryHive->DirtyVector.SizeOfBitMap / 8)
      {
         PULONG BitmapBuffer;

         BitmapBuffer = RegistryHive->Allocate(BitmapSize, TRUE, TAG_CM);
         RtlZeroMemory(BitmapBuffer, BitmapSize);
         if (RegistryHive->DirtyVector.SizeOfBitMap > 0)
         {
            ASSERT(RegistryHive->DirtyVector.Buffer);
            RtlCopyMemory(BitmapBuffer,
   		       RegistryHive->DirtyVector.Buffer,
   		       RegistryHive->DirtyVector.SizeOfBitMap / 8);
            RegistryHive->Free(RegistryHive->DirtyVector.Buffer, 0);
         }
         RtlInitializeBitMap(&RegistryHive->DirtyVector, BitmapBuffer,
                             BitmapSize * 8);
      }

      /* Mark new bin dirty. */
      RtlSetBits(&RegistryHive->DirtyVector,
                 Bin->FileOffset / HV_BLOCK_SIZE,
                 BlockCount);
   }

   return Bin;
}
