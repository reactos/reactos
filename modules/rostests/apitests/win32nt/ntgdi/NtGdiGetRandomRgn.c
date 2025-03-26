/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtGdiGetRandomRgn
 * PROGRAMMERS:
 */

#include "../win32nt.h"

START_TEST(NtGdiGetRandomRgn)
{
    HINSTANCE hinst = GetModuleHandle(NULL);
    HWND hWnd;
    HDC hDC;
    HRGN hrgn, hrgn2;

    /* Create a window */
    hWnd = CreateWindowW(L"BUTTON", L"TestWindow", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
                        NULL, NULL, hinst, 0);
//  UpdateWindow(hWnd);
    hDC = GetDC(hWnd);

    ok(hDC != NULL, "hDC was NULL.\n");

    hrgn = CreateRectRgn(0,0,0,0);
    hrgn2 = CreateRectRgn(3,3,10,10);
    SetLastError(ERROR_SUCCESS);
    ok_int(NtGdiGetRandomRgn(0, hrgn, 0), -1);
    ok_long(GetLastError(), ERROR_INVALID_HANDLE);

    SetLastError(ERROR_SUCCESS);
    ok_int(NtGdiGetRandomRgn((HDC)2345, hrgn, 1), -1);
    ok_long(GetLastError(), ERROR_INVALID_HANDLE);

    SetLastError(ERROR_SUCCESS);
    ok_int(NtGdiGetRandomRgn((HDC)2345, hrgn, 10), -1);
    ok_long(GetLastError(), ERROR_INVALID_HANDLE);

    SetLastError(ERROR_SUCCESS);
    ok_int(NtGdiGetRandomRgn((HDC)2345, (HRGN)10, 10), -1);
    ok_long(GetLastError(), ERROR_INVALID_HANDLE);

    SetLastError(ERROR_SUCCESS);
    ok_int(NtGdiGetRandomRgn((HDC)2345, 0, 1), -1);
    ok_long(GetLastError(), ERROR_INVALID_HANDLE);

    SetLastError(ERROR_SUCCESS);
    ok_int(NtGdiGetRandomRgn(hDC, 0, 0), 0);
    ok_int(NtGdiGetRandomRgn(hDC, 0, 1), 0);
    ok_int(NtGdiGetRandomRgn(hDC, (HRGN)-5, 0), 0);
    ok_int(NtGdiGetRandomRgn(hDC, (HRGN)-5, 1), 0);
    ok_int(NtGdiGetRandomRgn(hDC, hrgn, 0), 0);
    ok_int(NtGdiGetRandomRgn(hDC, hrgn, 1), 0);
    ok_int(NtGdiGetRandomRgn(hDC, hrgn, 2), 0);
    ok_int(NtGdiGetRandomRgn(hDC, hrgn, 3), 0);
    ok_int(NtGdiGetRandomRgn(hDC, hrgn, 4), 1);
    ok_int(NtGdiGetRandomRgn(hDC, hrgn, 5), 0);
    ok_int(NtGdiGetRandomRgn(hDC, hrgn, 10), 0);
    ok_int(NtGdiGetRandomRgn(hDC, hrgn, -10), 0);
    ok_long(GetLastError(), ERROR_SUCCESS);

    SelectClipRgn(hDC, hrgn2);
    ok_int(NtGdiGetRandomRgn(hDC, 0, 1), -1);
    ok_long(GetLastError(), ERROR_SUCCESS);
    ok_int(NtGdiGetRandomRgn(hDC, hrgn, 1), 1);
    ok_int(CombineRgn(hrgn, hrgn, hrgn, RGN_OR), SIMPLEREGION);
    ok_int(CombineRgn(hrgn, hrgn, hrgn2, RGN_XOR), NULLREGION);

    SetRectRgn(hrgn2,0,0,0,0);
    SelectClipRgn(hDC, hrgn2);
    ok_int(NtGdiGetRandomRgn(hDC, hrgn, 1), 1);

    ok_int(CombineRgn(hrgn2, hrgn, hrgn2, RGN_XOR), NULLREGION);
    ok_int(CombineRgn(hrgn2, hrgn, hrgn, RGN_OR), NULLREGION);

    SelectClipRgn(hDC, NULL);
    ok_int(NtGdiGetRandomRgn(hDC, hrgn, 1), 0);


    ok_int(NtGdiGetRandomRgn(hDC, hrgn, 4), 1);

    ok_long(GetLastError(), ERROR_SUCCESS);

    ReleaseDC(hWnd, hDC);
    DestroyWindow(hWnd);

}
