/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/clipboard.c
 * PURPOSE:         Clipboard Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtUserChangeClipboardChain(HWND hWndRemove,
                           HWND hWndNewNext)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserOpenClipboard(HWND hWnd,
                    HWND* phWnd)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserCloseClipboard(VOID)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtUserCountClipboardFormats(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserEmptyClipboard(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

HANDLE
APIENTRY
NtUserGetClipboardData(UINT uFormat,
                       PVOID pBuffer)
{
    UNIMPLEMENTED;
    return NULL;
}

HANDLE
APIENTRY
NtUserSetClipboardData(UINT uFormat,
                       HANDLE hMem,
                       DWORD Unknown2)
{
    UNIMPLEMENTED;
    return NULL;
}

INT
APIENTRY
NtUserGetClipboardFormatName(UINT format,
                             PUNICODE_STRING FormatName,
                             INT cchMaxCount)
{
    UNIMPLEMENTED;
    return 0;
}

HWND
APIENTRY
NtUserGetClipboardOwner(VOID)
{
    UNIMPLEMENTED;
    return NULL;
}

DWORD
APIENTRY
NtUserGetClipboardSequenceNumber(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

HWND
APIENTRY
NtUserGetClipboardViewer(VOID)
{
    UNIMPLEMENTED;
    return NULL;
}

HWND
APIENTRY
NtUserSetClipboardViewer(HWND hWndNewViewer)
{
    UNIMPLEMENTED;
    return NULL;
}

HWND
APIENTRY
NtUserGetOpenClipboardWindow(VOID)
{
    UNIMPLEMENTED;
    return NULL;
}

INT
APIENTRY
NtUserGetPriorityClipboardFormat(PUINT paFormatPriorityList,
                                 INT cFormats)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserIsClipboardFormatAvailable(UINT format)
{
    UNIMPLEMENTED;
    return FALSE;
}
