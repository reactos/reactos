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
/* $Id: clipboard.c,v 1.5 2003/07/10 21:04:31 chorns Exp $
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

/*
 * @unimplemented
 */
WINBOOL
STDCALL
CloseClipboard(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
int
STDCALL
CountClipboardFormats(VOID)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EmptyClipboard(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
UINT
STDCALL
EnumClipboardFormats(
  UINT format)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
HANDLE
STDCALL
GetClipboardData(
  UINT uFormat)
{
  UNIMPLEMENTED;
  return (HANDLE)0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetClipboardFormatNameA(
  UINT format,
  LPSTR lpszFormatName,
  int cchMaxCount)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetClipboardFormatNameW(
  UINT format,
  LPWSTR lpszFormatName,
  int cchMaxCount)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
HWND
STDCALL
GetClipboardOwner(VOID)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetClipboardSequenceNumber(VOID)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
HWND
STDCALL
GetClipboardViewer(VOID)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
HWND
STDCALL
GetOpenClipboardWindow(VOID)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetPriorityClipboardFormat(
  UINT *paFormatPriorityList,
  int cFormats)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
IsClipboardFormatAvailable(
  UINT format)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
OpenClipboard(
  HWND hWndNewOwner)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
UINT
STDCALL
RegisterClipboardFormatA(
  LPCSTR lpszFormat)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
UINT
STDCALL
RegisterClipboardFormatW(
  LPCWSTR lpszFormat)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
HANDLE
STDCALL
SetClipboardData(
  UINT uFormat,
  HANDLE hMem)
{
  UNIMPLEMENTED;
  return (HANDLE)0;
}


/*
 * @unimplemented
 */
HWND
STDCALL
SetClipboardViewer(
  HWND hWndNewViewer)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
ChangeClipboardChain(
  HWND hWndRemove,
  HWND hWndNewNext)
{
  UNIMPLEMENTED;
  return FALSE;
}
