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
/* $Id: input.c,v 1.22 2004/01/23 23:38:26 ekohl Exp $
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
#include <wchar.h>

/* FUNCTIONS *****************************************************************/


/*
 * @implemented
 */
BOOL
STDCALL
DragDetect(
  HWND hWnd,
  POINT pt)
{
  return NtUserDragDetect(hWnd, pt.x, pt.y);
}


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
BOOL STDCALL
BlockInput(BOOL fBlockIt)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
BOOL STDCALL
EnableWindow(HWND hWnd,
	     BOOL bEnable)
{
    LONG Style = NtUserGetWindowLong(hWnd, GWL_STYLE, FALSE);
    Style = bEnable ? Style & ~WS_DISABLED : Style | WS_DISABLED;
    NtUserSetWindowLong(hWnd, GWL_STYLE, Style, FALSE);
    
    SendMessageA(hWnd, WM_ENABLE, (LPARAM) IsWindowEnabled(hWnd), 0);
    
    // Return nonzero if it was disabled, or zero if it wasn't:
    return IsWindowEnabled(hWnd);
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
 * @implemented
 */
UINT
STDCALL
GetDoubleClickTime(VOID)
{
  return NtUserGetDoubleClickTime();
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
UINT STDCALL
GetKBCodePage(VOID)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @implemented
 */
int STDCALL
GetKeyNameTextA(LONG lParam,
		LPSTR lpString,
		int nSize)
{
  LPWSTR intermediateString = 
    HeapAlloc(GetProcessHeap(),0,nSize * sizeof(WCHAR));
  int ret = 0;
  UINT wstrLen = 0;
  BOOL defChar = FALSE;

  if( !intermediateString ) return 0;
  ret = GetKeyNameTextW(lParam,intermediateString,nSize);
  if( ret == 0 ) { lpString[0] = 0; return 0; }
  
  wstrLen = wcslen( intermediateString );
  ret = WideCharToMultiByte(CP_ACP, 0, 
			    intermediateString, wstrLen,
			    lpString, nSize, ".", &defChar );
  lpString[ret] = 0;
  HeapFree(GetProcessHeap(),0,intermediateString);

  return ret;
}

/*
 * @implemented
 */
int STDCALL
GetKeyNameTextW(LONG lParam,
		LPWSTR lpString,
		int nSize)
{
  return NtUserGetKeyNameText( lParam, lpString, nSize );
}


/*
 * @implemented
 */
SHORT STDCALL
GetKeyState(int nVirtKey)
{
 return (SHORT) NtUserGetKeyState((DWORD) nVirtKey);
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
BOOL STDCALL
GetKeyboardLayoutNameA(LPSTR pwszKLID)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
GetKeyboardLayoutNameW(LPWSTR pwszKLID)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
GetKeyboardState(PBYTE lpKeyState)
{
  
  return (BOOL) NtUserGetKeyboardState((LPBYTE) lpKeyState);
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
BOOL STDCALL
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
 * @implemented
 */
UINT STDCALL
MapVirtualKeyA(UINT uCode,
	       UINT uMapType)
{
  return MapVirtualKeyExA( uCode, uMapType, GetKeyboardLayout( 0 ) );
}


/*
 * @implemented
 */
UINT STDCALL
MapVirtualKeyExA(UINT uCode,
		 UINT uMapType,
		 HKL dwhkl)
{
  return MapVirtualKeyExW( uCode, uMapType, dwhkl );
}


/*
 * @implemented
 */
UINT STDCALL
MapVirtualKeyExW(UINT uCode,
		 UINT uMapType,
		 HKL dwhkl)
{
  return NtUserMapVirtualKeyEx( uCode, uMapType, 0, dwhkl );
}


/*
 * @implemented
 */
UINT STDCALL
MapVirtualKeyW(UINT uCode,
	       UINT uMapType)
{
  return MapVirtualKeyExW( uCode, uMapType, GetKeyboardLayout( 0 ) );
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
 * @implemented
 */
BOOL STDCALL
RegisterHotKey(HWND hWnd,
	       int id,
	       UINT fsModifiers,
	       UINT vk)
{
  return (BOOL)NtUserRegisterHotKey(hWnd,
                                       id,
                                       fsModifiers,
                                       vk);
}


/*
 * @implemented
 */
BOOL STDCALL
SetDoubleClickTime(UINT uInterval)
{
  return (BOOL)NtUserSystemParametersInfo(SPI_SETDOUBLECLICKTIME,
                                             uInterval,
                                             NULL,
                                             0);
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
BOOL STDCALL
SetKeyboardState(LPBYTE lpKeyState)
{
 return (BOOL) NtUserSetKeyboardState((LPBYTE)lpKeyState);
}


/*
 * @implemented
 */
BOOL
STDCALL
SwapMouseButton(
  BOOL fSwap)
{
  return (BOOL)NtUserCallOneParam((DWORD)fSwap, 
                                     ONEPARAM_ROUTINE_SWAPMOUSEBUTTON);
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
 * @implemented
 */
int STDCALL
ToUnicode(UINT wVirtKey,
	  UINT wScanCode,
	  CONST PBYTE lpKeyState,
	  LPWSTR pwszBuff,
	  int cchBuff,
	  UINT wFlags)
{
  return ToUnicodeEx( wVirtKey, wScanCode, lpKeyState, pwszBuff, cchBuff, 
		      wFlags, 0 );
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
  return NtUserToUnicodeEx( wVirtKey, wScanCode, lpKeyState, pwszBuff, cchBuff,
			    wFlags, dwhkl );
}


/*
 * @unimplemented
 */
BOOL STDCALL
UnloadKeyboardLayout(HKL hkl)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
BOOL STDCALL
UnregisterHotKey(HWND hWnd,
		 int id)
{
  return (BOOL)NtUserUnregisterHotKey(hWnd, id);
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

/*
 * Private call for CSRSS
 */
VOID
STDCALL
PrivateCsrssRegisterPrimitive(VOID)
{
  NtUserCallNoParam(NOPARAM_ROUTINE_REGISTER_PRIMITIVE);
}

/*
 * Another private call for CSRSS
 */
VOID
STDCALL
PrivateCsrssAcquireOrReleaseInputOwnership(BOOL Release)
{
  NtUserAcquireOrReleaseInputOwnership(Release);
}

/* EOF */
