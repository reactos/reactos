/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Memory Management Functions
 * FILE:              subsys/win32k/eng/mem.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */

#include <ddk/ntddk.h>
#include <ddk/winddi.h>

PVOID STDCALL EngAllocMem(ULONG Flags, ULONG MemSize, ULONG Tag)
{
   PVOID newMem;

   newMem = ExAllocatePoolWithTag(PagedPool, MemSize, Tag);

   if(Flags == FL_ZERO_MEMORY)
   {
     RtlZeroMemory(newMem, MemSize);
   }

   return newMem;
}

VOID STDCALL EngFreeMem(PVOID Mem)
{
   ExFreePool(Mem);
}

PVOID STDCALL EngAllocUserMem(ULONG cj, ULONG tag)
{
   PVOID newMem;

/*   return ZwAllocateVirtualMemory(mycurrentprocess, newMem, 0, cj,
     MEM_COMMIT, PAGE_READWRITE); */
}

VOID STDCALL EngFreeUserMem(PVOID pv)
{
/*   ZwFreeVirtualMemory (mycurrentprocess, pv, 0, MEM_DECOMMIT); */
}

