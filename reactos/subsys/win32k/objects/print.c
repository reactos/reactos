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
/* $Id: print.c,v 1.9 2003/05/18 17:16:18 ea Exp $ */
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/print.h>

#define NDEBUG
#include <win32k/debug1.h>

INT
STDCALL
W32kAbortDoc(HDC  hDC)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kEndDoc(HDC  hDC)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kEndPage(HDC  hDC)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kEscape(HDC  hDC,
                INT  Escape,
                INT  InSize,
                LPCSTR  InData,
                LPVOID  OutData)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kExtEscape(HDC  hDC,
                   INT  Escape,
                   INT  InSize,
                   LPCSTR  InData,
                   INT  OutSize,
                   LPSTR  OutData)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kSetAbortProc(HDC  hDC,
                      ABORTPROC  AbortProc)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kStartDoc(HDC  hDC,
                  CONST PDOCINFO  di)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kStartPage(HDC  hDC)
{
  UNIMPLEMENTED;
}
/* EOF */
