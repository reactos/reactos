/* $Id: heap.c,v 1.20 2002/09/07 15:12:26 chorns Exp $
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

#define NTOS_USER_MODE
#include <ntos.h>

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


DWORD
STDCALL
HeapCreateTagsW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
HeapExtend (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
#if 0
   NTSTATUS Status;

   Status = RtlExtendHeap(Unknown1, Unknown2, Unknown3, Unknown4);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }
   return TRUE;
#endif

   SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
   return 0;
}


DWORD
STDCALL
HeapQueryTagW (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
HeapSummary (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
HeapUsage (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
HeapWalk (
	HANDLE			hHeap,
	LPPROCESS_HEAP_ENTRY	lpEntry
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/* EOF */
