/* $Id: heap.c,v 1.18 2001/02/17 17:42:46 ekohl Exp $
 *
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
*                   GetProcessHeap  --  KERNEL32                     *
*********************************************************************/
HANDLE WINAPI GetProcessHeap(VOID)
{
   DPRINT("GetProcessHeap()\n");
   return(RtlGetProcessHeap());
}

/********************************************************************
*                   GetProcessHeaps  --  KERNEL32                   *
********************************************************************/
DWORD WINAPI GetProcessHeaps(DWORD maxheaps, PHANDLE phandles )
{
   return(RtlGetProcessHeaps(maxheaps, phandles));
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
UINT WINAPI HeapCompact(HANDLE hheap, DWORD flags)
{
   return RtlCompactHeap(hheap, flags);
}

/*********************************************************************
*                    HeapValidate  --  KERNEL32                      *
*********************************************************************/
BOOL WINAPI HeapValidate(HANDLE hheap, DWORD flags, LPCVOID pmem)
{
   return(RtlValidateHeap(hheap, flags, (PVOID)pmem));
}


/* EOF */
