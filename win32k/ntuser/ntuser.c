/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  $Id: painting.c 16320 2005-06-29 07:09:25Z navaraf $
 *
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS kernel
 *  PURPOSE:          ntuser init. and main funcs.
 *  FILE:             subsys/win32k/ntuser/ntuser.c
 *  REVISION HISTORY:
 *       16 July 2005   Created (hardon)
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>


//ERESOURCE UserLock;
FAST_MUTEX UserLock;

//char* _file;
//DWORD _line;
DWORD _locked=0;

/* FUNCTIONS **********************************************************/

inline PVOID FASTCALL UserAllocZeroTag(SIZE_T bytes, ULONG tag)
{
   PVOID mem = ExAllocatePoolWithTag(PagedPool, bytes, tag);
   if (!mem) return NULL;
   RtlZeroMemory(mem, bytes);
   return mem;
}

inline PVOID FASTCALL UserAllocZero(SIZE_T bytes)
{
      //FIXME: use default ntuser tag
   PVOID mem = ExAllocatePoolWithTag(PagedPool, bytes, 0);
   if (!mem) return NULL;
   RtlZeroMemory(mem, bytes);
   return mem;
}

inline PVOID FASTCALL UserAllocTag(SIZE_T bytes, ULONG tag)
{
   return ExAllocatePoolWithTag(PagedPool, bytes, tag);
}

inline PVOID FASTCALL UserAlloc(SIZE_T bytes)
{
   //FIXME: use default ntuser tag
   return ExAllocatePoolWithTag(PagedPool, bytes, 0);
}

inline VOID FASTCALL UserFree(PVOID mem)
{
   ExFreePool(mem);
}

VOID FASTCALL UserStackTrace()
{
//7   DPRINT("Strack trace\n");
//   DPRINT("ret1: 0x%x\n", __builtin_return_address(0));
//   DPRINT("ret2: 0x%x\n", __builtin_return_address(1));
//   DPRINT("ret3: 0x%x\n", __builtin_return_address(2));
//   DPRINT("ret4: 0x%x\n", __builtin_return_address(3));
//   DPRINT("ret5: 0x%x\n", __builtin_return_address(4));
}


NTSTATUS FASTCALL InitUserImpl(VOID)
{
//   DPRINT("Enter InitUserImpl\n");
//   ExInitializeResourceLite(&UserLock);
   ExInitializeFastMutex(&UserLock);
   return STATUS_SUCCESS;
}

/*
RETURN
   True if current thread owns the lock (possibly shared)

*/
BOOL FASTCALL UserIsEntered()
{
   
   return (UserLock.Owner == KeGetCurrentThread());
   
}

VOID FASTCALL CleanupUser(VOID)
{
//   ExDeleteResourceLite(&UserLock);
}

VOID FASTCALL UUserEnterShared(VOID)
{
//   DPRINT("Enter IntLockUserShared\n");
//   KeDumpStackFrames((PULONG)__builtin_frame_address(0));
//DPRINT("%x\n",__builtin_return_address(0));
//   KeEnterCriticalRegion();
//   ExAcquireResourceSharedLite(&UserLock, TRUE);
ExAcquireFastMutex(&UserLock);

}

VOID FASTCALL UUserEnterExclusive(VOID)
{
//   DPRINT("Enter UserEnterExclusive\n");
//   KeDumpStackFrames((PULONG)__builtin_frame_address(0));
//DPRINT("%x\n",__builtin_return_address(0));
//   KeEnterCriticalRegion();
//   ExAcquireResourceExclusiveLite(&UserLock, TRUE);
ExAcquireFastMutex(&UserLock);
}

VOID FASTCALL UUserLeave(VOID)
{
//   DPRINT("Enter UserLeave\n");
//   KeDumpStackFrames((PULONG)__builtin_frame_address(0));
//DPRINT("%x\n",__builtin_return_address(0));
//  ExReleaseResourceLite(&UserLock);
//   KeLeaveCriticalRegion();
ExReleaseFastMutex(&UserLock);
}
