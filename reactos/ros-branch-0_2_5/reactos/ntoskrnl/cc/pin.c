/* $Id: pin.c,v 1.18 2004/10/22 20:11:11 ekohl Exp $
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

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define ROUND_DOWN(N, S) ((N) - ((N) % (S)))

extern NPAGED_LOOKASIDE_LIST iBcbLookasideList;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOLEAN STDCALL
CcMapData (IN PFILE_OBJECT FileObject,
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
  ULONG ROffset;
  
  DPRINT("CcMapData(FileObject %x, FileOffset %d, Length %d, Wait %d,"
	 " pBcb %x, pBuffer %x)\n", FileObject, (ULONG)FileOffset->QuadPart,
	 Length, Wait, pBcb, pBuffer);
  
  ReadOffset = (ULONG)FileOffset->QuadPart;
  Bcb = FileObject->SectionObjectPointer->SharedCacheMap;
  ASSERT(Bcb);

  DPRINT("AllocationSize %d, FileSize %d\n",
         (ULONG)Bcb->AllocationSize.QuadPart,
         (ULONG)Bcb->FileSize.QuadPart);
  
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
      if (!Wait)
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
#if defined(__GNUC__)
  *pBuffer += ReadOffset % Bcb->CacheSegmentSize;
#else
  {
    char* pTemp = *pBuffer;
    pTemp += ReadOffset % Bcb->CacheSegmentSize;
    *pBuffer = pTemp;
  }
#endif
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
	IN	BOOLEAN			Wait,
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
	IN	BOOLEAN			Wait,
	OUT	PVOID			* Bcb,
	OUT	PVOID			* Buffer
	)
{
  if (CcMapData(FileObject, FileOffset, Length, Wait, Bcb, Buffer))
  {
    if (CcPinMappedData(FileObject, FileOffset, Length, Wait, Bcb))
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
	IN	BOOLEAN			Wait,
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
        return CcPinRead(FileObject, FileOffset, Length, Wait, Bcb, Buffer);
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
          ExAcquireFastMutex(&iBcb->CacheSegment->Lock);
          if (iBcb->CacheSegment->Dirty)
            {
              IoStatus->Status = CcRosFlushCacheSegment(iBcb->CacheSegment);
            }
          else
            {
              IoStatus->Status = STATUS_SUCCESS;
            }
          ExReleaseFastMutex(&iBcb->CacheSegment->Lock);
        }
      else
        {
          IoStatus->Status = STATUS_SUCCESS;
        }

      ExFreeToNPagedLookasideList(&iBcbLookasideList, iBcb);
    }
}
