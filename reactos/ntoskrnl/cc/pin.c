/* $Id: pin.c,v 1.6 2002/09/07 15:12:47 chorns Exp $
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

/* FUNCTIONS *****************************************************************/

typedef struct _INTERNAL_BCB
{
  PUBLIC_BCB PFCB;
  PROS_CACHE_SEGMENT CacheSegment;
  BOOLEAN Dirty;
} INTERNAL_BCB, *PINTERNAL_BCB;

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
  PROS_BCB Bcb;
  PROS_CACHE_SEGMENT CacheSeg;
  NTSTATUS Status;
  PINTERNAL_BCB iBcb;
  ULONG ROffset;
  
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
	  return(FALSE);
	}
    }
  *pBuffer += ReadOffset % Bcb->CacheSegmentSize;
  iBcb = ExAllocatePool (NonPagedPool, sizeof(INTERNAL_BCB));
  if (iBcb == NULL)
    {
      CcRosReleaseCacheSegment(Bcb, CacheSeg, TRUE, FALSE, FALSE);
      return FALSE;
    }
  iBcb->CacheSegment = CacheSeg;
  iBcb->Dirty = FALSE;
  iBcb->PFCB.MappedLength = Length;
  iBcb->PFCB.MappedFileOffset.QuadPart = FileOffset->QuadPart;
  *pBcb = (PVOID)iBcb;
  return(TRUE);
}

VOID STDCALL
CcUnpinData (IN PVOID Bcb)
{
  PINTERNAL_BCB iBcb = Bcb;
  CcRosReleaseCacheSegment(iBcb->CacheSegment->Bcb, iBcb->CacheSegment, TRUE, 
			   iBcb->Dirty, FALSE);
  ExFreePool(iBcb);
}

VOID STDCALL
CcSetDirtyPinnedData (IN PVOID Bcb,
		      IN PLARGE_INTEGER Lsn)
{
   PINTERNAL_BCB iBcb = Bcb;
#if 0
   iBcb->Dirty = TRUE;
#else
   WriteCacheSegment(iBcb->CacheSegment);
#endif
}

