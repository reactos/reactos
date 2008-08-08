/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/icon.c
 * PURPOSE:         Icon Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtUserDrawIconEx(HDC hdc,
                 INT xLeft,
                 INT yTop,
                 HICON hIcon,
                 INT cxWidth,
                 INT cyWidth,
                 UINT istepIfAniCur,
                 HBRUSH hbrFlickerFreeDraw,
                 UINT diFlags,
                 DWORD Unknown0,
                 DWORD Unknown1)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserGetIconInfo(HANDLE hCurIcon,
                  PICONINFO IconInfo,
                  PUNICODE_STRING lpInstName,
                  PUNICODE_STRING lpResName,
                  LPDWORD pbpp,
                  BOOL bInternal)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserGetIconSize(HANDLE Handle,
                  UINT istepIfAniCur,
                  PLONG plcx,
                  PLONG plcy)
{
    UNIMPLEMENTED;
    return FALSE;
}
