/*
 * kernel/heap.c
 * Copyright (C) 1996, Onno Hovers, All rights reserved
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; see the file COPYING.LIB. If
 * not, write to the Free Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA.
 *
 * Win32 heap functions (HeapXXX).
 *
 */

/*
 * Adapted for the ReactOS system libraries by David Welch (welch@mcmail.com)
 * Put the type definitions of the heap in a seperate header. Boudewijn Dekker
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/*********************************************************************
*                     HeapCreate -- KERNEL32                         *
*********************************************************************/
HANDLE STDCALL HeapCreate(DWORD flags, DWORD minsize, DWORD maxsize)
{

   DPRINT("HeapCreate( 0x%lX, 0x%lX, 0x%lX )\n", flags, minsize, maxsize);
   return(RtlCreateHeap(flags, NULL, maxsize, minsize, NULL, NULL));
}

/*********************************************************************
*                     HeapDestroy -- KERNEL32                        *
*********************************************************************/
BOOL WINAPI HeapDestroy(HANDLE hheap)
{
   return(RtlDestroyHeap(hheap));
}

/*********************************************************************
*                     HeapAlloc -- KERNEL32                          *
*********************************************************************/
LPVOID STDCALL HeapAlloc(HANDLE hheap, DWORD flags, DWORD size)
{
   return(RtlAllocateHeap(hheap, flags, size));
}

/*********************************************************************
*                     HeapReAlloc -- KERNEL32                        *
*********************************************************************/
LPVOID STDCALL HeapReAlloc(HANDLE hheap, DWORD flags, LPVOID ptr, DWORD size)
{
   return(RtlReAllocHeap(hheap, flags, ptr, size));
}

/*********************************************************************
*                     HeapFree -- KERNEL32                           *
*********************************************************************/
WINBOOL STDCALL HeapFree(HANDLE hheap, DWORD flags, LPVOID ptr)
{
   return(RtlFreeHeap(hheap, flags, ptr));
}

/*********************************************************************
*                   GetProcessHeap  --  KERNEL32                     *
*********************************************************************/
HANDLE WINAPI GetProcessHeap(VOID)
{
   DPRINT("GetProcessHeap()\n");
   return(RtlGetProcessHeap());
}

/********************************************************************
*                   GetProcessHeaps  --  KERNEL32                   *
*                                                                   *
* NOTE in Win95 this function is not implemented and just returns   *
* ERROR_CALL_NOT_IMPLEMENTED                                        *
********************************************************************/
DWORD WINAPI GetProcessHeaps(DWORD maxheaps, PHANDLE phandles )
{
}

/*********************************************************************
*                    HeapLock  --  KERNEL32                          *
*********************************************************************/
BOOL WINAPI HeapLock(HANDLE hheap)
{
   return(RtlLockHeap(hheap));
}

/*********************************************************************
*                    HeapUnlock  --  KERNEL32                        *
*********************************************************************/
BOOL WINAPI HeapUnlock(HANDLE hheap)
{
   return(RtlUnlockHeap(hheap));
}

/*********************************************************************
*                    HeapCompact  --  KERNEL32                       *
*                                                                    *
* NT uses this function to compact moveable blocks and other things  *
* Here it does not compact, but it finds the largest free region     *
*********************************************************************/
UINT HeapCompact(HANDLE hheap, DWORD flags)
{
   return(RtlCompactHeap(hheap, flags));
}

/*********************************************************************
*                    HeapSize  --  KERNEL32                          *
*********************************************************************/
DWORD WINAPI HeapSize(HANDLE hheap, DWORD flags, LPCVOID pmem)
{
   return(RtlSizeHeap(hheap, flags, pmem));
}

/*********************************************************************
*                    HeapValidate  --  KERNEL32                      *
*                                                                    *
* NOTE: only implemented in NT                                       *
*********************************************************************/
BOOL WINAPI HeapValidate(HANDLE hheap, DWORD flags, LPCVOID pmem)
{
   return(RtlValidateHeap(hheap, flags, pmem));
}

