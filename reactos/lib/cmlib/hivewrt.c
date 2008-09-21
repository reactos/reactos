/*
 * PROJECT:   registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#include "cmlib.h"
#define NDEBUG
#include <debug.h>

static BOOLEAN CMAPI
HvpWriteLog(
   PHHIVE RegistryHive)
{
   ULONG FileOffset;
   SIZE_T BufferSize;
   SIZE_T BitmapSize;
   PUCHAR Buffer;
   PUCHAR Ptr;
   ULONG BlockIndex;
   ULONG LastIndex;
   PVOID BlockPtr;
   BOOLEAN Success;

   DPRINT1("FIXME: HvpWriteLog doesn't do anything atm\n");
   return TRUE;

   ASSERT(RegistryHive->ReadOnly == FALSE);

   DPRINT("HvpWriteLog called\n");

   if (RegistryHive->BaseBlock->Sequence1 !=
       RegistryHive->BaseBlock->Sequence2)
   {
      return FALSE;
   }

   BitmapSize = RegistryHive->DirtyVector.SizeOfBitMap;
   BufferSize = HV_LOG_HEADER_SIZE + sizeof(ULONG) + BitmapSize;
   BufferSize = ROUND_UP(BufferSize, HV_BLOCK_SIZE);

   DPRINT("Bitmap size %lu  buffer size: %lu\n", BitmapSize, BufferSize);

   Buffer = RegistryHive->Allocate(BufferSize, TRUE, TAG_CM);
   if (Buffer == NULL)
   {
      return FALSE;
   }

   /* Update first update counter and CheckSum */
   RegistryHive->BaseBlock->Type = HFILE_TYPE_LOG;
   RegistryHive->BaseBlock->Sequence1++;
   RegistryHive->BaseBlock->CheckSum =
      HvpHiveHeaderChecksum(RegistryHive->BaseBlock);

   /* Copy hive header */
   RtlCopyMemory(Buffer, RegistryHive->BaseBlock, HV_LOG_HEADER_SIZE);
   Ptr = Buffer + HV_LOG_HEADER_SIZE;
   RtlCopyMemory(Ptr, "DIRT", 4);
   Ptr += 4;
   RtlCopyMemory(Ptr, RegistryHive->DirtyVector.Buffer, BitmapSize);

   /* Write hive block and block bitmap */
   FileOffset = 0;
   Success = RegistryHive->FileWrite(RegistryHive, HFILE_TYPE_LOG,
                                     &FileOffset, Buffer, BufferSize);
   RegistryHive->Free(Buffer, 0);

   if (!Success)
   {
      return FALSE;
   }

   /* Write dirty blocks */
   FileOffset = BufferSize;
   BlockIndex = 0;
   while (BlockIndex < RegistryHive->Storage[Stable].Length)
   {
      LastIndex = BlockIndex;
      BlockIndex = RtlFindSetBits(&RegistryHive->DirtyVector, 1, BlockIndex);
      if (BlockIndex == ~0 || BlockIndex < LastIndex)
      {
         break;
      }

      BlockPtr = (PVOID)RegistryHive->Storage[Stable].BlockList[BlockIndex].BlockAddress;

      /* Write hive block */
      Success = RegistryHive->FileWrite(RegistryHive, HFILE_TYPE_LOG,
                                        &FileOffset, BlockPtr,
                                        HV_BLOCK_SIZE);
      if (!Success)
      {
         return FALSE;
      }

      BlockIndex++;
      FileOffset += HV_BLOCK_SIZE;
    }

   Success = RegistryHive->FileSetSize(RegistryHive, HFILE_TYPE_LOG, FileOffset, FileOffset);
   if (!Success)
   {
      DPRINT("FileSetSize failed\n");
      return FALSE;
    }

   /* Flush the log file */
   Success = RegistryHive->FileFlush(RegistryHive, HFILE_TYPE_LOG, NULL, 0);
   if (!Success)
   {
      DPRINT("FileFlush failed\n");
   }

   /* Update first and second update counter and CheckSum. */
   RegistryHive->BaseBlock->Sequence2++;
   RegistryHive->BaseBlock->CheckSum =
      HvpHiveHeaderChecksum(RegistryHive->BaseBlock);

   /* Write hive header again with updated sequence counter. */
   FileOffset = 0;
   Success = RegistryHive->FileWrite(RegistryHive, HFILE_TYPE_LOG,
                                     &FileOffset, RegistryHive->BaseBlock,
                                     HV_LOG_HEADER_SIZE);
   if (!Success)
   {
      return FALSE;
   }

   /* Flush the log file */
   Success = RegistryHive->FileFlush(RegistryHive, HFILE_TYPE_LOG, NULL, 0);
   if (!Success)
   {
      DPRINT("FileFlush failed\n");
   }

   return TRUE;
}

