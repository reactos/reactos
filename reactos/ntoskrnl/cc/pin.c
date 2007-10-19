/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cc/pin.c
 * PURPOSE:         Implements cache managers pinning interface
 *
 * PROGRAMMERS:
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

extern NPAGED_LOOKASIDE_LIST iBcbLookasideList;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOLEAN STDCALL
CcMapData (IN PFILE_OBJECT FileObject,
	   IN PLARGE_INTEGER FileOffset,
	   IN ULONG Length,
	   IN ULONG Flags,
	   OUT PVOID *pBcb,
	   OUT PVOID *pBuffer)
{
  ULONG ReadOffset;
  BOOLEAN Valid;
  PBCB Bcb;
  PCACHE_SEGMENT CacheSeg;
  NTSTATUS Status;
  PINTERNAL_BCB iBcb;
  ULONG ROffset;

  DPRINT("CcMapData(FileObject 0x%p, FileOffset %I64x, Length %d, Flags %d,"
	 " pBcb 0x%p, pBuffer 0x%p)\n", FileObject, FileOffset->QuadPart,
	 Length, Flags, pBcb, pBuffer);

  ReadOffset = (ULONG)FileOffset->QuadPart;
  Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
  ASSERT(Bcb);

  DPRINT("AllocationSize %I64x, FileSize %I64x\n",
         Bcb->AllocationSize.QuadPart,
         Bcb->FileSize.QuadPart);

  if (ReadOffset % Bcb->CacheSegmentSize + Length > Bcb->CacheSegmentSize)
    {
      return(FALSE);
    }
  ROffset = ROUND_DOWN (ReadOffset, Bcb->CacheSegmentSize);
  Status = CcRosRequestCacheSegment(Bcb,
				    ROffset,
				    pBuffer,
				    &Valid,
				    &CacheSeg);
  if (!NT_SUCCESS(Status))
    {
      return(FALSE);
    }
  if (!Valid)
    {
      if (!(Flags & MAP_WAIT))
	{
          CcRosReleaseCacheSegment(Bcb, CacheSeg, FALSE, FALSE, FALSE);
	  return(FALSE);
	}
      if (!NT_SUCCESS(ReadCacheSegment(CacheSeg)))
	{
          CcRosReleaseCacheSegment(Bcb, CacheSeg, FALSE, FALSE, FALSE);
	  return(FALSE);
	}
    }

  *pBuffer = (PVOID)((ULONG_PTR)(*pBuffer) + (ReadOffset % Bcb->CacheSegmentSize));
  iBcb = ExAllocateFromNPagedLookasideList(&iBcbLookasideList);
  if (iBcb == NULL)
    {
      CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE, FALSE, FALSE);
      return FALSE;
    }
  memset(iBcb, 0, sizeof(INTERNAL_BCB));
  iBcb->PFCB.NodeTypeCode = 0xDE45; /* Undocumented (CAPTIVE_PUBLIC_BCB_NODETYPECODE) */
  iBcb->PFCB.NodeByteSize = sizeof(PUBLIC_BCB);
  iBcb->PFCB.MappedLength = Length;
  iBcb->PFCB.MappedFileOffset = *FileOffset;
  iBcb->CacheSegment = CacheSeg;
  iBcb->Dirty = FALSE;
  iBcb->RefCount = 1;
  *pBcb = (PVOID)iBcb;
  return(TRUE);
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
CcPinMappedData (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	ULONG			Flags,
	OUT	PVOID			* Bcb
	)
{
  /* no-op for current implementation. */
  return TRUE;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
CcPinRead (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	ULONG			Flags,
	OUT	PVOID			* Bcb,
	OUT	PVOID			* Buffer
	)
{
  if (CcMapData(FileObject, FileOffset, Length, Flags, Bcb, Buffer))
  {
    if (CcPinMappedData(FileObject, FileOffset, Length, Flags, Bcb))
      return TRUE;
    else
      CcUnpinData(Bcb);
  }
  return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
CcPreparePinWrite (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG			Length,
	IN	BOOLEAN			Zero,
	IN	ULONG			Flags,
	OUT	PVOID			* Bcb,
	OUT	PVOID			* Buffer
	)
{
        /*
         * FIXME: This is function is similar to CcPinRead, but doesn't
         * read the data if they're not present. Instead it should just
         * prepare the cache segments and zero them out if Zero == TRUE.
         *
         * For now calling CcPinRead is better than returning error or
         * just having UNIMPLEMENTED here.
         */
        return CcPinRead(FileObject, FileOffset, Length, Flags, Bcb, Buffer);
}

/*
 * @implemented
 */
VOID STDCALL
CcSetDirtyPinnedData (IN PVOID Bcb,
		      IN PLARGE_INTEGER Lsn)
{
   PINTERNAL_BCB iBcb = Bcb;
   iBcb->Dirty = TRUE;
}


/*
 * @implemented
 */
VOID STDCALL
CcUnpinData (IN PVOID Bcb)
{
  PINTERNAL_BCB iBcb = Bcb;
  CcRosReleaseCacheSegment(iBcb->CacheSegment->Bcb, iBcb->CacheSegment, TRUE,
                           iBcb->Dirty, FALSE);
  if (--iBcb->RefCount == 0)
  {
    ExFreeToNPagedLookasideList(&iBcbLookasideList, iBcb);
  }
}

/*
 * @unimplemented
 */
VOID
STDCALL
CcUnpinDataForThread (
	IN	PVOID			Bcb,
	IN	ERESOURCE_THREAD	ResourceThreadId
	)
{
	UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
STDCALL
CcRepinBcb (
	IN	PVOID	Bcb
	)
{
  PINTERNAL_BCB iBcb = Bcb;
  iBcb->RefCount++;
}

/*
 * @unimplemented
 */
VOID
STDCALL
CcUnpinRepinnedBcb (
	IN	PVOID			Bcb,
	IN	BOOLEAN			WriteThrough,
	IN	PIO_STATUS_BLOCK	IoStatus
	)
{
  PINTERNAL_BCB iBcb = Bcb;

  if (--iBcb->RefCount == 0)
    {
      IoStatus->Information = 0;
      if (WriteThrough)
        {
          ExEnterCriticalRegionAndAcquireFastMutexUnsafe(&iBcb->CacheSegment->Lock);
          if (iBcb->CacheSegment->Dirty)
            {
              IoStatus->Status = CcRosFlushCacheSegment(iBcb->CacheSegment);
            }
          else
            {
              IoStatus->Status = STATUS_SUCCESS;
            }
          ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(&iBcb->CacheSegment->Lock);
        }
      else
        {
          IoStatus->Status = STATUS_SUCCESS;
        }

      ExFreeToNPagedLookasideList(&iBcbLookasideList, iBcb);
    }
}
