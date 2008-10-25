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
/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Driver Memory Management Functions
 * FILE:              subsys/win32k/eng/mem.c
 * PROGRAMER:         Jason Filby
 * REVISION HISTORY:
 *                 3/7/1999: Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

typedef struct _USERMEMHEADER
  {
  ULONG Tag;
  ULONG MemSize;
  }
USERMEMHEADER, *PUSERMEMHEADER;

/*
 * @implemented
 */
PVOID STDCALL
EngAllocMem(ULONG Flags,
	    ULONG MemSize,
	    ULONG Tag)
{
  PVOID newMem;

  newMem = ExAllocatePoolWithTag(PagedPool, MemSize, Tag);

  if (Flags == FL_ZERO_MEMORY && NULL != newMem)
  {
    RtlZeroMemory(newMem, MemSize);
  }

  return newMem;
}

/*
 * @implemented
 */
VOID STDCALL
EngFreeMem(PVOID Mem)
{
  ExFreePool(Mem);
}

/*
 * @implemented
 */
PVOID STDCALL
EngAllocUserMem(SIZE_T cj, ULONG Tag)
{
  PVOID NewMem = NULL;
  NTSTATUS Status;
  SIZE_T MemSize = sizeof(USERMEMHEADER) + cj;
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

/*
 * @implemented
 */
VOID STDCALL
EngFreeUserMem(PVOID pv)
{
  PUSERMEMHEADER Header = ((PUSERMEMHEADER) pv) - 1;
  SIZE_T MemSize = sizeof(USERMEMHEADER) + Header->MemSize;

  ZwFreeVirtualMemory(NtCurrentProcess(), (PVOID *) &Header, &MemSize, MEM_RELEASE);
}



PVOID
NTAPI
HackSecureVirtualMemory(
	IN PVOID Address,
	IN SIZE_T Size,
	IN ULONG ProbeMode,
	OUT PVOID *SafeAddress)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PMDL mdl;
	LOCK_OPERATION Operation;

	if (ProbeMode == PAGE_READONLY) Operation = IoReadAccess;
	else if (ProbeMode == PAGE_READWRITE) Operation = IoModifyAccess;
	else return NULL;

	mdl = IoAllocateMdl(Address, Size, FALSE, TRUE, NULL);
	if (mdl == NULL)
	{
		return NULL;
	}

	_SEH_TRY
	{
		MmProbeAndLockPages(mdl, UserMode, Operation);
	}
	_SEH_HANDLE
	{
		Status = _SEH_GetExceptionCode();
	}
	_SEH_END

	if (!NT_SUCCESS(Status))
	{
		IoFreeMdl(mdl);
		return NULL;
	}

	*SafeAddress = MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority);

	if(!*SafeAddress)
	{
		MmUnlockPages(mdl);
		IoFreeMdl(mdl);
		return NULL;           
	}

	return mdl;
}

VOID
NTAPI
HackUnsecureVirtualMemory(
	IN PVOID  SecureHandle)
{
	PMDL mdl = (PMDL)SecureHandle;

	MmUnlockPages(mdl);
	IoFreeMdl(mdl);  
}

/*
 * @implemented
 */
HANDLE STDCALL
EngSecureMem(PVOID Address, ULONG Length)
{
  return MmSecureVirtualMemory(Address, Length, PAGE_READWRITE);
}

/*
 * @implemented
 */
VOID STDCALL
EngUnsecureMem(HANDLE Mem)
{
  return MmUnsecureVirtualMemory((PVOID) Mem);
}

/* EOF */
