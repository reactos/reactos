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
/* $Id: cursor.c,v 1.2 2002/09/07 15:12:45 chorns Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/cursor.c
 * PURPOSE:         Cursor
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

/* FUNCTIONS *****************************************************************/

WINBOOL STDCALL
CreateCaret(HWND hWnd,
	    HBITMAP hBitmap,
	    int nWidth,
	    int nHeight)
{
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
  
  return (HCURSOR)0;
}

WINBOOL STDCALL
DestroyCaret(VOID)
{
  return FALSE;
}

WINBOOL STDCALL
DestroyCursor(HCURSOR hCursor)
{
  return FALSE;
}

UINT STDCALL
GetCaretBlinkTime(VOID)
{
  return 0;
}

WINBOOL STDCALL
GetCaretPos(LPPOINT lpPoint)
{
  return FALSE;
}

WINBOOL STDCALL
GetClipCursor(LPRECT lpRect)
{
  return FALSE;
}

HCURSOR STDCALL
GetCursor(VOID)
{
  return (HCURSOR)0;
}

WINBOOL STDCALL
GetCursorInfo(PCURSORINFO pci)
{
  return FALSE;
}

WINBOOL STDCALL
GetCursorPos(LPPOINT lpPoint)
{
  return FALSE;
}

WINBOOL STDCALL
HideCaret(HWND hWnd)
{
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
  return(LoadImageW(0, lpFileName, IMAGE_CURSOR, 0, 0, 
		    LR_LOADFROMFILE | LR_DEFAULTSIZE));
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
  return FALSE;
}

WINBOOL STDCALL 
SetCaretPos(int X,
	    int Y)
{
  return FALSE;
}

HCURSOR STDCALL
SetCursor(HCURSOR hCursor)
{
  return (HCURSOR)0;
}

WINBOOL STDCALL
SetCursorPos(int X,
	     int Y)
{
  return FALSE;
}

WINBOOL STDCALL
SetSystemCursor(HCURSOR hcur,
		DWORD id)
{
  return FALSE;
}

WINBOOL STDCALL
ShowCaret(HWND hWnd)
{
  return FALSE;
}

int STDCALL
ShowCursor(WINBOOL bShow)
{
  return 0;
}
