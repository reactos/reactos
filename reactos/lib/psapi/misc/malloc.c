/* $Id: malloc.c,v 1.3 2002/08/31 17:11:24 hyperion Exp $
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

void *malloc(size_t size)
{
 return MemAlloc(NULL, NULL, size);
}

void *realloc(void *ptr, size_t size)
{
 return MemAlloc(NULL, ptr, size);
}

void free(void *ptr)
{
 MemAlloc(NULL, ptr, 0);
}

/* EOF */

