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
/* $Id: font.c,v 1.3 2002/09/08 10:23:12 chorns Exp $
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

/* FUNCTIONS *****************************************************************/

DWORD
STDCALL
GetTabbedTextExtentA(
  HDC hDC,
  LPCSTR lpString,
  int nCount,
  int nTabPositions,
  CONST LPINT lpnTabStopPositions)
{
  return 0;
}

DWORD
STDCALL
GetTabbedTextExtentW(
  HDC hDC,
  LPCWSTR lpString,
  int nCount,
  int nTabPositions,
  CONST LPINT lpnTabStopPositions)
{
  return 0;
}
int
STDCALL
DrawTextA(
  HDC hDC,
  LPCSTR lpString,
  int nCount,
  LPRECT lpRect,
  UINT uFormat)
{
  return 0;
}

int
STDCALL
DrawTextExA(
  HDC hdc,
  LPSTR lpchText,
  int cchText,
  LPRECT lprc,
  UINT dwDTFormat,
  LPDRAWTEXTPARAMS lpDTParams)
{
  return 0;
}

int
STDCALL
DrawTextExW(
  HDC hdc,
  LPWSTR lpchText,
  int cchText,
  LPRECT lprc,
  UINT dwDTFormat,
  LPDRAWTEXTPARAMS lpDTParams)
{
  return 0;
}

int
STDCALL
DrawTextW(
  HDC hDC,
  LPCWSTR lpString,
  int nCount,
  LPRECT lpRect,
  UINT uFormat)
{
  return 0;
}
