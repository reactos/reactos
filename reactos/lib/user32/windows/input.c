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
/* $Id: input.c,v 1.8 2003/07/10 21:04:32 chorns Exp $
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
HKL STDCALL
ActivateKeyboardLayout(HKL hkl,
		       UINT Flags)
{
  UNIMPLEMENTED;
  return (HKL)0;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
BlockInput(WINBOOL fBlockIt)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
EnableWindow(HWND hWnd,
	     WINBOOL bEnable)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
SHORT STDCALL
GetAsyncKeyState(int vKey)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
HKL STDCALL
GetKeyboardLayout(DWORD idThread)
{
  UNIMPLEMENTED;
  return (HKL)0;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL GetInputState(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
UINT STDCALL
GetKBCodePage(VOID)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int STDCALL
GetKeyNameTextA(LONG lParam,
		LPSTR lpString,
		int nSize)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int STDCALL
GetKeyNameTextW(LONG lParam,
		LPWSTR lpString,
		int nSize)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
SHORT STDCALL
GetKeyState(int nVirtKey)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
UINT STDCALL
GetKeyboardLayoutList(int nBuff,
		      HKL FAR *lpList)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetKeyboardLayoutNameA(LPSTR pwszKLID)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetKeyboardLayoutNameW(LPWSTR pwszKLID)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetKeyboardState(PBYTE lpKeyState)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
int STDCALL
GetKeyboardType(int nTypeFlag)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetLastInputInfo(PLASTINPUTINFO plii)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
HKL STDCALL
LoadKeyboardLayoutA(LPCSTR pwszKLID,
		    UINT Flags)
{
  UNIMPLEMENTED;
  return (HKL)0;
}


/*
 * @unimplemented
 */
HKL STDCALL
LoadKeyboardLayoutW(LPCWSTR pwszKLID,
		    UINT Flags)
{
  UNIMPLEMENTED;
  return (HKL)0;
}


/*
 * @unimplemented
 */
UINT STDCALL
MapVirtualKeyA(UINT uCode,
	       UINT uMapType)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
UINT STDCALL
MapVirtualKeyExA(UINT uCode,
		 UINT uMapType,
		 HKL dwhkl)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
UINT STDCALL
MapVirtualKeyExW(UINT uCode,
		 UINT uMapType,
		 HKL dwhkl)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
UINT STDCALL
MapVirtualKeyW(UINT uCode,
	       UINT uMapType)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
OemKeyScan(WORD wOemChar)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
HWND STDCALL
SetFocus(HWND hWnd)
{
  return NtUserSetFocus(hWnd);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
SetKeyboardState(LPBYTE lpKeyState)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
int STDCALL
ToAscii(UINT uVirtKey,
	UINT uScanCode,
	CONST PBYTE lpKeyState,
	LPWORD lpChar,
	UINT uFlags)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int STDCALL
ToAsciiEx(UINT uVirtKey,
	  UINT uScanCode,
	  CONST PBYTE lpKeyState,
	  LPWORD lpChar,
	  UINT uFlags,
	  HKL dwhkl)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int STDCALL
ToUnicode(UINT wVirtKey,
	  UINT wScanCode,
	  CONST PBYTE lpKeyState,
	  LPWSTR pwszBuff,
	  int cchBuff,
	  UINT wFlags)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int STDCALL
ToUnicodeEx(UINT wVirtKey,
	    UINT wScanCode,
	    CONST PBYTE lpKeyState,
	    LPWSTR pwszBuff,
	    int cchBuff,
	    UINT wFlags,
	    HKL dwhkl)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
UnloadKeyboardLayout(HKL hkl)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
SHORT STDCALL
VkKeyScanA(CHAR ch)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
SHORT STDCALL
VkKeyScanExA(CHAR ch,
	     HKL dwhkl)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
SHORT STDCALL
VkKeyScanExW(WCHAR ch,
	     HKL dwhkl)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
SHORT STDCALL
VkKeyScanW(WCHAR ch)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
UINT
STDCALL
SendInput(
  UINT nInputs,
  LPINPUT pInputs,
  int cbSize)
{
  UNIMPLEMENTED;
  return 0;
}
