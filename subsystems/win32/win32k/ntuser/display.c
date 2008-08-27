/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/display.c
 * PURPOSE:         Display Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

LONG
APIENTRY
NtUserChangeDisplaySettings(PUNICODE_STRING lpszDeviceName,
                            LPDEVMODEW lpDevMode,
                            DWORD dwflags,
                            LPVOID lParam)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserEnumDisplayDevices (PUNICODE_STRING lpDevice,
                          DWORD iDevNum,
                          PDISPLAY_DEVICEW lpDisplayDevice,
                          DWORD dwFlags)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserEnumDisplayMonitors (HDC hdc,
                           LPCRECT lprcClip,
                           MONITORENUMPROC lpfnEnum,
                           LPARAM dwData)
{
    UNIMPLEMENTED;
    return FALSE;
}

NTSTATUS
APIENTRY
NtUserEnumDisplaySettings(PUNICODE_STRING lpszDeviceName,
                          DWORD iModeNum,
                          LPDEVMODEW lpDevMode,
                          DWORD dwFlags)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

DWORD
APIENTRY
NtUserCtxDisplayIOCtl(DWORD dwUnknown1,
                      DWORD dwUnknown2,
                      DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}
