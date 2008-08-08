/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/painting.c
 * PURPOSE:         Various Painting Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

HDC
APIENTRY
NtUserBeginPaint(HWND hWnd,
                 PAINTSTRUCT* lPs)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserEndPaint(HWND hWnd,
               CONST PAINTSTRUCT* lPs)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserBitBltSysBmp(HDC hdc,
                   INT nXDest,
                   INT nYDest,
                   INT nWidth,
                   INT nHeight,
                   INT nXSrc,
                   INT nYSrc,
                   DWORD dwRop)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserDrawAnimatedRects(HWND hwnd,
                        INT idAni,
                        RECT *lprcFrom,
                        RECT *lprcTo)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserDrawCaption(HWND hWnd,
                  HDC hDc,
                  LPCRECT lpRc,
                  UINT uFlags)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserDrawCaptionTemp(HWND hWnd,
                      HDC hDC,
                      LPCRECT lpRc,
                      HFONT hFont,
                      HICON hIcon,
                      CONST PUNICODE_STRING str,
                      UINT uFlags)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserExcludeUpdateRgn(HDC hDC,
                       HWND hWnd)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserGetUpdateRect(HWND hWnd,
                    LPRECT lpRect,
                    BOOL fErase)
{
    UNIMPLEMENTED;
    return 0;
}

INT
APIENTRY
NtUserGetUpdateRgn(HWND hWnd,
                   HRGN hRgn,
                   BOOL bErase)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserInvalidateRect(HWND hWnd,
                     CONST RECT *lpRect,
                     BOOL bErase)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserInvalidateRgn(HWND hWnd,
                    HRGN hRgn,
                    BOOL bErase)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserValidateRect(HWND hWnd,
                   CONST RECT *lpRect)
{
    UNIMPLEMENTED;
    return FALSE;
}
