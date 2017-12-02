/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetMessage/PeekMessage
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

void Test_GetMessage(HWND hWnd)
{
    MSG msg;

    SetLastError(DNS_ERROR_RCODE_NXRRSET);

    ok(GetMessage(&msg, hWnd, 0, 0) == -1, "\n");
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "GetLastError() = %lu\n", GetLastError());
}

void Test_PeekMessage(HWND hWnd)
{
    MSG msg;

    SetLastError(DNS_ERROR_RCODE_NXRRSET);

    ok(PeekMessage(&msg, hWnd, 0, 0, PM_NOREMOVE) == 0, "\n");
    ok(GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "GetLastError() = %lu\n", GetLastError());
}

START_TEST(GetPeekMessage)
{
    HWND hWnd = CreateWindowExW(0, L"EDIT", L"miau", 0, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
    ok(hWnd != INVALID_HANDLE_VALUE, "\n");
    /* make sure we pass an invalid handle to GetMessage/PeekMessage */
    ok(DestroyWindow(hWnd), "\n");

    Test_GetMessage(hWnd);
    Test_PeekMessage(hWnd);
}
