/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetClipBox
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <apitest.h>

#include <wingdi.h>
#include <winuser.h>

#define ok_rect(_prc, _left, _top, _right, _bottom) \
    ok_int((_prc)->left, _left); \
    ok_int((_prc)->top, _top); \
    ok_int((_prc)->right, _right); \
    ok_int((_prc)->bottom, _bottom); \

void Test_GetClipBox()
{
    HWND hWnd;
    HDC hdc;
    RECT rect;
    HRGN hrgn, hrgn2;
    int ret;

    /* Create a window */
    hWnd = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
                        NULL, NULL, 0, 0);
    ok(hWnd != NULL, "CreateWindowW failed\n");
    if (hWnd == NULL)
    {
        return;
    }

    hdc = GetDC(hWnd);

    /* Test invalid DC */
    SetLastError(ERROR_SUCCESS);
    ret = GetClipBox((HDC)0x12345, &rect);
    ok(ret == ERROR, "Expected ERROR, got %d\n", ret);
    ok((GetLastError() == 0) || (GetLastError() == ERROR_INVALID_HANDLE), "Expected 0, got %ld\n", GetLastError());

    //ret = GetClipBox(hdc, &rect);
    //ok_int(ret, SIMPLEREGION);
    //ok_rect(&rect, 0, 0, 132, 68);

    /* Create a clip region */
    hrgn = CreateRectRgn(5, 7, 50, 50);
    SelectClipRgn(hdc, hrgn);
    DeleteObject(hrgn);
    ret = GetClipBox(hdc, &rect);
    ok_int(ret, SIMPLEREGION);
    ok_rect(&rect, 5, 7, 50, 50);

    /* Set clip region as meta region */
    SetMetaRgn(hdc);

    /* Create a new clip region */
    hrgn = CreateRectRgn(10, 10, 100, 100);
    SelectClipRgn(hdc, hrgn);
    DeleteObject(hrgn);
    ret = GetClipBox(hdc, &rect);
    ok_int(ret, SIMPLEREGION);
    ok_rect(&rect, 10, 10, 50, 50);

    /* Create an empty clip region */
    hrgn = CreateRectRgn(10, 10, 10, 30);
    SelectClipRgn(hdc, hrgn);
    DeleteObject(hrgn);
    ret = GetClipBox(hdc, &rect);
    ok_int(ret, NULLREGION);
    ok_rect(&rect, 0, 0, 0, 0);

    /* Create a complex region */
    hrgn = CreateRectRgn(10, 10, 30, 30);
    hrgn2 = CreateRectRgn(20, 20, 60, 60);
    ok_int(CombineRgn(hrgn, hrgn, hrgn2, RGN_OR), COMPLEXREGION);
    SelectClipRgn(hdc, hrgn);
    DeleteObject(hrgn2);
    ret = GetClipBox(hdc, &rect);
    ok_int(ret, COMPLEXREGION);
    ok_rect(&rect, 10, 10, 50, 50);

    /* Set scaling but keep the mapping mode (viewport should not be changed) */
    ok_int(SetViewportExtEx(hdc, 1000, 1000, NULL), 1);
    ret = GetClipBox(hdc, &rect);
    ok_int(ret, COMPLEXREGION);
    ok_rect(&rect, 10, 10, 50, 50);

    /* Set unisotropic mode, ClipBox should be unchanged */
    ok_int(SetMapMode(hdc, MM_ANISOTROPIC), 1);
    ret = GetClipBox(hdc, &rect);
    ok_int(ret, COMPLEXREGION);
    ok_rect(&rect, 10, 10, 50, 50);

    /* Now set viewport again */
    ok_int(SetViewportExtEx(hdc, 200, 400, NULL), 1);
    ret = GetClipBox(hdc, &rect);
    ok_int(ret, COMPLEXREGION); // obviously some special secret feature...
    ok_rect(&rect, 0, 0, 0, 0);

    /* Reset clip region */
    SelectClipRgn(hdc, NULL);
    SetMetaRgn(hdc);
    ret = GetClipBox(hdc, &rect);
    ok_int(ret, SIMPLEREGION);
    ok_rect(&rect, 0, 0, 0, 0);

    hrgn = CreateRectRgn(10, 10, 190, 190);
    SelectClipRgn(hdc, hrgn);
    ret = GetClipBox(hdc, &rect);
    ok_int(ret, SIMPLEREGION);
    ok_rect(&rect, 0, 0, 0, 0);

    /* Now also set the window extension */
    ok_int(SetWindowExtEx(hdc, 400, 600, NULL), 1);
    ret = GetClipBox(hdc, &rect);
    ok_int(ret, SIMPLEREGION);
    ok_rect(&rect, 20, 15, 100, 75);

    hrgn = CreateRectRgn(30, 30, 300, 300);
    SelectClipRgn(hdc, hrgn);
    SetMetaRgn(hdc);
    ret = GetClipBox(hdc, &rect);
    ok_int(ret, SIMPLEREGION);
    ok_rect(&rect, 60, 45, 100, 75);

    ReleaseDC(hWnd, hdc);
    DestroyWindow(hWnd);
}

START_TEST(GetClipBox)
{
    Test_GetClipBox();
}

