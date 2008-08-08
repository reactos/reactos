/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/cursor.c
 * PURPOSE:         Cursor Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtUserClipCursor(RECT *lpRect)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserDestroyCursor(HANDLE Handle,
                    DWORD Unknown)
{
    UNIMPLEMENTED;
    return FALSE;
}

HICON
APIENTRY
NtUserFindExistingCursorIcon(HMODULE hModule,
                             HRSRC hRsrc,
                             LONG cx,
                             LONG cy) 
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserGetClipCursor(RECT *lpRect)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserGetCursorFrameInfo(DWORD Unknown0,
                         DWORD Unknown1,
                         DWORD Unknown2,
                         DWORD Unknown3)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserGetCursorInfo(PCURSORINFO pci)
{
    UNIMPLEMENTED;
    return FALSE;
}

HCURSOR
APIENTRY
NtUserSetCursor(HCURSOR hCursor)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserSetCursorContents(HANDLE Handle,
                        PICONINFO IconInfo)
{
    UNIMPLEMENTED;
    return FALSE;
}


BOOL
APIENTRY
NtUserSetCursorIconData(HANDLE Handle,
                        PBOOL fIcon,
                        POINT *Hotspot,
                        HMODULE hModule,
                        HRSRC hRsrc,
                        HRSRC hGroupRsrc)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserSetSystemCursor(HCURSOR hcur,
                      DWORD id)
{
    UNIMPLEMENTED;
    return FALSE;
}
