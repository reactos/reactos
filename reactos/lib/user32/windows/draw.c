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
/* $Id: draw.c,v 1.2 2002/09/01 20:39:55 dwelch Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/input.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>

/* GLOBALS *******************************************************************/

#define COLOR_MAX (28)

/* FUNCTIONS *****************************************************************/

WINBOOL
STDCALL
GrayStringA(
  HDC hDC,
  HBRUSH hBrush,
  GRAYSTRINGPROC lpOutputFunc,
  LPARAM lpData,
  int nCount,
  int X,
  int Y,
  int nWidth,
  int nHeight)
{
  return FALSE;
}

WINBOOL
STDCALL
GrayStringW(
  HDC hDC,
  HBRUSH hBrush,
  GRAYSTRINGPROC lpOutputFunc,
  LPARAM lpData,
  int nCount,
  int X,
  int Y,
  int nWidth,
  int nHeight)
{
  return FALSE;
}
WINBOOL
STDCALL
InvertRect(
  HDC hDC,
  CONST RECT *lprc)
{
  return FALSE;
}
LONG
STDCALL
TabbedTextOutA(
  HDC hDC,
  int X,
  int Y,
  LPCSTR lpString,
  int nCount,
  int nTabPositions,
  CONST LPINT lpnTabStopPositions,
  int nTabOrigin)
{
  return 0;
}

LONG
STDCALL
TabbedTextOutW(
  HDC hDC,
  int X,
  int Y,
  LPCWSTR lpString,
  int nCount,
  int nTabPositions,
  CONST LPINT lpnTabStopPositions,
  int nTabOrigin)
{
  return 0;
}
int
STDCALL
FrameRect(
  HDC hDC,
  CONST RECT *lprc,
  HBRUSH hbr)
{
  return 0;
}
WINBOOL
STDCALL
FlashWindow(
  HWND hWnd,
  WINBOOL bInvert)
{
  return FALSE;
}

WINBOOL
STDCALL
FlashWindowEx(
  PFLASHWINFO pfwi)
{
  return FALSE;
}

int STDCALL
FillRect(HDC hDC, CONST RECT *lprc, HBRUSH hbr)
{
  HBRUSH prevhbr;
  /*if (hbr <= (HBRUSH)(COLOR_MAX + 1))
    {
      hbr = GetSysColorBrush((INT)hbr - 1);
      }*/
  if ((prevhbr = SelectObject(hDC, hbr)) == NULL)
    {
      return(FALSE);
    }
  PatBlt(hDC, lprc->left, lprc->top, lprc->right - lprc->left,
	 lprc->bottom - lprc->top, PATCOPY);
  SelectObject(hDC, prevhbr);
  return(TRUE);
}

WINBOOL
STDCALL
DrawAnimatedRects(
  HWND hwnd,
  int idAni,
  CONST RECT *lprcFrom,
  CONST RECT *lprcTo)
{
  return FALSE;
}

WINBOOL
STDCALL
DrawCaption(
  HWND hwnd,
  HDC hdc,
  LPRECT lprc,
  UINT uFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
DrawEdge(
  HDC hdc,
  LPRECT qrc,
  UINT edge,
  UINT grfFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
DrawFocusRect(
  HDC hDC,
  CONST RECT *lprc)
{
  return FALSE;
}


WINBOOL
STDCALL
DrawStateA(
  HDC hdc,
  HBRUSH hbr,
  DRAWSTATEPROC lpOutputFunc,
  LPARAM lData,
  WPARAM wData,
  int x,
  int y,
  int cx,
  int cy,
  UINT fuFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
DrawStateW(
  HDC hdc,
  HBRUSH hbr,
  DRAWSTATEPROC lpOutputFunc,
  LPARAM lData,
  WPARAM wData,
  int x,
  int y,
  int cx,
  int cy,
  UINT fuFlags)
{
  return FALSE;
}
