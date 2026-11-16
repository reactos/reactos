/*
 * PROJECT:     VerOS NT Subsystem
 * LICENSE:     GNU Lesser General Public License, version 2.1 or later (LGPL-2.1-or-later)
 * FILE:        win32ss/user/user32_vista/dpi.c
 * PURPOSE:     DPI functions for user32 and user32_vista.
 * COPYRIGHT:   Copyright 2024 Carl Bialorucki <cbialo2@outlook.com>
 *              Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <windef.h>
#include <wingdi.h>
#include <winuser.h>

HDC APIENTRY
NtUserGetDC(HWND hWnd);

#define NDEBUG
#include <debug.h>

/*
 * @stub
 */
UINT
WINAPI
GetDpiForSystem(VOID)
{
    HDC hDC;
    UINT Dpi;
    hDC = NtUserGetDC(NULL);
    Dpi = GetDeviceCaps(hDC, LOGPIXELSY);
    ReleaseDC(NULL, hDC);
    return Dpi;
}

/*
 * @stub
 */
UINT
WINAPI
GetDpiForWindow(
    _In_ HWND hWnd)
{
    UNIMPLEMENTED_ONCE;
    UNREFERENCED_PARAMETER(hWnd);
    return GetDpiForSystem();
}

/*
 * @implemented
 */
int
WINAPI
GetSystemMetricsForDpi(int index,
                       UINT dpi)
{
    int value;
    UINT systemDpi;

    if (!dpi)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    value = GetSystemMetrics(index);
    /* Counts and flags stay as they are. Sizes follow the requested DPI. */
    if (value > 1 || value < -1)
    {
        systemDpi = GetDpiForSystem();
        if (systemDpi && dpi != systemDpi)
            value = MulDiv(value, dpi, systemDpi);
    }
    return value;
}

/*
 * @implemented
 */
BOOL
WINAPI
AdjustWindowRectExForDpi(LPRECT rect,
                         DWORD style,
                         BOOL menu,
                         DWORD exStyle,
                         UINT dpi)
{
    RECT original;
    UINT systemDpi;

    if (!rect || !dpi)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    original = *rect;
    if (!AdjustWindowRectEx(rect, style, menu, exStyle))
        return FALSE;

    systemDpi = GetDpiForSystem();
    if (!systemDpi || dpi == systemDpi)
        return TRUE;

    rect->left = original.left - MulDiv(original.left - rect->left, dpi, systemDpi);
    rect->top = original.top - MulDiv(original.top - rect->top, dpi, systemDpi);
    rect->right = original.right + MulDiv(rect->right - original.right, dpi, systemDpi);
    rect->bottom = original.bottom + MulDiv(rect->bottom - original.bottom, dpi, systemDpi);
    return TRUE;
}

static DPI_AWARENESS_CONTEXT ThreadDpiContext = DPI_AWARENESS_CONTEXT_UNAWARE;

static BOOL
IsKnownDpiContext(DPI_AWARENESS_CONTEXT context)
{
    LONG_PTR value = (LONG_PTR)context;

    return value <= -1 && value >= -5;
}

/*
 * @implemented
 */
DPI_AWARENESS_CONTEXT
WINAPI
SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT context)
{
    DPI_AWARENESS_CONTEXT previous;

    if (!IsKnownDpiContext(context))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    previous = ThreadDpiContext;
    ThreadDpiContext = context;
    return previous;
}

/*
 * @implemented
 *
 * https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getthreaddpiawarenesscontext
 * Firefox delay-load: user32.dll!GetThreadDpiAwarenessContext.
 */
DPI_AWARENESS_CONTEXT
WINAPI
GetThreadDpiAwarenessContext(VOID)
{
    return ThreadDpiContext;
}

/*
 * @implemented
 */
DPI_AWARENESS_CONTEXT
WINAPI
GetWindowDpiAwarenessContext(HWND hwnd)
{
    if (hwnd && !IsWindow(hwnd))
    {
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
        return NULL;
    }

    return ThreadDpiContext;
}

/*
 * @implemented
 */
BOOL
WINAPI
AreDpiAwarenessContextsEqual(DPI_AWARENESS_CONTEXT first,
                             DPI_AWARENESS_CONTEXT second)
{
    if (!first || !second)
        return FALSE;

    return first == second;
}

/*
 * @stub
 */
BOOL
WINAPI
IsProcessDPIAware(VOID)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @stub
 */
BOOL
WINAPI
SetProcessDPIAware(VOID)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @implemented
 *
 * https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setprocessdpiawarenesscontext
 * Firefox delay-load: user32.dll!SetProcessDpiAwarenessContext.
 * Sets the process-default context used by GetThreadDpiAwarenessContext
 * until a thread overrides it with SetThreadDpiAwarenessContext.
 */
BOOL
WINAPI
SetProcessDpiAwarenessContext(
    _In_ DPI_AWARENESS_CONTEXT context)
{
    if (!IsKnownDpiContext(context))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ThreadDpiContext = context;
    return TRUE;
}

/*
 * @stub
 */
BOOL
WINAPI
GetProcessDpiAwarenessInternal(
    _In_  HANDLE process,
    _Out_ DPI_AWARENESS *awareness)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @stub
 */
BOOL
WINAPI
SetProcessDpiAwarenessInternal(
    _In_ DPI_AWARENESS awareness)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @stub
 */
BOOL
WINAPI
GetDpiForMonitorInternal(
    _In_  HMONITOR monitor,
    _In_  UINT type,
    _Out_ UINT *x,
    _Out_ UINT *y)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @stub
 */
BOOL
WINAPI
LogicalToPhysicalPoint(
    _In_ HWND hwnd, 
    _Inout_ POINT *point )
{
    UNIMPLEMENTED;
    return TRUE;
}

