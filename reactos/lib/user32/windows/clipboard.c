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
/* $Id: clipboard.c,v 1.1 2002/06/13 20:36:40 dwelch Exp $
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

WINBOOL
STDCALL
CloseClipboard(VOID)
{
  return FALSE;
}
int
STDCALL
CountClipboardFormats(VOID)
{
  return 0;
}
WINBOOL
STDCALL
EmptyClipboard(VOID)
{
  return FALSE;
}
UINT
STDCALL
EnumClipboardFormats(
  UINT format)
{
  return 0;
}
HANDLE
STDCALL
GetClipboardData(
  UINT uFormat)
{
  return (HANDLE)0;
}

int
STDCALL
GetClipboardFormatNameA(
  UINT format,
  LPSTR lpszFormatName,
  int cchMaxCount)
{
  return 0;
}

int
STDCALL
GetClipboardFormatNameW(
  UINT format,
  LPWSTR lpszFormatName,
  int cchMaxCount)
{
  return 0;
}

HWND
STDCALL
GetClipboardOwner(VOID)
{
  return (HWND)0;
}

DWORD
STDCALL
GetClipboardSequenceNumber(VOID)
{
  return 0;
}

HWND
STDCALL
GetClipboardViewer(VOID)
{
  return (HWND)0;
}
HWND
STDCALL
GetOpenClipboardWindow(VOID)
{
  return (HWND)0;
}

int
STDCALL
GetPriorityClipboardFormat(
  UINT *paFormatPriorityList,
  int cFormats)
{
  return 0;
}
WINBOOL
STDCALL
IsClipboardFormatAvailable(
  UINT format)
{
  return FALSE;
}
WINBOOL
STDCALL
OpenClipboard(
  HWND hWndNewOwner)
{
  return FALSE;
}
UINT
STDCALL
RegisterClipboardFormatA(
  LPCSTR lpszFormat)
{
  return 0;
}

UINT
STDCALL
RegisterClipboardFormatW(
  LPCWSTR lpszFormat)
{
  return 0;
}
HANDLE
STDCALL
SetClipboardData(
  UINT uFormat,
  HANDLE hMem)
{
  return (HANDLE)0;
}

HWND
STDCALL
SetClipboardViewer(
  HWND hWndNewViewer)
{
  return (HWND)0;
}
WINBOOL
STDCALL
ChangeClipboardChain(
  HWND hWndRemove,
  HWND hWndNewNext)
{
  return FALSE;
}
