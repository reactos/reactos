/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CreateWindowEx
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>
#include <winuser.h>


START_TEST(CreateWindowEx)
{
    HWND hWnd;
    DWORD dwError;

    SetLastError(0x1234);
    hWnd = CreateWindowExW(0, L"BUTTON", NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    dwError = GetLastError();
    ok(hWnd != NULL, "hWnd = %p\n", hWnd);
    ok(dwError == 0, "error = %lu\n", dwError);
    DestroyWindow(hWnd);

    SetLastError(0x1234);
    hWnd = CreateWindowExW(0, L"BUTTON", NULL, 0, 0, 0, 0, 0, (HWND)(LONG_PTR)-1, NULL, NULL, NULL);
    dwError = GetLastError();
    ok(hWnd == NULL, "hWnd = %p\n", hWnd);
    ok(dwError == ERROR_INVALID_WINDOW_HANDLE, "error = %lu\n", dwError);

    SetLastError(0x1234);
    hWnd = CreateWindowExW(0, L"BUTTON", NULL, WS_CHILD, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    dwError = GetLastError();
    ok(hWnd == NULL, "hWnd = %p\n", hWnd);
    ok(dwError == ERROR_TLW_WITH_WSCHILD, "error = %lu\n", dwError);

    SetLastError(0x1234);
    hWnd = CreateWindowExW(0, L"BUTTON", NULL, WS_CHILD, 0, 0, 0, 0, (HWND)(LONG_PTR)-1, NULL, NULL, NULL);
    dwError = GetLastError();
    ok(hWnd == NULL, "hWnd = %p\n", hWnd);
    ok(dwError == ERROR_INVALID_WINDOW_HANDLE, "error = %lu\n", dwError);

    SetLastError(0x1234);
    hWnd = CreateWindowExW(0, L"BUTTON", NULL, WS_POPUP, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    dwError = GetLastError();
    ok(hWnd != NULL, "hWnd = %p\n", hWnd);
    ok(dwError == 0, "error = %lu\n", dwError);
    DestroyWindow(hWnd);

    SetLastError(0x1234);
    hWnd = CreateWindowExW(0, L"BUTTON", NULL, WS_POPUP, 0, 0, 0, 0, (HWND)(LONG_PTR)-1, NULL, NULL, NULL);
    dwError = GetLastError();
    ok(hWnd == NULL, "hWnd = %p\n", hWnd);
    ok(dwError == ERROR_INVALID_WINDOW_HANDLE, "error = %lu\n", dwError);

    SetLastError(0x1234);
    hWnd = CreateWindowExW(0, L"BUTTON", NULL, WS_CHILD|WS_POPUP, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
    dwError = GetLastError();
    ok(hWnd != NULL, "hWnd = %p\n", hWnd);
    ok(dwError == 0, "error = %lu\n", dwError);
    DestroyWindow(hWnd);

    SetLastError(0x1234);
    hWnd = CreateWindowExW(0, L"BUTTON", NULL, WS_CHILD|WS_POPUP, 0, 0, 0, 0, (HWND)(LONG_PTR)-1, NULL, NULL, NULL);
    dwError = GetLastError();
    ok(hWnd == NULL, "hWnd = %p\n", hWnd);
    ok(dwError == ERROR_INVALID_WINDOW_HANDLE, "error = %lu\n", dwError);
}
