/* $Id: libsupp.c,v 1.1 2004/05/31 19:33:59 gdalsnes Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/rtl/libsup.c
 * PURPOSE:         Rtl library support routines
 * PROGRAMMER:      Gunnar Dalsnes
 * UPDATE HISTORY:
 *
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <ctype.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* FUNCTIONS ***************************************************************/

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
