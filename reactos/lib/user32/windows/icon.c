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
/* $Id: icon.c,v 1.3 2002/09/08 10:23:12 chorns Exp $
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

HICON
STDCALL
CopyIcon(
  HICON hIcon)
{
  return (HICON)0;
}
HICON
STDCALL
CreateIcon(
  HINSTANCE hInstance,
  int nWidth,
  int nHeight,
  BYTE cPlanes,
  BYTE cBitsPixel,
  CONST BYTE *lpbANDbits,
  CONST BYTE *lpbXORbits)
{
  return (HICON)0;
}

HICON
STDCALL
CreateIconFromResource(
  PBYTE presbits,
  DWORD dwResSize,
  WINBOOL fIcon,
  DWORD dwVer)
{
  return (HICON)0;
}

HICON
STDCALL
CreateIconFromResourceEx(
  PBYTE pbIconBits,
  DWORD cbIconBits,
  WINBOOL fIcon,
  DWORD dwVersion,
  int cxDesired,
  int cyDesired,
  UINT uFlags)
{
  return (HICON)0;
}

HICON
STDCALL
CreateIconIndirect(
  PICONINFO piconinfo)
{
  return (HICON)0;
}
WINBOOL
STDCALL
DestroyIcon(
  HICON hIcon)
{
  return FALSE;
}
WINBOOL
STDCALL
DrawIcon(
  HDC hDC,
  int X,
  int Y,
  HICON hIcon)
{
  return FALSE;
}

WINBOOL
STDCALL
DrawIconEx(
  HDC hdc,
  int xLeft,
  int yTop,
  HICON hIcon,
  int cxWidth,
  int cyWidth,
  UINT istepIfAniCur,
  HBRUSH hbrFlickerFreeDraw,
  UINT diFlags)
{
  return FALSE;
}
WINBOOL
STDCALL
GetIconInfo(
  HICON hIcon,
  PICONINFO piconinfo)
{
  return FALSE;
}
HICON
STDCALL
LoadIconA(
  HINSTANCE hInstance,
  LPCSTR lpIconName)
{
  return (HICON)0;
}

HICON
STDCALL
LoadIconW(
  HINSTANCE hInstance,
  LPCWSTR lpIconName)
{
  return (HICON)0;
}
int
STDCALL
LookupIconIdFromDirectory(
  PBYTE presbits,
  WINBOOL fIcon)
{
  return 0;
}

int
STDCALL
LookupIconIdFromDirectoryEx(
  PBYTE presbits,
  WINBOOL fIcon,
  int cxDesired,
  int cyDesired,
  UINT Flags)
{
  return 0;
}
