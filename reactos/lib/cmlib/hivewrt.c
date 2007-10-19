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
   ULONGLONG FileOffset;
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

   if (RegistryHive->HiveHeader->Sequence1 !=
       RegistryHive->HiveHeader->Sequence2)
   {
      return FALSE;
   }

   BitmapSize = RegistryHive->DirtyVector.SizeOfBitMap;
   BufferSize = HV_LOG_HEADER_SIZE + sizeof(ULONG) + BitmapSize;
   BufferSize = ROUND_UP(BufferSize, HV_BLOCK_SIZE);

   DPRINT("Bitmap size %lu  buffer size: %lu\n", BitmapSize, BufferSize);

   Buffer = RegistryHive->Allocate(BufferSize, TRUE);
   if (Buffer == NULL)
   {
      return FALSE;
   }

   /* Update first update counter and checksum */
   RegistryHive->HiveHeader->Type = HV_TYPE_LOG;
   RegistryHive->HiveHeader->Sequence1++;
   RegistryHive->HiveHeader->Checksum =
      HvpHiveHeaderChecksum(RegistryHive->HiveHeader);

   /* Copy hive header */
   RtlCopyMemory(Buffer, RegistryHive->HiveHeader, HV_LOG_HEADER_SIZE);
   Ptr = Buffer + HV_LOG_HEADER_SIZE;
   RtlCopyMemory(Ptr, "DIRT", 4);
   Ptr += 4;
   RtlCopyMemory(Ptr, RegistryHive->DirtyVector.Buffer, BitmapSize);

   /* Write hive block and block bitmap */
   Success = RegistryHive->FileWrite(RegistryHive, HV_TYPE_LOG,
                                     0, Buffer, BufferSize);
   RegistryHive->Free(Buffer);

   if (!Success)
   {
      return FALSE;
   }

   /* Write dirty blocks */
   FileOffset = BufferSize;
   BlockIndex = 0;
   while (BlockIndex < RegistryHive->Storage[HvStable].Length)
   {
      LastIndex = BlockIndex;
      BlockIndex = RtlFindSetBits(&RegistryHive->DirtyVector, 1, BlockIndex);
      if (BlockIndex == ~0 || BlockIndex < LastIndex)
      {
         break;
      }

      BlockPtr = (PVOID)RegistryHive->Storage[HvStable].BlockList[BlockIndex].Block;

      /* Write hive block */
      Success = RegistryHive->FileWrite(RegistryHive, HV_TYPE_LOG,
                                        FileOffset, BlockPtr,
                                        HV_BLOCK_SIZE);
      if (!Success)
      {
         return FALSE;
      }

      BlockIndex++;
      FileOffset += HV_BLOCK_SIZE;
    }

   Success = RegistryHive->FileSetSize(RegistryHive, HV_TYPE_LOG, FileOffset);
   if (!Success)
   {
      DPRINT("FileSetSize failed\n");
      return FALSE;
    }

   /* Flush the log file */
   Success = RegistryHive->FileFlush(RegistryHive, HV_TYPE_LOG);
   if (!Success)
   {
      DPRINT("FileFlush failed\n");
   }

   /* Update first and second update counter and checksum. */
   RegistryHive->HiveHeader->Sequence2++;
   RegistryHive->HiveHeader->Checksum =
      HvpHiveHeaderChecksum(RegistryHive->HiveHeader);

   /* Write hive header again with updated sequence counter. */
   Success = RegistryHive->FileWrite(RegistryHive, HV_TYPE_LOG,
                                     0, RegistryHive->HiveHeader,
                                     HV_LOG_HEADER_SIZE);
   if (!Success)
   {
      return FALSE;
   }

   /* Flush the log file */
   Success = RegistryHive->FileFlush(RegistryHive, HV_TYPE_LOG);
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
   ULONGLONG FileOffset;
   ULONG BlockIndex;
   ULONG LastIndex;
   PVOID BlockPtr;
   BOOLEAN Success;

   ASSERT(RegistryHive->ReadOnly == FALSE);

   DPRINT("HvpWriteHive called\n");

   if (RegistryHive->HiveHeader->Sequence1 !=
       RegistryHive->HiveHeader->Sequence2)
   {
      return FALSE;
   }

   /* Update first update counter and checksum */
   RegistryHive->HiveHeader->Type = HV_TYPE_PRIMARY;
   RegistryHive->HiveHeader->Sequence1++;
   RegistryHive->HiveHeader->Checksum =
      HvpHiveHeaderChecksum(RegistryHive->HiveHeader);

   /* Write hive block */
   Success = RegistryHive->FileWrite(RegistryHive, HV_TYPE_PRIMARY,
                                     0, RegistryHive->HiveHeader,
                                     sizeof(HBASE_BLOCK));
   if (!Success)
   {
      return FALSE;
   }

   BlockIndex = 0;
   while (BlockIndex < RegistryHive->Storage[HvStable].Length)
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

      BlockPtr = (PVOID)RegistryHive->Storage[HvStable].BlockList[BlockIndex].Block;
      FileOffset = (ULONGLONG)(BlockIndex + 1) * (ULONGLONG)HV_BLOCK_SIZE;

      /* Write hive block */
      Success = RegistryHive->FileWrite(RegistryHive, HV_TYPE_PRIMARY,
                                        FileOffset, BlockPtr,
                                        HV_BLOCK_SIZE);
      if (!Success)
      {
         return FALSE;
      }

      BlockIndex++;
   }

   Success = RegistryHive->FileFlush(RegistryHive, HV_TYPE_PRIMARY);
   if (!Success)
   {
      DPRINT("FileFlush failed\n");
   }

   /* Update second update counter and checksum */
   RegistryHive->HiveHeader->Sequence2++;
   RegistryHive->HiveHeader->Checksum =
      HvpHiveHeaderChecksum(RegistryHive->HiveHeader);

   /* Write hive block */
   Success = RegistryHive->FileWrite(RegistryHive, HV_TYPE_PRIMARY,
                                     0, RegistryHive->HiveHeader,
                                     sizeof(HBASE_BLOCK));
   if (!Success)
   {
      return FALSE;
   }

   Success = RegistryHive->FileFlush(RegistryHive, HV_TYPE_PRIMARY);
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
   KeQuerySystemTime(&RegistryHive->HiveHeader->TimeStamp);

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
   KeQuerySystemTime(&RegistryHive->HiveHeader->TimeStamp);

   /* Update hive file */
   if (!HvpWriteHive(RegistryHive, FALSE))
   {
      return FALSE;
   }

   return TRUE;
}
