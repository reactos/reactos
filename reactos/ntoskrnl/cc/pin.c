/* $Id: pin.c,v 1.1 2001/10/10 21:49:15 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/pin.c
 * PURPOSE:         Implements cache managers pinning interface
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

typedef struct _INTERNAL_BCB
{
  PUBLIC_BCB PFCB;
  PCACHE_SEGMENT CacheSegment;
}
INTERNAL_BCB, *PINTERNAL_BCB;

BOOLEAN STDCALL
CcMapData (
   IN PFILE_OBJECT FileObject,
   IN PLARGE_INTEGER FileOffset,
   IN ULONG Length,
   IN BOOLEAN Wait,
   OUT PVOID *pBcb,
   OUT PVOID *pBuffer)
{
   ULONG ReadOffset;
   BOOLEAN Valid;
   PBCB Bcb;
   PCACHE_SEGMENT CacheSeg;
   NTSTATUS Status;
   PINTERNAL_BCB iBcb;

   DPRINT("CcMapData(FileObject %x, FileOffset %d, Length %d, Wait %d,"
           " pBcb %x, pBuffer %x)\n", FileObject, (ULONG)FileOffset->QuadPart,
           Length, Wait, pBcb, pBuffer);

   ReadOffset = FileOffset->QuadPart;
   Bcb = ((REACTOS_COMMON_FCB_HEADER*)FileObject->FsContext)->Bcb;

   DPRINT("AllocationSize %d, FileSize %d\n",
         (ULONG)Bcb->AllocationSize.QuadPart,
         (ULONG)Bcb->FileSize.QuadPart);

   if (ReadOffset % Bcb->CacheSegmentSize + Length > Bcb->CacheSegmentSize)
   {
      return FALSE;
   }
   Status = CcRosRequestCacheSegment(Bcb,
               ROUND_DOWN (ReadOffset, Bcb->CacheSegmentSize),
               pBuffer,
               &Valid,
               &CacheSeg);
   if (!NT_SUCCESS(Status))
   {
      return FALSE;
   }
   if (!Valid)
   {
      if (!Wait)
      {
          CcRosReleaseCacheSegment(Bcb, CacheSeg, FALSE);
	      return FALSE;
      }
      if (!NT_SUCCESS(ReadCacheSegment(CacheSeg)))
      {
        return FALSE;
      }
   }
   *pBuffer += ReadOffset % Bcb->CacheSegmentSize;
   iBcb = ExAllocatePool (NonPagedPool, sizeof(INTERNAL_BCB));
   if (iBcb == NULL)
   {
     CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE);
     return FALSE;
   }
   iBcb->CacheSegment = CacheSeg;
   iBcb->PFCB.MappedLength = Length;
   iBcb->PFCB.MappedFileOffset.QuadPart = FileOffset->QuadPart;
   *pBcb = (PVOID)iBcb;
   return TRUE;
}

VOID STDCALL
CcUnpinData (
   IN PVOID Bcb)
{
   PINTERNAL_BCB iBcb = Bcb;
   CcRosReleaseCacheSegment(iBcb->CacheSegment->Bcb, iBcb->CacheSegment, TRUE);
   ExFreePool(iBcb);
}

VOID STDCALL
CcSetDirtyPinnedData (
    IN PVOID Bcb,
    IN PLARGE_INTEGER Lsn)
{
   PINTERNAL_BCB iBcb = Bcb;
   // FIXME: write only the modifyed 4-pages back
   WriteCacheSegment(iBcb->CacheSegment);
}

