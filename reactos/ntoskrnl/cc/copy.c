/* $Id: copy.c,v 1.2 2001/12/27 23:56:41 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/copy.c
 * PURPOSE:         Implements cache managers copy interface
 * PROGRAMMER:      Hartmut Birr
 * UPDATE HISTORY:
 *                  Created 05.10.2001
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <internal/mm.h>
#include <internal/cc.h>
#include <internal/pool.h>
#include <internal/io.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define ROUND_DOWN(N, S) ((N) - ((N) % (S)))

/* FUNCTIONS *****************************************************************/

NTSTATUS ReadCacheSegment(PCACHE_SEGMENT CacheSeg)
{
  ULONG Size;
  PMDL Mdl;
  NTSTATUS Status;
  LARGE_INTEGER SegOffset;
  IO_STATUS_BLOCK IoStatus;

  SegOffset.QuadPart = CacheSeg->FileOffset;
  Size = CacheSeg->Bcb->AllocationSize.QuadPart - CacheSeg->FileOffset;
  if (Size > CacheSeg->Bcb->CacheSegmentSize)
  {
    Size = CacheSeg->Bcb->CacheSegmentSize;
  }
  Mdl = MmCreateMdl(NULL, CacheSeg->BaseAddress, Size);
  MmBuildMdlForNonPagedPool(Mdl);
  Status = IoPageRead(CacheSeg->Bcb->FileObject, Mdl, &SegOffset, &IoStatus, TRUE);
  if (!NT_SUCCESS(Status) && Status != STATUS_END_OF_FILE)
  {
    CcRosReleaseCacheSegment(CacheSeg->Bcb, CacheSeg, FALSE);
    DPRINT1("IoPageRead failed, Status %x\n", Status);
	return Status;
  }
  if (CacheSeg->Bcb->CacheSegmentSize > Size)
  {
    memset (CacheSeg->BaseAddress + Size, 0, CacheSeg->Bcb->CacheSegmentSize - Size);
  }
  return STATUS_SUCCESS;
}

NTSTATUS WriteCacheSegment(PCACHE_SEGMENT CacheSeg)
{
  ULONG Size;
  PMDL Mdl;
  NTSTATUS Status;
  IO_STATUS_BLOCK IoStatus;
  LARGE_INTEGER SegOffset;

  SegOffset.QuadPart = CacheSeg->FileOffset;
  Size = CacheSeg->Bcb->AllocationSize.QuadPart - CacheSeg->FileOffset;
  if (Size > CacheSeg->Bcb->CacheSegmentSize)
  {
    Size = CacheSeg->Bcb->CacheSegmentSize;
  }
  Mdl = MmCreateMdl(NULL, CacheSeg->BaseAddress, Size);
  MmBuildMdlForNonPagedPool(Mdl);
  Status = IoPageWrite(CacheSeg->Bcb->FileObject, Mdl, &SegOffset, &IoStatus, TRUE);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("IoPageWrite failed, Status %x\n", Status);
  }
  return Status;
}

BOOLEAN STDCALL
CcCopyRead (
   IN PFILE_OBJECT FileObject,
   IN PLARGE_INTEGER FileOffset,
   IN ULONG Length,
   IN BOOLEAN Wait,
   OUT PVOID Buffer,
   OUT PIO_STATUS_BLOCK IoStatus)
{
   ULONG ReadOffset;
   ULONG TempLength;
   NTSTATUS Status = STATUS_SUCCESS;
   PVOID BaseAddress;
   PCACHE_SEGMENT CacheSeg;
   BOOLEAN Valid;
   ULONG ReadLength = 0;
   PBCB Bcb;
   KIRQL oldirql;
   PLIST_ENTRY current_entry;
   PCACHE_SEGMENT current;

   DPRINT("CcCopyRead(FileObject %x, FileOffset %x, "
	      "Length %d, Wait %d, Buffer %x, IoStatus %x)\n",
          FileObject, (ULONG)FileOffset->QuadPart, Length, Wait,
          Buffer, IoStatus);

   Bcb = ((REACTOS_COMMON_FCB_HEADER*)FileObject->FsContext)->Bcb;
   ReadOffset = FileOffset->QuadPart;

   DPRINT("AllocationSize %d, FileSize %d\n",
         (ULONG)Bcb->AllocationSize.QuadPart,
         (ULONG)Bcb->FileSize.QuadPart);

   if (!Wait)
   {
     KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);
     current_entry = Bcb->BcbSegmentListHead.Flink;
     while (current_entry != &Bcb->BcbSegmentListHead)
     {
       current = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, BcbSegmentListEntry);
       if (!current->Valid && current->FileOffset < ReadOffset + Length
         && current->FileOffset + Bcb->CacheSegmentSize > ReadOffset)
       {
         KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
         IoStatus->Status = STATUS_UNSUCCESSFUL;
         IoStatus->Information = 0;
         return FALSE;
       }
       current_entry = current_entry->Flink;
     }
     KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
   }

   TempLength = ReadOffset % Bcb->CacheSegmentSize;
   if (TempLength != 0)
   {
     TempLength = min (Length, Bcb->CacheSegmentSize - TempLength);
     Status = CcRosRequestCacheSegment(Bcb,
                ROUND_DOWN(ReadOffset, Bcb->CacheSegmentSize),
                &BaseAddress, &Valid, &CacheSeg);
     if (!NT_SUCCESS(Status))
     {
       IoStatus->Information = 0;
       IoStatus->Status = Status;
       DPRINT("CcRosRequestCacheSegment faild, Status %x\n", Status);
       return FALSE;
     }
     if (!Valid)
     {
       Status = ReadCacheSegment(CacheSeg);
       if (!NT_SUCCESS(Status))
       {
         IoStatus->Information = 0;
         IoStatus->Status = Status;
         return FALSE;
       }
     }
     memcpy (Buffer, BaseAddress + ReadOffset % Bcb->CacheSegmentSize, TempLength);
     CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE);
     ReadLength += TempLength;
     Length -= TempLength;
     ReadOffset += TempLength;
     Buffer += TempLength;
   }

   while (Length > 0)
   {
     TempLength = min (Bcb->CacheSegmentSize, Length);
     Status = CcRosRequestCacheSegment(Bcb, ReadOffset,
                &BaseAddress, &Valid, &CacheSeg);
     if (!NT_SUCCESS(Status))
     {
       IoStatus->Information = 0;
       IoStatus->Status = Status;
       DPRINT("CcRosRequestCacheSegment faild, Status %x\n", Status);
       return FALSE;
     }
     if (!Valid)
     {
       Status = ReadCacheSegment(CacheSeg);
       if (!NT_SUCCESS(Status))
       {
         IoStatus->Information = 0;
         IoStatus->Status = Status;
         return FALSE;
       }
     }
     memcpy (Buffer, BaseAddress, TempLength);
     CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE);
     ReadLength += TempLength;
     Length -= TempLength;
     ReadOffset += TempLength;
     Buffer += TempLength;
   }
   IoStatus->Status = STATUS_SUCCESS;
   IoStatus->Information = ReadLength;
   DPRINT("CcCopyRead O.K.\n");
   return TRUE;
}

