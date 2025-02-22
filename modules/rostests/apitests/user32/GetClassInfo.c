/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for GetClassInfo
 * COPYRIGHT:   Copyright 2023 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

static USHORT GetWinVersion(VOID)
{
    return ((GetVersion() & 0xFF) << 8) |
        ((GetVersion() >> 8) & 0xFF);
}

VOID Test_Desktop(VOID)
{
    WNDCLASSEXW wcex;
    BOOL result;

    memset(&wcex, 0xab, sizeof(wcex));

    result = GetClassInfoExW(GetModuleHandle(NULL), (LPCWSTR)WC_DESKTOP, &wcex);
    ok_int(result, (ULONG_PTR)WC_DESKTOP);

    ok_hex(wcex.cbSize, 0xabababab);
    ok_hex(wcex.style, 0x8);
    ok(wcex.lpfnWndProc != NULL, "lpfnWndProc shound't be NULL\n");
    ok_int(wcex.cbClsExtra, 0);
    ok_int(wcex.cbWndExtra, GetWinVersion() <= 0x502 ? 8 : 0);
    ok_ptr(wcex.hInstance, GetModuleHandle(NULL));
    ok_ptr(wcex.hIcon, NULL);
    ok(wcex.hCursor != NULL, "hCursor shound't be NULL\n");
    if (GetWinVersion() > 0x502)
        ok_ptr(wcex.hbrBackground, (HBRUSH)(ULONG_PTR)2);
    else
        ok(wcex.hbrBackground != NULL, "hbrBackground shound't be NULL\n");
    ok_ptr(wcex.lpszMenuName, NULL);
    ok_ptr(wcex.lpszClassName, (LPCWSTR)WC_DESKTOP);
    ok_ptr(wcex.hIconSm, NULL);
}

VOID Test_Dialog(VOID)
{
    WNDCLASSEXW wcex;
    BOOL result;

    memset(&wcex, 0xab, sizeof(wcex));

    result = GetClassInfoExW(GetModuleHandle(NULL), (LPCWSTR)(ULONG_PTR)WC_DIALOG, &wcex);
    ok_int(result, (ULONG_PTR)WC_DIALOG);
    
    ok_hex(wcex.cbSize, 0xabababab);
    ok_hex(wcex.style, 0x808);
    ok(wcex.lpfnWndProc != NULL, "lpfnWndProc shound't be NULL\n");
    ok_int(wcex.cbClsExtra, 0);
    ok_int(wcex.cbWndExtra, 30); // DLGWINDOWEXTRA
    ok_ptr(wcex.hInstance, GetModuleHandle(NULL));
    ok_ptr(wcex.hIcon, NULL);
    ok(wcex.hCursor != NULL, "hCursor shound't be NULL\n");
    ok_ptr(wcex.hbrBackground, NULL);
    ok_ptr(wcex.lpszMenuName, NULL);
    ok_ptr(wcex.lpszClassName, (LPCWSTR)(ULONG_PTR)WC_DIALOG);
    ok_ptr(wcex.hIconSm, NULL);
}

VOID Test_Menu(VOID)
{
    WNDCLASSEXW wcex;
    BOOL result;

    memset(&wcex, 0xab, sizeof(wcex));

    result = GetClassInfoExW(GetModuleHandle(NULL), (LPCWSTR)(ULONG_PTR)WC_MENU, &wcex);
    ok_int(result, (ULONG_PTR)WC_MENU);

    ok_hex(wcex.cbSize, 0xabababab);
    ok_hex(wcex.style, 0x803);
    ok_ptr(wcex.lpfnWndProc, NULL);
    ok_int(wcex.cbClsExtra, 0);
    ok_int(wcex.cbWndExtra, 16);
    ok_ptr(wcex.hInstance, GetModuleHandle(NULL));
    ok_ptr(wcex.hIcon, NULL);
    ok(wcex.hCursor != NULL, "hCursor shound't be NULL\n");
    ok_ptr(wcex.hbrBackground, NULL);
    ok_ptr(wcex.lpszMenuName, NULL);
    ok_ptr(wcex.lpszClassName, (LPCWSTR)(ULONG_PTR)WC_MENU);
    ok_ptr(wcex.hIconSm, NULL);
}


VOID Test_SwitchWnd(VOID)
{
    WNDCLASSEXW wcex;
    BOOL result;

    memset(&wcex, 0xab, sizeof(wcex));

    result = GetClassInfoExW(GetModuleHandle(NULL), (LPCWSTR)(ULONG_PTR)WC_SWITCH, &wcex);
    ok_int(result, (ULONG_PTR)WC_SWITCH);

    ok_hex(wcex.cbSize, 0xabababab);
    ok_hex(wcex.style, 0x803);
    ok_ptr(wcex.lpfnWndProc, NULL);
    ok_int(wcex.cbClsExtra, 0);
    ok_int(wcex.cbWndExtra, GetWinVersion() <= 0x502 ? sizeof(ULONG_PTR) : 16);
    ok_ptr(wcex.hInstance, GetModuleHandle(NULL));
    ok_ptr(wcex.hIcon, NULL);
    ok(wcex.hCursor != NULL, "hCursor shound't be NULL\n");
    ok_ptr(wcex.hbrBackground, NULL);
    ok_ptr(wcex.lpszMenuName, NULL);
    ok_ptr(wcex.lpszClassName, (LPCWSTR)(ULONG_PTR)WC_SWITCH);
    ok_ptr(wcex.hIconSm, NULL);
}

VOID Test_Custom(VOID)
{
    WNDCLASSEXW wcex;
    ATOM atom;
    BOOL result;

    memset(&wcex, 0, sizeof(wcex));

    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = 0x1;
    wcex.lpfnWndProc = DefWindowProcW;
    wcex.cbClsExtra = 1;
    wcex.cbWndExtra = 5;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hIcon = NULL;
    wcex.hCursor = NULL;
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"ProTestClass1";
    wcex.hIconSm = 0;

    atom = RegisterClassExW(&wcex);
    ok(atom != 0, "Failed to register class!\n");
    if (atom == 0)
    {
        skip("Failed to register class!");
        return;
    }

    memset(&wcex, 0xab, sizeof(wcex));

    result = GetClassInfoExW(GetModuleHandle(NULL), (LPCWSTR)(ULONG_PTR)atom, &wcex);
    ok_int(result, atom);

    ok_hex(wcex.cbSize, 0xabababab);
    ok_hex(wcex.style, 0x1);
    ok_ptr(wcex.lpfnWndProc, DefWindowProcW);
    ok_int(wcex.cbClsExtra, 1);
    ok_int(wcex.cbWndExtra, 5);
    ok_ptr(wcex.hInstance, GetModuleHandle(NULL));
    ok_ptr(wcex.hIcon, NULL);
    ok_ptr(wcex.hIcon, NULL);
    ok_ptr(wcex.hbrBackground, NULL);
    ok_ptr(wcex.lpszMenuName, NULL);
    ok_ptr(wcex.lpszClassName, (LPCWSTR)(ULONG_PTR)atom);
    ok_ptr(wcex.hIconSm, NULL);
}

START_TEST(GetClassInfo)
{
    Test_Desktop();
    Test_Dialog();
    Test_SwitchWnd();
    Test_Custom();
}
