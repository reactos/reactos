/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/* $Id: cursor.c,v 1.5 2003/05/12 19:30:00 jfilby Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/cursor.c
 * PURPOSE:         Cursor
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>

/* FUNCTIONS *****************************************************************/

WINBOOL STDCALL
CreateCaret(HWND hWnd,
	    HBITMAP hBitmap,
	    int nWidth,
	    int nHeight)
{
  UNIMPLEMENTED;
  return FALSE;
}

HCURSOR STDCALL
CreateCursor(HINSTANCE hInst,
	     int xHotSpot,
	     int yHotSpot,
	     int nWidth,
	     int nHeight,
	     CONST VOID *pvANDPlane,
	     CONST VOID *pvXORPlane)
{

  UNIMPLEMENTED;
  return (HCURSOR)0;
}

WINBOOL STDCALL
DestroyCaret(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
DestroyCursor(HCURSOR hCursor)
{
  UNIMPLEMENTED;
  return FALSE;
}

UINT STDCALL
GetCaretBlinkTime(VOID)
{
  UNIMPLEMENTED;
  return 0;
}

WINBOOL STDCALL
GetCaretPos(LPPOINT lpPoint)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
GetClipCursor(LPRECT lpRect)
{
  UNIMPLEMENTED;
  return FALSE;
}

HCURSOR STDCALL
GetCursor(VOID)
{
  UNIMPLEMENTED;
  return (HCURSOR)0;
}

WINBOOL STDCALL
GetCursorInfo(PCURSORINFO pci)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
GetCursorPos(LPPOINT lpPoint)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
HideCaret(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

HCURSOR STDCALL
LoadCursorA(HINSTANCE hInstance,
	    LPCSTR lpCursorName)
{
  return(LoadImageA(hInstance, lpCursorName, IMAGE_CURSOR, 0, 0,
		    LR_DEFAULTSIZE));
}

HCURSOR STDCALL
LoadCursorFromFileA(LPCSTR lpFileName)
{
  UNICODE_STRING FileName;
  HCURSOR Result;
  RtlCreateUnicodeStringFromAsciiz(&FileName, (LPSTR)lpFileName);
  Result = LoadImageW(0, FileName.Buffer, IMAGE_CURSOR, 0, 0, 
		      LR_LOADFROMFILE | LR_DEFAULTSIZE);
  RtlFreeUnicodeString(&FileName);
  return(Result);
}

HCURSOR STDCALL
LoadCursorFromFileW(LPCWSTR lpFileName)
{
  return(LoadImageW(0, lpFileName, IMAGE_CURSOR, 0, 0, 
		    LR_LOADFROMFILE | LR_DEFAULTSIZE));
}

HCURSOR STDCALL
LoadCursorW(HINSTANCE hInstance,
	    LPCWSTR lpCursorName)
{
  return(LoadImageW(hInstance, lpCursorName, IMAGE_CURSOR, 0, 0,
		    LR_DEFAULTSIZE));
}

WINBOOL STDCALL
SetCaretBlinkTime(UINT uMSeconds)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
SetCaretPos(int X,
	    int Y)
{
  UNIMPLEMENTED;
  return FALSE;
}

HCURSOR STDCALL
SetCursor(HCURSOR hCursor)
{
  UNIMPLEMENTED;
  return (HCURSOR)0;
}

WINBOOL STDCALL
SetCursorPos(int X,
	     int Y)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
SetSystemCursor(HCURSOR hcur,
		DWORD id)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
ShowCaret(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

int STDCALL
ShowCursor(WINBOOL bShow)
{
  UNIMPLEMENTED;
  return 0;
}
