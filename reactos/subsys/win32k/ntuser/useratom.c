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
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          User Atom helper routines
 * FILE:             subsys/win32k/ntuser/useratom.c
 * PROGRAMER:        Filip Navara <xnavara@volny.cz>
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

RTL_ATOM FASTCALL
IntAddAtom(LPWSTR AtomName)
{
   PWINSTATION_OBJECT WinStaObject;
   NTSTATUS Status;
   RTL_ATOM Atom;

   Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
      KernelMode, 0, &WinStaObject);
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);      
      return (RTL_ATOM)0;
   }
   Status = RtlAddAtomToAtomTable(WinStaObject->AtomTable,
      AtomName, &Atom);
   ObDereferenceObject(WinStaObject);
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);      
      return (RTL_ATOM)0;
   }
   return Atom;
}

ULONG FASTCALL
IntGetAtomName(RTL_ATOM nAtom, LPWSTR lpBuffer, ULONG nSize)
{
   PWINSTATION_OBJECT WinStaObject;
   NTSTATUS Status;
   ULONG Size = nSize;

   Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
      KernelMode, 0, &WinStaObject);
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);      
      return 0;
   }
   Status = RtlQueryAtomInAtomTable(WinStaObject->AtomTable,
      nAtom, NULL, NULL, lpBuffer, &Size);
   if (Size < nSize)
      *(lpBuffer + Size) = 0;
   ObDereferenceObject(WinStaObject);
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);      
      return 0;
   }
   return Size;
}

/* EOF */
