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
/* $Id: input.c,v 1.5 2002/09/08 10:23:12 chorns Exp $
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

HKL STDCALL
ActivateKeyboardLayout(HKL hkl,
		       UINT Flags)
{
  return (HKL)0;
}

WINBOOL STDCALL
BlockInput(WINBOOL fBlockIt)
{
  return FALSE;
}

WINBOOL STDCALL
EnableWindow(HWND hWnd,
	     WINBOOL bEnable)
{
  return FALSE;
}

SHORT STDCALL
GetAsyncKeyState(int vKey)
{
  return 0;
}

HKL STDCALL
GetKeyboardLayout(DWORD idThread)
{
  return (HKL)0;
}

WINBOOL STDCALL GetInputState(VOID)
{
  return FALSE;
}

UINT STDCALL
GetKBCodePage(VOID)
{
  return 0;
}

int STDCALL
GetKeyNameTextA(LONG lParam,
		LPSTR lpString,
		int nSize)
{
  return 0;
}

int STDCALL
GetKeyNameTextW(LONG lParam,
		LPWSTR lpString,
		int nSize)
{
  return 0;
}

SHORT STDCALL
GetKeyState(int nVirtKey)
{
  return 0;
}

UINT STDCALL
GetKeyboardLayoutList(int nBuff,
		      HKL FAR *lpList)
{
  return 0;
}

WINBOOL STDCALL
GetKeyboardLayoutNameA(LPSTR pwszKLID)
{
  return FALSE;
}

WINBOOL STDCALL
GetKeyboardLayoutNameW(LPWSTR pwszKLID)
{
  return FALSE;
}

WINBOOL STDCALL
GetKeyboardState(PBYTE lpKeyState)
{
  return FALSE;
}

int STDCALL
GetKeyboardType(int nTypeFlag)
{
  return 0;
}

WINBOOL STDCALL
GetLastInputInfo(PLASTINPUTINFO plii)
{
  return FALSE;
}

HKL STDCALL
LoadKeyboardLayoutA(LPCSTR pwszKLID,
		    UINT Flags)
{
  return (HKL)0;
}

HKL STDCALL
LoadKeyboardLayoutW(LPCWSTR pwszKLID,
		    UINT Flags)
{
  return (HKL)0;
}

UINT STDCALL
MapVirtualKeyA(UINT uCode,
	       UINT uMapType)
{
  return 0;
}

UINT STDCALL
MapVirtualKeyExA(UINT uCode,
		 UINT uMapType,
		 HKL dwhkl)
{
  return 0;
}

UINT STDCALL
MapVirtualKeyExW(UINT uCode,
		 UINT uMapType,
		 HKL dwhkl)
{
  return 0;
}

UINT STDCALL
MapVirtualKeyW(UINT uCode,
	       UINT uMapType)
{
  return 0;
}

DWORD STDCALL
OemKeyScan(WORD wOemChar)
{
  return 0;
}

WINBOOL STDCALL
SetKeyboardState(LPBYTE lpKeyState)
{
  return FALSE;
}

int STDCALL
ToAscii(UINT uVirtKey,
	UINT uScanCode,
	CONST PBYTE lpKeyState,
	LPWORD lpChar,
	UINT uFlags)
{
  return 0;
}

int STDCALL
ToAsciiEx(UINT uVirtKey,
	  UINT uScanCode,
	  CONST PBYTE lpKeyState,
	  LPWORD lpChar,
	  UINT uFlags,
	  HKL dwhkl)
{
  return 0;
}

int STDCALL
ToUnicode(UINT wVirtKey,
	  UINT wScanCode,
	  CONST PBYTE lpKeyState,
	  LPWSTR pwszBuff,
	  int cchBuff,
	  UINT wFlags)
{
  return 0;
}

int STDCALL
ToUnicodeEx(UINT wVirtKey,
	    UINT wScanCode,
	    CONST PBYTE lpKeyState,
	    LPWSTR pwszBuff,
	    int cchBuff,
	    UINT wFlags,
	    HKL dwhkl)
{
  return 0;
}

WINBOOL STDCALL
UnloadKeyboardLayout(HKL hkl)
{
  return FALSE;
}

SHORT STDCALL
VkKeyScanA(CHAR ch)
{
  return 0;
}

SHORT STDCALL
VkKeyScanExA(CHAR ch,
	     HKL dwhkl)
{
  return 0;
}

SHORT STDCALL
VkKeyScanExW(WCHAR ch,
	     HKL dwhkl)
{
  return 0;
}

SHORT STDCALL
VkKeyScanW(WCHAR ch)
{
  return 0;
}

UINT
STDCALL
SendInput(
  UINT nInputs,
  LPINPUT pInputs,
  int cbSize)
{
  return 0;
}
