/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/class.c
 * PURPOSE:         Class Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtUserGetClassInfoEx(DWORD Param1,
                     DWORD Param2,
                     DWORD Param3,
                     DWORD Param4,
                     DWORD Param5)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtUserGetClassName(HWND hWnd,
                   BOOL Real,
                   PUNICODE_STRING ClassName)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserGetWOWClass(DWORD Unknown0,
                  DWORD Unknown1)
{
    UNIMPLEMENTED;
    return 0;
}

RTL_ATOM
APIENTRY
NtUserRegisterClassExWOW(WNDCLASSEXW* lpwcx,
                         PUNICODE_STRING pustrClassName,
                         PUNICODE_STRING pustrCNVersion,
                         PCLSMENUNAME pClassMenuName,
                         DWORD fnID,
                         DWORD Flags,
                         LPDWORD pWow)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserUnregisterClass(PUNICODE_STRING ClassNameOrAtom,
                      HINSTANCE hInstance,
                      PCLSMENUNAME pClassMenuName)
{
    UNIMPLEMENTED;
    return FALSE;
}

ULONG_PTR
APIENTRY
NtUserSetClassLong(HWND  hWnd,
                   INT Offset,
                   ULONG_PTR dwNewLong,
                   BOOL Ansi)
{
    UNIMPLEMENTED;
    return 0;
}

WORD
APIENTRY
NtUserSetClassWord(HWND hWnd,
                   INT nIndex,
                   WORD wNewWord)
{
    UNIMPLEMENTED;
    return 0;
}
