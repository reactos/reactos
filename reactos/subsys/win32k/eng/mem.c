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

PVOID EngAllocMem(ULONG Flags, ULONG MemSize, ULONG Tag)
{
   PVOID newMem;

   newMem = ExAllocatePoolWithTag(PagedPool, MemSize, Tag);

   if(Flags == FL_ZERO_MEMORY)
   {
     RtlZeroMemory(newMem, MemSize);
   }

   return newMem;
}

VOID EngFreeMem(PVOID Mem)
{
   ExFreePool(Mem);
}

PVOID EngAllocUserMem(ULONG cj, ULONG tag)
{
   PVOID newMem;

/*   return ZwAllocateVirtualMemory(mycurrentprocess, newMem, 0, cj,
     MEM_COMMIT, PAGE_READWRITE); */
}

VOID EngFreeUserMem(PVOID pv)
{
/*   ZwFreeVirtualMemory (mycurrentprocess, pv, 0, MEM_DECOMMIT); */
}
