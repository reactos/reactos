/* $Id: malloc.c,v 1.4 2003/04/02 22:09:57 hyperion Exp $
 */
/*
 * COPYRIGHT:   None
 * LICENSE:     Public domain
 * PROJECT:     ReactOS system libraries
 * FILE:        reactos/lib/psapi/misc/malloc.c
 * PURPOSE:     Memory allocator for PSAPI
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 * UPDATE HISTORY:
 *              10/06/2002: Created
 *              12/02/2003: malloc and free renamed to PsaiMalloc and PsaiFree,
 *                          for better reusability
 */

#include <ddk/ntddk.h>
#include <napi/teb.h>
#include <ntos/heap.h>

PVOID STDCALL MemAlloc
(
 IN HANDLE Heap,
 IN PVOID Ptr,
 IN ULONG Size
)
{
 PVOID pBuf = NULL;

 if(Size == 0 && Ptr == NULL)
  return (NULL);
  
 if(Heap == NULL)
  Heap = NtCurrentPeb()->ProcessHeap;
 
 if(Size > 0)
 {
  if(Ptr == NULL)
   /* malloc */
   pBuf = RtlAllocateHeap(Heap, 0, Size);
  else
   /* realloc */
   pBuf = RtlReAllocateHeap(Heap, 0, Ptr, Size);
 }
 else
  /* free */
  RtlFreeHeap(Heap, 0, Ptr);

 return pBuf;
}

void *PsaiMalloc(SIZE_T size)
{
 return MemAlloc(NULL, NULL, size);
}

void *PsaiRealloc(void *ptr, SIZE_T size)
{
 return MemAlloc(NULL, ptr, size);
}

void PsaiFree(void *ptr)
{
 MemAlloc(NULL, ptr, 0);
}

/* EOF */

