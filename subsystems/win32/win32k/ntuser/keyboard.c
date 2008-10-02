/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/keyboard.c
 * PURPOSE:         Keyboard Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

HKL
APIENTRY
NtUserActivateKeyboardLayout(HKL hKl,
                             ULONG Flags)
{
    UNIMPLEMENTED;
    return NULL;
}

SHORT
APIENTRY
NtUserGetAsyncKeyState(INT Key)
{
    UNIMPLEMENTED;
    return 0;
}

INT
APIENTRY
NtUserGetKeyNameText(LONG lParam,
                     PWSTR String,
                     INT nSize )
{
    UNIMPLEMENTED;
    return 0;
}

SHORT
APIENTRY
NtUserGetKeyState(INT VirtKey)
{
    UNIMPLEMENTED;
    return 0;
}

UINT
APIENTRY
NtUserGetKeyboardLayoutList(INT nItems,
                            HKL *pHklBuff)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserGetKeyboardLayoutName(LPWSTR lpszName)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserGetKeyboardState(PBYTE pKeyState)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserSetKeyboardState(LPBYTE lpKeyState)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserRegisterHotKey(HWND hWnd,
		             INT id,
		             UINT fsModifiers,
		             UINT vk)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserUnregisterHotKey(HWND hWnd,
		               INT id)
{
    UNIMPLEMENTED;
    return FALSE;
}

HKL
STDCALL
NtUserLoadKeyboardLayoutEx(IN HANDLE Handle,
                           IN DWORD offTable,
                           IN PUNICODE_STRING puszKeyboardName,
                           IN HKL hKL,
                           IN PUNICODE_STRING puszKLID,
                           IN DWORD dwKLID,
                           IN UINT Flags)
{
    UNIMPLEMENTED;
	return NULL;
}

BOOL
APIENTRY
NtUserUnloadKeyboardLayout(HKL hKl)
{
    UNIMPLEMENTED;
    return FALSE;
}

UINT
APIENTRY
NtUserMapVirtualKeyEx(UINT uCode,
		              UINT uMapType,
		              HKL dwHKL,
		              BOOL Unknown)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserVkKeyScanEx(WCHAR wChar,
                  HKL KeyboardLayout,
                  BOOL Unknown)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserSetConsoleReserveKeys(DWORD Unknown0,
                            DWORD Unknown1)
{
    UNIMPLEMENTED;
    return 0;
}

HWND
APIENTRY
NtUserSetFocus(HWND hWnd)
{
    UNIMPLEMENTED;
    return NULL;
}