BOOLEAN STDCALL
CcCopyWrite (
   IN PFILE_OBJECT FileObject,
   IN PLARGE_INTEGER FileOffset,
   IN ULONG Length,
   IN BOOLEAN Wait,
   IN PVOID Buffer)
{
   NTSTATUS Status;
   ULONG WriteOffset;
   KIRQL oldirql;
   PBCB Bcb;
   PLIST_ENTRY current_entry;
   PCACHE_SEGMENT CacheSeg;
   ULONG TempLength;
   PVOID BaseAddress;
   BOOLEAN Valid;

   DPRINT("CcCopyWrite(FileObject %x, FileOffset %x, "
	      "Length %d, Wait %d, Buffer %x)\n",
          FileObject, (ULONG)FileOffset->QuadPart, Length, Wait, Buffer);

   Bcb = ((REACTOS_COMMON_FCB_HEADER*)FileObject->FsContext)->Bcb;
   WriteOffset = (ULONG)FileOffset->QuadPart;

   if (!Wait)
   {
     // testing, if the requested datas are available
     KeAcquireSpinLock(&Bcb->BcbLock, &oldirql);
     current_entry = Bcb->BcbSegmentListHead.Flink;
     while (current_entry != &Bcb->BcbSegmentListHead)
     {
       CacheSeg = CONTAINING_RECORD(current_entry, CACHE_SEGMENT, BcbSegmentListEntry);
       if (!CacheSeg->Valid)
       {
         if ((WriteOffset >= CacheSeg->FileOffset && WriteOffset < CacheSeg->FileOffset + Bcb->CacheSegmentSize)
           || (WriteOffset + Length > CacheSeg->FileOffset && WriteOffset + Length <= CacheSeg->FileOffset + Bcb->CacheSegmentSize))
         {
           KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
           // datas not available
           return FALSE;
         }
       }
       current_entry = current_entry->Flink;
     }
     KeReleaseSpinLock(&Bcb->BcbLock, oldirql);
   }

   TempLength = WriteOffset % Bcb->CacheSegmentSize;
   if (TempLength != 0)
   {
      TempLength = min (Length, Bcb->CacheSegmentSize - TempLength);
      Status = CcRosRequestCacheSegment(Bcb,
                 ROUND_DOWN(WriteOffset, Bcb->CacheSegmentSize),
                 &BaseAddress, &Valid, &CacheSeg);
      if (!NT_SUCCESS(Status))
      {
        return FALSE;
      }
      if (!Valid)
      {
         if (!NT_SUCCESS(ReadCacheSegment(CacheSeg)))
         {
           return FALSE;
         }
      }
      memcpy (BaseAddress + WriteOffset % Bcb->CacheSegmentSize, Buffer, TempLength);
      if (!NT_SUCCESS(WriteCacheSegment(CacheSeg)))
      {
        return FALSE;
      }
      CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE);

      Length -= TempLength;
      WriteOffset += TempLength;
      Buffer += TempLength;
   }

   while (Length > 0)
   {
      TempLength = min (Bcb->CacheSegmentSize, Length);
      Status = CcRosRequestCacheSegment(Bcb, WriteOffset,
                 &BaseAddress, &Valid, &CacheSeg);
      if (!NT_SUCCESS(Status))
      {
         return FALSE;
      }
      if (!Valid && TempLength < Bcb->CacheSegmentSize)
      {
        if (!NT_SUCCESS(ReadCacheSegment(CacheSeg)))
        {
          return FALSE;
        }
      }
      memcpy (BaseAddress, Buffer, TempLength);
      if (!NT_SUCCESS(WriteCacheSegment(CacheSeg)))
      {
        return FALSE;
      }
      CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE);
      Length -= TempLength;
      WriteOffset += TempLength;
      Buffer += TempLength;
   }
   return TRUE;
}


