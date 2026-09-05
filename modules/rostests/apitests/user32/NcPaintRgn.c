/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for the WM_NCPAINT update region on frame geometry changes
 * COPYRIGHT:   Copyright 2026 Ahmed Arif <arif.ing@outlook.com>
 */

#include "precomp.h"

/*
 * When the frame geometry of a window changes, SetWindowPos copies the bits it
 * can instead of repainting them. The copied bits carry the old caption along,
 * so the whole non-client frame has to end up in the WM_NCPAINT update region,
 * or the caption buttons are left painted at their former position (CORE-20769).
 */

/* WM_NCPAINT passes this instead of a region when the whole window is to be painted. */
#define NCPAINT_WHOLE_WINDOW ((HRGN)1)

/* Accumulated non-client update region of the current operation, screen coordinates. */
static HRGN g_hrgnNcUpdate;
static BOOL g_bWholeWindow;
static INT g_cNcPaint;

static
VOID
ResetNcPaint(VOID)
{
    SetRectRgn(g_hrgnNcUpdate, 0, 0, 0, 0);
    g_bWholeWindow = FALSE;
    g_cNcPaint = 0;
}

static
LRESULT
CALLBACK
NcPaintWndProc(
    _In_ HWND hWnd,
    _In_ UINT message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    if (message == WM_NCPAINT)
    {
        HRGN hrgn = (HRGN)wParam;

        g_cNcPaint++;

        if (hrgn == NCPAINT_WHOLE_WINDOW)
            g_bWholeWindow = TRUE;
        else if (hrgn != NULL)
            CombineRgn(g_hrgnNcUpdate, g_hrgnNcUpdate, hrgn, RGN_OR);
    }

    return DefWindowProcW(hWnd, message, wParam, lParam);
}

static
VOID
PumpMessages(VOID)
{
    MSG msg;

    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

/* Verify that nothing of the non-client frame was left out of the update region. */
static
VOID
ExpectWholeFrameRepainted(
    _In_ HWND hWnd,
    _In_ PCSTR pszStage)
{
    RECT rcWindow, rcClient;
    HRGN hrgnFrame, hrgnClient, hrgnMissing;
    INT type;

    GetWindowRect(hWnd, &rcWindow);
    GetClientRect(hWnd, &rcClient);
    MapWindowPoints(hWnd, NULL, (PPOINT)&rcClient, 2);

    hrgnFrame = CreateRectRgnIndirect(&rcWindow);
    hrgnClient = CreateRectRgnIndirect(&rcClient);
    hrgnMissing = CreateRectRgn(0, 0, 0, 0);

    /* The non-client frame is what the window covers but the client area does not. */
    CombineRgn(hrgnFrame, hrgnFrame, hrgnClient, RGN_DIFF);
    type = CombineRgn(hrgnMissing, hrgnFrame, g_hrgnNcUpdate, RGN_DIFF);

    ok(g_cNcPaint > 0, "%s: no WM_NCPAINT was sent\n", pszStage);

    if (!g_bWholeWindow)
    {
        RECT rcMissing;

        GetRgnBox(hrgnMissing, &rcMissing);
        ok(type == NULLREGION,
           "%s: (%ld,%ld)-(%ld,%ld) of the non-client frame is not in the update region\n",
           pszStage, rcMissing.left, rcMissing.top, rcMissing.right, rcMissing.bottom);
    }

    DeleteObject(hrgnMissing);
    DeleteObject(hrgnClient);
    DeleteObject(hrgnFrame);
}

START_TEST(NcPaintRgn)
{
    WNDCLASSEXW wc;
    HWND hWnd;
    ATOM atom;

    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = NcPaintWndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hCursor = LoadCursorW(NULL, (PCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"NcPaintRgnTestClass";

    atom = RegisterClassExW(&wc);
    if (atom == 0)
    {
        skip("RegisterClassExW failed with %lu\n", GetLastError());
        return;
    }

    g_hrgnNcUpdate = CreateRectRgn(0, 0, 0, 0);
    if (g_hrgnNcUpdate == NULL)
    {
        skip("CreateRectRgn failed with %lu\n", GetLastError());
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return;
    }

    hWnd = CreateWindowExW(0,
                           wc.lpszClassName,
                           L"NcPaintRgn",
                           WS_OVERLAPPEDWINDOW,
                           100, 100, 400, 300,
                           NULL, NULL, wc.hInstance, NULL);
    if (hWnd == NULL)
    {
        skip("CreateWindowExW failed with %lu\n", GetLastError());
        DeleteObject(g_hrgnNcUpdate);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return;
    }

    /* Get the window on screen and fully painted, so that the frame is settled. */
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    PumpMessages();

    ResetNcPaint();
    ShowWindow(hWnd, SW_MAXIMIZE);
    UpdateWindow(hWnd);
    PumpMessages();
    ExpectWholeFrameRepainted(hWnd, "maximize");

    ResetNcPaint();
    ShowWindow(hWnd, SW_RESTORE);
    UpdateWindow(hWnd);
    PumpMessages();
    ExpectWholeFrameRepainted(hWnd, "restore");

    DestroyWindow(hWnd);
    PumpMessages();

    DeleteObject(g_hrgnNcUpdate);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
}
