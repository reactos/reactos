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
/* $Id: mdi.c,v 1.2 2002/09/07 15:12:45 chorns Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/input.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

/* FUNCTIONS *****************************************************************/

HWND
STDCALL
CreateMDIWindowA(
  LPCSTR lpClassName,
  LPCSTR lpWindowName,
  DWORD dwStyle,
  int X,
  int Y,
  int nWidth,
  int nHeight,
  HWND hWndParent,
  HINSTANCE hInstance,
  LPARAM lParam)
{
  return (HWND)0;
}

HWND
STDCALL
CreateMDIWindowW(
  LPCWSTR lpClassName,
  LPCWSTR lpWindowName,
  DWORD dwStyle,
  int X,
  int Y,
  int nWidth,
  int nHeight,
  HWND hWndParent,
  HINSTANCE hInstance,
  LPARAM lParam)
{
  return (HWND)0;
}
LRESULT
STDCALL
DefMDIChildProcA(
  HWND hWnd,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

LRESULT
STDCALL
DefMDIChildProcW(
  HWND hWnd,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}
WINBOOL
STDCALL
TranslateMDISysAccel(
  HWND hWndClient,
  LPMSG lpMsg)
{
  return FALSE;
}
