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
 */
/* $Id: mem.c,v 1.9 2003/06/06 10:17:44 gvg Exp $
 *
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

typedef struct _USERMEMHEADER
  {
  ULONG Tag;
  ULONG MemSize;
  }
USERMEMHEADER, *PUSERMEMHEADER;

PVOID STDCALL
EngAllocMem(ULONG Flags,
	    ULONG MemSize,
	    ULONG Tag)
{
  PVOID newMem;

  newMem = ExAllocatePoolWithTag(NonPagedPool, MemSize, Tag); // FIXME: Use PagedPool when it is implemented

  if(Flags == FL_ZERO_MEMORY)
  {
    RtlZeroMemory(newMem, MemSize);
  }

  return newMem;
}

VOID STDCALL
EngFreeMem(PVOID Mem)
{
  ExFreePool(Mem);
}

PVOID STDCALL
EngAllocUserMem(ULONG cj, ULONG Tag)
{
  PVOID NewMem = NULL;
  NTSTATUS Status;
  ULONG MemSize = sizeof(USERMEMHEADER) + cj;
  PUSERMEMHEADER Header;

  Status = ZwAllocateVirtualMemory(NtCurrentProcess(), &NewMem, 0, &MemSize, MEM_COMMIT, PAGE_READWRITE);

  if (! NT_SUCCESS(Status))
    {
      return NULL;
    }

  Header = (PUSERMEMHEADER) NewMem;
  Header->Tag = Tag;
  Header->MemSize = cj;

  return (PVOID)(Header + 1);
}

VOID STDCALL
EngFreeUserMem(PVOID pv)
{
  PUSERMEMHEADER Header = ((PUSERMEMHEADER) pv) - 1;
  ULONG MemSize = sizeof(USERMEMHEADER) + Header->MemSize;

  ZwFreeVirtualMemory(NtCurrentProcess(), (PVOID *) &Header, &MemSize, MEM_DECOMMIT);
}
/* EOF */
