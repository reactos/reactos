/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            dll/win32/user32/misc/imm.c
 * PURPOSE:         User32.dll Imm functions
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 * UPDATE HISTORY:
 *      01/27/2009  Created
 */

#include <user32.h>

#include <wine/debug.h>


WINE_DEFAULT_DEBUG_CHANNEL(user32);

/*
 * @unimplemented
 */
DWORD WINAPI User32InitializeImmEntryTable(PVOID p)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
IMPSetIMEW(HWND hwnd, LPIMEPROW ime)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
IMPQueryIMEW(LPIMEPROW ime)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
IMPGetIMEW(HWND hwnd, LPIMEPROW ime)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
IMPSetIMEA(HWND hwnd, LPIMEPROA ime)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
IMPQueryIMEA(LPIMEPROA ime)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
IMPGetIMEA(HWND hwnd, LPIMEPROA ime)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
LRESULT
WINAPI
SendIMEMessageExW(HWND hwnd, LPARAM lparam)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
LRESULT
WINAPI
SendIMEMessageExA(HWND hwnd, LPARAM lparam)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
WINNLSEnableIME(HWND hwnd, BOOL enable)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
WINNLSGetEnableStatus(HWND hwnd)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
UINT
WINAPI
WINNLSGetIMEHotkey(HWND hwnd)
{
    UNIMPLEMENTED;
    return FALSE;
}
