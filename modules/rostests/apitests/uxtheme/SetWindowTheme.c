/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetWindowTheme
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include <apitest.h>
#include <stdio.h>
#include <windows.h>
#include <uxtheme.h>

void TestParams(HWND hwnd)
{
    HRESULT hr;

    hr = SetWindowTheme(0, NULL, NULL);
    ok (hr == E_HANDLE, "Expected E_HANDLE got 0x%lx error\n", hr);

    hr = SetWindowTheme((HWND)(ULONG_PTR)0xdeaddeaddeaddeadULL, NULL, NULL);
    ok (hr == E_HANDLE, "Expected E_HANDLE got 0x%lx error\n", hr);

    hr = SetWindowTheme(hwnd, NULL, NULL);
    ok (hr == S_OK, "Expected S_OK got 0x%lx error\n", hr);

    hr = SetWindowTheme(hwnd, L"none", L"none");
    ok (hr == S_OK, "Expected S_OK got 0x%lx error\n", hr);

    hr = SetWindowTheme(hwnd, NULL, L"none");
    ok (hr == S_OK, "Expected S_OK got 0x%lx error\n", hr);

    hr = SetWindowTheme(hwnd, L"none", NULL);
    ok (hr == S_OK, "Expected S_OK got 0x%lx error\n", hr);

    hr = SetWindowTheme(hwnd, L"", L"");
    ok (hr == S_OK, "Expected S_OK got 0x%lx error\n", hr);
}

void TestTheme(HWND hwnd)
{
    HRESULT hr;
    HTHEME htheme1, htheme2;

    hr = SetWindowTheme(hwnd, NULL, NULL);
    ok (hr == S_OK, "Expected S_OK got 0x%lx error\n", hr);

    htheme1 = OpenThemeData(hwnd, L"Toolbar");
    if (IsThemeActive())
        ok (htheme1 != NULL, "OpenThemeData failed\n");
    else
        skip("Theme not active\n");

    hr = SetWindowTheme(hwnd, L"", L"");
    ok (hr == S_OK, "Expected S_OK got 0x%lx error\n", hr);

    htheme2 = OpenThemeData(hwnd, L"Toolbar");
    ok (htheme2 == NULL, "Expected OpenThemeData to fail\n");

    hr = SetWindowTheme(hwnd, L"TrayNotify", L"");
    ok (hr == S_OK, "Expected S_OK got 0x%lx error\n", hr);

    htheme2 = OpenThemeData(hwnd, L"Toolbar");
    ok (htheme2 == NULL, "Expected OpenThemeData to fail\n");

    hr = SetWindowTheme(hwnd, L"TrayNotify", NULL);
    ok (hr == S_OK, "Expected S_OK got 0x%lx error\n", hr);

    htheme2 = OpenThemeData(hwnd, L"Toolbar");
    if (IsThemeActive())
    {
        ok (htheme2 != NULL, "OpenThemeData failed\n");
        ok(htheme1 != htheme2, "Expected different theme data\n");
    }
    else
    {
        skip("Theme not active\n");
    }
}

START_TEST(SetWindowTheme)
{
    HWND hwnd;

    hwnd = CreateWindowW(L"button", L"Test window", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 200, 200, 0, NULL, NULL, NULL);
    ok (hwnd != NULL, "Expected CreateWindowW to succeed\n");

    TestParams(hwnd);
    TestTheme(hwnd);

    DestroyWindow(hwnd);
}
