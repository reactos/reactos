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
/* $Id: cursor.c,v 1.6 2003/07/10 21:04:31 chorns Exp $
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

/*
 * @unimplemented
 */
WINBOOL STDCALL
CreateCaret(HWND hWnd,
	    HBITMAP hBitmap,
	    int nWidth,
	    int nHeight)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
WINBOOL STDCALL
DestroyCaret(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
DestroyCursor(HCURSOR hCursor)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
UINT STDCALL
GetCaretBlinkTime(VOID)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetCaretPos(LPPOINT lpPoint)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetClipCursor(LPRECT lpRect)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
HCURSOR STDCALL
GetCursor(VOID)
{
  UNIMPLEMENTED;
  return (HCURSOR)0;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetCursorInfo(PCURSORINFO pci)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetCursorPos(LPPOINT lpPoint)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
HideCaret(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
HCURSOR STDCALL
LoadCursorA(HINSTANCE hInstance,
	    LPCSTR lpCursorName)
{
  return(LoadImageA(hInstance, lpCursorName, IMAGE_CURSOR, 0, 0,
		    LR_DEFAULTSIZE));
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
HCURSOR STDCALL
LoadCursorFromFileW(LPCWSTR lpFileName)
{
  return(LoadImageW(0, lpFileName, IMAGE_CURSOR, 0, 0, 
		    LR_LOADFROMFILE | LR_DEFAULTSIZE));
}


/*
 * @implemented
 */
HCURSOR STDCALL
LoadCursorW(HINSTANCE hInstance,
	    LPCWSTR lpCursorName)
{
  return(LoadImageW(hInstance, lpCursorName, IMAGE_CURSOR, 0, 0,
		    LR_DEFAULTSIZE));
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
SetCaretBlinkTime(UINT uMSeconds)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
SetCaretPos(int X,
	    int Y)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
HCURSOR STDCALL
SetCursor(HCURSOR hCursor)
{
  UNIMPLEMENTED;
  return (HCURSOR)0;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
SetCursorPos(int X,
	     int Y)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
SetSystemCursor(HCURSOR hcur,
		DWORD id)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
ShowCaret(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
int STDCALL
ShowCursor(WINBOOL bShow)
{
  UNIMPLEMENTED;
  return 0;
}
