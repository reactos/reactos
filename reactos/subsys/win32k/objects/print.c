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
/* $Id: print.c,v 1.12 2004/02/08 16:16:24 navaraf Exp $ */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/print.h>
#include <win32k/dc.h>

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

INT STDCALL
NtGdiExtEscape(
   HDC hDC,
   INT Escape,
   INT InSize,
   LPCSTR InData,
   INT OutSize,
   LPSTR OutData)
{
   PDC pDC = DC_LockDc(hDC);
   INT Result;

   if (pDC == NULL)
   {
      return -1;
   }

   Result = pDC->DriverFunctions.Escape(
      pDC->Surface,
      Escape,
      InSize,
      (PVOID)InData,
      OutSize,
      (PVOID)OutData);

   DC_UnlockDc(hDC);

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
