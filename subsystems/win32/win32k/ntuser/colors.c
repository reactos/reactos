/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/color.c
 * PURPOSE:         Color Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

HBRUSH
APIENTRY
NtUserGetControlBrush(HWND hwnd,
                      HDC hdc,
                      UINT ctlType)
{
    UNIMPLEMENTED;
    return NULL;
}

HBRUSH
APIENTRY
NtUserGetControlColor(HWND hwndParent,
                      HWND hwnd,
                      HDC hdc,
                      UINT CtlMsg)
{
    UNIMPLEMENTED;
    return NULL;
}

HPALETTE
APIENTRY
NtUserSelectPalette(HDC hDC,
                    HPALETTE hpal,
                    BOOL ForceBackground)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserSetSysColors(INT cElements,
                   IN CONST INT *lpaElements,
                   IN CONST COLORREF *lpaRgbValues,
                   FLONG Flags)
{
    UNIMPLEMENTED;
    return FALSE;
}