static BOOLEAN CMAPI
HvpWriteHive(
   PHHIVE RegistryHive,
   BOOLEAN OnlyDirty)
{
   ULONG FileOffset;
   ULONG BlockIndex;
   ULONG LastIndex;
   PVOID BlockPtr;
   BOOLEAN Success;

   ASSERT(RegistryHive->ReadOnly == FALSE);

   DPRINT("HvpWriteHive called\n");

   if (RegistryHive->BaseBlock->Sequence1 !=
       RegistryHive->BaseBlock->Sequence2)
   {
      return FALSE;
   }

   /* Update first update counter and CheckSum */
   RegistryHive->BaseBlock->Type = HFILE_TYPE_PRIMARY;
   RegistryHive->BaseBlock->Sequence1++;
   RegistryHive->BaseBlock->CheckSum =
   HvpHiveHeaderChecksum(RegistryHive->BaseBlock);

   /* Write hive block */
   FileOffset = 0;
   Success = RegistryHive->FileWrite(RegistryHive, HFILE_TYPE_PRIMARY,
                                     &FileOffset, RegistryHive->BaseBlock,
                                     sizeof(HBASE_BLOCK));
   if (!Success)
   {
      return FALSE;
   }

   BlockIndex = 0;
   while (BlockIndex < RegistryHive->Storage[Stable].Length)
   {
      if (OnlyDirty)
      {
         LastIndex = BlockIndex;
         BlockIndex = RtlFindSetBits(&RegistryHive->DirtyVector, 1, BlockIndex);
         if (BlockIndex == ~0 || BlockIndex < LastIndex)
         {
            break;
         }
      }

      BlockPtr = (PVOID)RegistryHive->Storage[Stable].BlockList[BlockIndex].BlockAddress;
      FileOffset = (BlockIndex + 1) * HV_BLOCK_SIZE;

      /* Write hive block */
      Success = RegistryHive->FileWrite(RegistryHive, HFILE_TYPE_PRIMARY,
                                        &FileOffset, BlockPtr,
                                        HV_BLOCK_SIZE);
      if (!Success)
      {
         return FALSE;
      }

      BlockIndex++;
   }

   Success = RegistryHive->FileFlush(RegistryHive, HFILE_TYPE_PRIMARY, NULL, 0);
   if (!Success)
   {
      DPRINT("FileFlush failed\n");
   }

   /* Update second update counter and CheckSum */
   RegistryHive->BaseBlock->Sequence2++;
   RegistryHive->BaseBlock->CheckSum =
      HvpHiveHeaderChecksum(RegistryHive->BaseBlock);

   /* Write hive block */
   FileOffset = 0;
   Success = RegistryHive->FileWrite(RegistryHive, HFILE_TYPE_PRIMARY,
                                     &FileOffset, RegistryHive->BaseBlock,
                                     sizeof(HBASE_BLOCK));
   if (!Success)
   {
      return FALSE;
   }

   Success = RegistryHive->FileFlush(RegistryHive, HFILE_TYPE_PRIMARY, NULL, 0);
   if (!Success)
   {
      DPRINT("FileFlush failed\n");
   }

   return TRUE;
}

BOOLEAN CMAPI
HvSyncHive(
   PHHIVE RegistryHive)
{
   ASSERT(RegistryHive->ReadOnly == FALSE);

   if (RtlFindSetBits(&RegistryHive->DirtyVector, 1, 0) == ~0)
   {
      return TRUE;
   }

   /* Update hive header modification time */
   KeQuerySystemTime(&RegistryHive->BaseBlock->TimeStamp);

   /* Update log file */
   if (!HvpWriteLog(RegistryHive))
   {
      return FALSE;
   }

   /* Update hive file */
   if (!HvpWriteHive(RegistryHive, TRUE))
   {
      return FALSE;
   }

   /* Clear dirty bitmap. */
   RtlClearAllBits(&RegistryHive->DirtyVector);

   return TRUE;
}

BOOLEAN CMAPI
HvWriteHive(
   PHHIVE RegistryHive)
{
   ASSERT(RegistryHive->ReadOnly == FALSE);

   /* Update hive header modification time */
   KeQuerySystemTime(&RegistryHive->BaseBlock->TimeStamp);

   /* Update hive file */
   if (!HvpWriteHive(RegistryHive, FALSE))
   {
      return FALSE;
   }

   return TRUE;
}
