/* $Id: pin.c,v 1.15 2004/06/21 04:11:44 ion Exp $
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
  assert(Bcb);

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
  iBcb->CacheSegment = CacheSeg;
  iBcb->Dirty = FALSE;
  iBcb->PFCB.MappedLength = Length;
  iBcb->PFCB.MappedFileOffset.QuadPart = FileOffset->QuadPart;
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
	UNIMPLEMENTED;
	return FALSE;
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
	UNIMPLEMENTED;
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
	UNIMPLEMENTED;
	return FALSE;
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
  ExFreeToNPagedLookasideList(&iBcbLookasideList, iBcb);
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
