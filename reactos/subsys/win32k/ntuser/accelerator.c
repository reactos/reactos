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
/* $Id: accelerator.c,v 1.2 2003/12/07 14:21:00 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/class.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
/* INCLUDES ******************************************************************/

#include <roskrnl.h>
#include <win32k/win32k.h>
#include <internal/safe.h>
#include <napi/win32.h>
#include <include/error.h>
#include <include/winsta.h>
#include <include/object.h>
#include <include/guicheck.h>
#include <include/window.h>
#include <include/accelerator.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
InitAcceleratorImpl(VOID)
{
  return(STATUS_SUCCESS);
}

NTSTATUS FASTCALL
CleanupAcceleratorImpl(VOID)
{
  return(STATUS_SUCCESS);
}

int
STDCALL
NtUserCopyAcceleratorTable(
  HACCEL Table,
  LPACCEL Entries,
  int EntriesCount)
{
  PWINSTATION_OBJECT WindowStation;
  PACCELERATOR_TABLE AcceleratorTable;
  NTSTATUS Status;
  int Ret;
  
  Status = IntValidateWindowStationHandle(NtUserGetProcessWindowStation(),
    UserMode,
	0,
	&WindowStation);
  if (!NT_SUCCESS(Status))
  {
    SetLastNtError(STATUS_ACCESS_DENIED);
    return 0;
  }
  
  Status = ObmReferenceObjectByHandle(WindowStation->HandleTable,
    Table,
    otAcceleratorTable,
    (PVOID*)&AcceleratorTable);
  if (!NT_SUCCESS(Status))
  {
    SetLastWin32Error(ERROR_INVALID_ACCEL_HANDLE);
    ObDereferenceObject(WindowStation);
    return 0;
  }
  
  if(Entries)
  {
    Ret = max(EntriesCount, AcceleratorTable->Count);
    Status = MmCopyToCaller(Entries, AcceleratorTable->Table, Ret * sizeof(ACCEL));
    if (!NT_SUCCESS(Status))
    {
	  ObmDereferenceObject(AcceleratorTable);
      ObDereferenceObject(WindowStation);
      SetLastNtError(Status);
      return 0;
    }
  }
  else
  {
    Ret = AcceleratorTable->Count;
  }
  
  ObmDereferenceObject(AcceleratorTable);
  ObDereferenceObject(WindowStation);

  return Ret;
}

HACCEL
STDCALL
NtUserCreateAcceleratorTable(
  LPACCEL Entries,
  SIZE_T EntriesCount)
{
  PWINSTATION_OBJECT WindowStation;
  PACCELERATOR_TABLE AcceleratorTable;
  NTSTATUS Status;
  HACCEL Handle;

  DbgPrint("NtUserCreateAcceleratorTable(Entries %p, EntriesCount %d)\n",
    Entries, EntriesCount);

  Status = IntValidateWindowStationHandle(NtUserGetProcessWindowStation(),
    UserMode,
	0,
	&WindowStation);
  if (!NT_SUCCESS(Status))
  {
    SetLastNtError(STATUS_ACCESS_DENIED);
    DbgPrint("E1\n");
    return FALSE;
  }

  AcceleratorTable = ObmCreateObject(
    WindowStation->HandleTable,
    (PHANDLE)&Handle,
    otAcceleratorTable,
	sizeof(ACCELERATOR_TABLE));
  if (AcceleratorTable == NULL)
  {
    ObDereferenceObject(WindowStation);
    SetLastNtError(STATUS_NO_MEMORY);
    DbgPrint("E2\n");
    return (HACCEL) 0;
  }

  AcceleratorTable->Count = EntriesCount;
  if (AcceleratorTable->Count > 0)
  {
	AcceleratorTable->Table = ExAllocatePool(PagedPool, EntriesCount * sizeof(ACCEL));
	if (AcceleratorTable->Table == NULL)
	{
		ObmCloseHandle(WindowStation->HandleTable, Handle);
		ObDereferenceObject(WindowStation);
		SetLastNtError(Status);
		DbgPrint("E3\n");
		return (HACCEL) 0;
	}

    Status = MmCopyFromCaller(AcceleratorTable->Table, Entries, EntriesCount * sizeof(ACCEL));
    if (!NT_SUCCESS(Status))
    {
	  ExFreePool(AcceleratorTable->Table);
	  ObmCloseHandle(WindowStation->HandleTable, Handle);
      ObDereferenceObject(WindowStation);
      SetLastNtError(Status);
      DbgPrint("E4\n");
      return (HACCEL) 0;
    }
  }

  ObDereferenceObject(WindowStation);

  /* FIXME: Save HandleTable in a list somewhere so we can clean it up again */

  DbgPrint("NtUserCreateAcceleratorTable(Entries %p, EntriesCount %d) = %x end\n",
    Entries, EntriesCount, Handle);

  return (HACCEL) Handle;
}

BOOLEAN
STDCALL
NtUserDestroyAcceleratorTable(
  HACCEL Table)
{
  PWINSTATION_OBJECT WindowStation;
  PACCELERATOR_TABLE AcceleratorTable;
  NTSTATUS Status;
  HACCEL Handle;

  /* FIXME: If the handle table is from a call to LoadAcceleratorTable, decrement it's
     usage count (and return TRUE).
	 FIXME: Destroy only tables created using CreateAcceleratorTable.
   */

  DbgPrint("NtUserDestroyAcceleratorTable(Table %x)\n",
    Table);

  Status = IntValidateWindowStationHandle(NtUserGetProcessWindowStation(),
    UserMode,
	0,
	&WindowStation);
  if (!NT_SUCCESS(Status))
  {
    SetLastNtError(STATUS_ACCESS_DENIED);
    DbgPrint("E1\n");
    return FALSE;
  }

  Status = ObmReferenceObjectByHandle(WindowStation->HandleTable,
    Table,
    otAcceleratorTable,
    (PVOID*)&AcceleratorTable);
  if (!NT_SUCCESS(Status))
  {
    SetLastNtError(STATUS_INVALID_HANDLE);
    ObDereferenceObject(WindowStation);
    DbgPrint("E2\n");
    return FALSE;
  }

  if (AcceleratorTable->Table != NULL)
  {
    ExFreePool(AcceleratorTable->Table);
  }

  ObmCloseHandle(WindowStation->HandleTable, Handle);

  ObDereferenceObject(WindowStation);

  DbgPrint("NtUserDestroyAcceleratorTable(Table %x)\n",
    Table);

  return TRUE;
}

int
STDCALL
NtUserTranslateAccelerator(
  HWND Window,
  HACCEL Table,
  LPMSG Message)
{
  UNIMPLEMENTED

  return 0;
}
