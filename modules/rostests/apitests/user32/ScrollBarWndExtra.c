/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test for ScrollBar cbWndExtra
 * COPYRIGHT:   Copyright 2019 Mark Jansen <mark.jansen@reactos.org>
 *
 * Why do we need this test?
 * Ask the authors of Civilization II...
 */

#include "precomp.h"

#define BUILTIN_SCROLLBAR   "Scrollbar"
#define CUSTOM_SCROLLBAR    "MSScrollBarClass"



START_TEST(ScrollBarWndExtra)
{
    HWND hScrollBar;
    HWND hScrollBarImpersonator;
    WNDCLASSA WndClass;
    ATOM ClassAtom;

    LONG_PTR dummyData = (LONG_PTR)0xbeefbeefbeefbeefULL, result;
    WNDPROC lpfnWndProc;
    DWORD dwExtra;

    hScrollBar = CreateWindowExA(0, BUILTIN_SCROLLBAR, "", WS_POPUP,
                                 20, 20, 120, 120, NULL, 0, GetModuleHandle(NULL), 0);

    ok(hScrollBar != NULL, "Scrollbar creation failed (%lu)\n", GetLastError());

    lpfnWndProc = (WNDPROC)GetWindowLongPtrA(hScrollBar, GWL_WNDPROC);
    dwExtra = GetClassLongPtrA(hScrollBar, GCL_CBWNDEXTRA);

    ZeroMemory(&WndClass, sizeof(WndClass));
    WndClass.style = CS_DBLCLKS | CS_PARENTDC | CS_HREDRAW | CS_VREDRAW;
    WndClass.lpfnWndProc = lpfnWndProc;
    WndClass.cbWndExtra = dwExtra + sizeof(LONG_PTR);
    WndClass.hInstance = GetModuleHandle(NULL);
    WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WndClass.hbrBackground = GetStockObject(LTGRAY_BRUSH);
    WndClass.lpszClassName = CUSTOM_SCROLLBAR;
    ClassAtom = RegisterClassA(&WndClass);

    ok(ClassAtom != 0, "RegisterClassA failed (%lu)\n", GetLastError());
    DestroyWindow(hScrollBar);


    hScrollBarImpersonator = CreateWindowExA(0, CUSTOM_SCROLLBAR, "", WS_POPUP,
                                             20, 20, 120, 120, NULL, 0, GetModuleHandle(NULL), 0);
    ok(hScrollBarImpersonator != NULL, "Scrollbar creation failed (%lu)\n", GetLastError());

    SetWindowLongPtrA(hScrollBarImpersonator, dwExtra, dummyData);
    result = GetWindowLongPtrA(hScrollBarImpersonator, dwExtra);
    ok(result == dummyData, "Invalid dummyData\n");

    DestroyWindow(hScrollBarImpersonator);
    UnregisterClassA(CUSTOM_SCROLLBAR, GetModuleHandle(NULL));
}
