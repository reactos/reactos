/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/rtl/libsup.c
 * PURPOSE:         Rtl library support routines
 * PROGRAMMER:      Gunnar Dalsnes
 * UPDATE HISTORY:
 *
 */

#include <ntdll.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

KPROCESSOR_MODE
RtlpGetMode()
{
   return UserMode;
}


PVOID
STDCALL
ExAllocatePool(
   IN POOL_TYPE   PoolType,
   IN SIZE_T      Bytes
)
{
   return RtlAllocateHeap (
      RtlGetProcessHeap (),
      0,
      Bytes);
}

PVOID
STDCALL
ExAllocatePoolWithTag(
   IN POOL_TYPE   PoolType,
   IN SIZE_T      Bytes,
   IN ULONG       Tag
)
{
   return RtlAllocateHeap (
      RtlGetProcessHeap (),
      0,
      Bytes);
}

VOID
STDCALL
ExFreePool(IN PVOID Mem)
{
   RtlFreeHeap (
      RtlGetProcessHeap (),
      0,
      Mem);
}

VOID
STDCALL
ExFreePoolWithTag(IN PVOID Mem, IN ULONG Tag)
{
   RtlFreeHeap (
      RtlGetProcessHeap (),
      0,
      Mem);
}


#ifdef DBG
VOID FASTCALL
CHECK_PAGED_CODE_RTL(char *file, int line)
{
  /* meaningless in user mode */
}
#endif
