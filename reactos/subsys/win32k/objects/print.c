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
/* $Id: print.c,v 1.13 2004/02/18 02:37:18 royce Exp $ */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/print.h>
#include <win32k/dc.h>
#include <include/error.h>
#include <internal/safe.h>

#define NDEBUG
#include <win32k/debug1.h>

INT
STDCALL
NtGdiAbortDoc(HDC  hDC)
{
  UNIMPLEMENTED;
}

INT
STDCALL
NtGdiEndDoc(HDC  hDC)
{
  UNIMPLEMENTED;
}

INT
STDCALL
NtGdiEndPage(HDC  hDC)
{
  UNIMPLEMENTED;
}

INT
STDCALL
NtGdiEscape(HDC  hDC,
                INT  Escape,
                INT  InSize,
                LPCSTR  InData,
                LPVOID  OutData)
{
  UNIMPLEMENTED;
}

INT
STDCALL
IntEngExtEscape(
   HSURF  Surface,
   INT    Escape,
   INT    InSize,
   LPVOID InData,
   INT    OutSize,
   LPVOID OutData)
{
   UNIMPLEMENTED;
   return -1;
}

INT
STDCALL
IntGdiExtEscape(
   PDC    dc,
   INT    Escape,
   INT    InSize,
   LPCSTR InData,
   INT    OutSize,
   LPSTR  OutData)
{
   if ( NULL == dc->DriverFunctions.Escape )
   {
      return IntEngExtEscape(
         dc->Surface,
         Escape,
         InSize,
         (PVOID)InData,
         OutSize,
         (PVOID)OutData);
   }
   else
   {
      return dc->DriverFunctions.Escape(
         dc->Surface,
         Escape,
         InSize,
         (PVOID)InData,
         OutSize,
         (PVOID)OutData );
   }
}

INT
STDCALL
NtGdiExtEscape(
   HDC    hDC,
   INT    Escape,
   INT    InSize,
   LPCSTR UnsafeInData,
   INT    OutSize,
   LPSTR  UnsafeOutData)
{
   PDC      pDC = DC_LockDc(hDC);
   LPVOID   SafeInData = NULL;
   LPVOID   SafeOutData = NULL;
   NTSTATUS Status;
   INT      Result;

   if ( pDC == NULL )
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return -1;
   }

   if ( InSize && UnsafeInData )
   {
      SafeInData = ExAllocatePool ( NonPagedPool, InSize );
      if ( !SafeInData )
      {
         DC_UnlockDc(hDC);
         SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
         return -1;
      }
      Status = MmCopyFromCaller ( SafeInData, UnsafeInData, InSize );
      if ( !NT_SUCCESS(Status) )
      {
         ExFreePool ( SafeInData );
         DC_UnlockDc(hDC);
         SetLastNtError(Status);
         return -1;
      }
   }

   if ( OutSize && UnsafeOutData )
   {
      SafeOutData = ExAllocatePool ( NonPagedPool, OutSize );
      if ( !SafeOutData )
      {
         if ( SafeInData )
            ExFreePool ( SafeInData );
         DC_UnlockDc(hDC);
         SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
         return -1;
      }
   }

   Result = IntGdiExtEscape ( pDC, Escape, InSize, SafeInData, OutSize, SafeOutData );

   DC_UnlockDc(hDC);

   if ( SafeInData )
      ExFreePool ( SafeInData );

   if ( SafeOutData )
   {
      Status = MmCopyToCaller ( UnsafeOutData, SafeOutData, OutSize );
      ExFreePool ( SafeOutData );
      if ( !NT_SUCCESS(Status) )
      {
         SetLastNtError(Status);
         return -1;
      }
   }

   return Result;
}

INT
STDCALL
NtGdiSetAbortProc(HDC  hDC,
                      ABORTPROC  AbortProc)
{
  UNIMPLEMENTED;
}

INT
STDCALL
NtGdiStartDoc(HDC  hDC,
                  CONST PDOCINFOW  di)
{
  UNIMPLEMENTED;
}

INT
STDCALL
NtGdiStartPage(HDC  hDC)
{
  UNIMPLEMENTED;
}
/* EOF */
