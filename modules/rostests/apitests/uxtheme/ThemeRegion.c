/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for the theme window region
 * COPYRIGHT:   Copyright 2026 Mark Jansen <mark.jansen@reactos.org>
 */

#include <apitest.h>
#include <windows.h>
#include <uxtheme.h>

static int GetRgnType(HWND hWnd)
{
    HRGN hrgn = CreateRectRgn(0, 0, 0, 0);
    int ret = GetWindowRgn(hWnd, hrgn);
    DeleteObject(hrgn);
    return ret;
}

static void SetCaption(HWND hWnd, BOOL bCaption)
{
    LONG style = GetWindowLongW(hWnd, GWL_STYLE);
    style = bCaption ? (style | WS_CAPTION) : (style & ~WS_CAPTION);
    SetWindowLongW(hWnd, GWL_STYLE, style);
    SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

static void Resize(HWND hWnd, int size)
{
    SetWindowPos(hWnd, NULL, 0, 0, size, size, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

START_TEST(ThemeRegion)
{
    HWND hWnd;

    if (!IsThemeActive())
    {
        skip("No active theme, skipping ThemeRegion tests\n");
        return;
    }

    hWnd = CreateWindowExW(0, L"static", L"", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                           100, 100, 200, 200, NULL, NULL, NULL, NULL);
    ok(hWnd != NULL, "CreateWindowExW failed\n");
    if (!hWnd)
        return;

    Resize(hWnd, 250);
    if (GetRgnType(hWnd) == ERROR)
    {
        skip("No theme region (DWM?), skipping ThemeRegion tests\n");
        DestroyWindow(hWnd);
        return;
    }

    /* Removing the caption drops the theme region */
    SetCaption(hWnd, FALSE);
    ok_int(GetRgnType(hWnd), ERROR);

    /* Restoring it brings the region back on the next resize */
    SetCaption(hWnd, TRUE);
    Resize(hWnd, 260);
    ok(GetRgnType(hWnd) != ERROR, "Expected a theme region\n");

    /* An app defined region is kept */
    SetWindowRgn(hWnd, CreateRectRgn(0, 0, 50, 50), TRUE);
    SetCaption(hWnd, FALSE);
    ok_int(GetRgnType(hWnd), SIMPLEREGION);

    DestroyWindow(hWnd);
}
