/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for ScrollWindowEx
 * PROGRAMMERS:     Timo Kreuzer
 *                  Katayama Hirofumi MZ
 */

#include "precomp.h"

void
Test_ScrollWindowEx()
{
    HWND hWnd, hChild1, hChild2;
    HRGN hrgn;
    int Result;
    RECT rc, rcChild1, rcChild2;
    INT x1, y1, x2, y2, dx, dy;
    DWORD style;

    /* Create a window */
    style = WS_POPUP | SS_WHITERECT | WS_VISIBLE;
    hWnd = CreateWindowW(L"STATIC", L"TestWindow", style, CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, NULL, NULL, 0, 0);
    ok(hWnd != NULL, "hWnd was NULL.\n");
    UpdateWindow(hWnd);

    /* Assert that no update region is there */
    hrgn = CreateRectRgn(0, 0, 0, 0);
    Result = GetUpdateRgn(hWnd, hrgn, FALSE);
    ok(Result == NULLREGION, "Result = %d\n", Result);

    Result = ScrollWindowEx(hWnd, 20, 0, NULL, NULL, NULL, NULL, 0);
    ok(Result == SIMPLEREGION, "Result = %d\n", Result);
    Result = GetUpdateRgn(hWnd, hrgn, FALSE);
    ok(Result == NULLREGION, "Result = %d\n", Result);

    Result = ScrollWindowEx(hWnd, 20, 0, NULL, NULL, NULL, NULL, SW_INVALIDATE);
    ok(Result == SIMPLEREGION, "Result = %d\n", Result);
    Result = GetUpdateRgn(hWnd, hrgn, FALSE);
    ok(Result == SIMPLEREGION, "Result = %d\n", Result);
    UpdateWindow(hWnd);

    // test invalid update region
    DeleteObject(hrgn);
    Result = ScrollWindowEx(hWnd, 20, 0, NULL, NULL, hrgn, NULL, SW_INVALIDATE);
    ok(Result == ERROR, "Result = %d\n", Result);
    hrgn = CreateRectRgn(0, 0, 0, 0);
    UpdateWindow(hWnd);

    // Test invalid updaterect pointer
    Result = ScrollWindowEx(hWnd, 20, 0, NULL, NULL, NULL, (LPRECT)1, SW_INVALIDATE);
    ok(Result == ERROR, "Result = %d\n", Result);
    Result = GetUpdateRgn(hWnd, hrgn, FALSE);
    ok(Result == SIMPLEREGION, "Result = %d\n", Result);

    /* create child window 1 */
    x1 = 1;
    y1 = 3;
    style = WS_CHILD | WS_VISIBLE | SS_BLACKRECT;
    hChild1 = CreateWindowW(L"STATIC", L"Child1", style, x1, y1, 10, 10, hWnd, NULL, 0, 0);
    ok(hChild1 != NULL, "hChild1 was NULL.\n");
    UpdateWindow(hChild1);

    /* create child window 2 */
    x2 = 5;
    y2 = 7;
    style = WS_CHILD | WS_VISIBLE | SS_WHITERECT;
    hChild2 = CreateWindowW(L"STATIC", L"Child2", style, x2, y2, 10, 10, hWnd, NULL, 0, 0);
    ok(hChild2 != NULL, "hChild2 was NULL.\n");
    UpdateWindow(hChild2);

    /* scroll with child windows */
    dx = 3;
    dy = 8;
    ScrollWindowEx(hWnd, dx, dy, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN);
    UpdateWindow(hWnd);

    /* check the positions */
    GetWindowRect(hWnd, &rc);
    GetWindowRect(hChild1, &rcChild1);
    GetWindowRect(hChild2, &rcChild2);
    ok_long(rcChild1.left - rc.left, x1 + dx);
    ok_long(rcChild2.left - rc.left, x2 + dx);
    ok_long(rcChild1.top - rc.top, y1 + dy);
    ok_long(rcChild2.top - rc.top, y2 + dy);

    /* update */
    x1 += dx;
    y1 += dy;
    x2 += dx;
    y2 += dy;

    /* scroll with child windows */
    dx = 9;
    dy = -2;
    ScrollWindowEx(hWnd, dx, dy, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN);
    UpdateWindow(hWnd);

    /* check the positions */
    GetWindowRect(hWnd, &rc);
    GetWindowRect(hChild1, &rcChild1);
    GetWindowRect(hChild2, &rcChild2);
    ok_long(rcChild1.left - rc.left, x1 + dx);
    ok_long(rcChild2.left - rc.left, x2 + dx);
    ok_long(rcChild1.top - rc.top, y1 + dy);
    ok_long(rcChild2.top - rc.top, y2 + dy);

    DestroyWindow(hChild1);
    DestroyWindow(hChild2);
    DeleteObject(hrgn);
    DestroyWindow(hWnd);
}

START_TEST(ScrollWindowEx)
{
    Test_ScrollWindowEx();
}
