/* Unit tests for systray
 *
 * Copyright 2007 Mikolaj Zalewski
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#define _WIN32_IE 0x600
#include <assert.h>
#include <stdarg.h>

#include <windows.h>

#include "wine/test.h"


static HWND hMainWnd;
static BOOL (WINAPI *pShell_NotifyIconW)(DWORD,PNOTIFYICONDATAW);
static HMONITOR (WINAPI *pMonitorFromWindow)(HWND, DWORD);

void test_cbsize(void)
{
    NOTIFYICONDATAA nidA;

    if (pShell_NotifyIconW)
    {
        NOTIFYICONDATAW nidW;

        ZeroMemory(&nidW, sizeof(nidW));
        nidW.cbSize = NOTIFYICONDATAW_V1_SIZE;
        nidW.hWnd = hMainWnd;
        nidW.uID = 1;
        nidW.uFlags = NIF_ICON|NIF_MESSAGE;
        nidW.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        nidW.uCallbackMessage = WM_USER+17;
        ok(pShell_NotifyIconW(NIM_ADD, &nidW), "NIM_ADD failed!\n");

        /* using an invalid cbSize does work */
        nidW.cbSize = 3;
        nidW.hWnd = hMainWnd;
        nidW.uID = 1;
        ok(pShell_NotifyIconW(NIM_DELETE, &nidW), "NIM_DELETE failed!\n");
        /* as icon doesn't exist anymore - now there will be an error */
        nidW.cbSize = sizeof(nidW);
        ok(!pShell_NotifyIconW(NIM_DELETE, &nidW), "The icon was not deleted\n");
    }

    /* same for Shell_NotifyIconA */
    ZeroMemory(&nidA, sizeof(nidA));
    nidA.cbSize = NOTIFYICONDATAA_V1_SIZE;
    nidA.hWnd = hMainWnd;
    nidA.uID = 1;
    nidA.uFlags = NIF_ICON|NIF_MESSAGE;
    nidA.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    nidA.uCallbackMessage = WM_USER+17;
    ok(Shell_NotifyIconA(NIM_ADD, &nidA), "NIM_ADD failed!\n");

    /* using an invalid cbSize does work */
    nidA.cbSize = 3;
    nidA.hWnd = hMainWnd;
    nidA.uID = 1;
    ok(Shell_NotifyIconA(NIM_DELETE, &nidA), "NIM_DELETE failed!\n");
    /* as icon doesn't exist anymore - now there will be an error */
    nidA.cbSize = sizeof(nidA);
    ok(!Shell_NotifyIconA(NIM_DELETE, &nidA), "The icon was not deleted\n");
}

static void test_SHAppBarMessage(void)
{
    APPBARDATA abd;
    HWND hwnd, foregnd;
    UINT_PTR ret;

    memset(&abd, 0xcc, sizeof(abd));
    abd.cbSize = sizeof(abd);
    abd.uEdge = ABE_BOTTOM;

    hwnd = (HWND)SHAppBarMessage(ABM_GETAUTOHIDEBAR, &abd);
    ok(hwnd == NULL || IsWindow(hwnd), "ret %p which is not a window\n", hwnd);
    ok(abd.hWnd == (HWND)0xcccccccc, "hWnd overwritten\n");

    if (!pMonitorFromWindow)
    {
        skip("MonitorFromWindow is not available\n");
    }
    else
    {
        /* Presumably one can pass a hwnd with ABM_GETAUTOHIDEBAR to specify a monitor.
           Pass the foreground window and check */
        foregnd = GetForegroundWindow();
        if(foregnd)
        {
            abd.hWnd = foregnd;
            hwnd = (HWND)SHAppBarMessage(ABM_GETAUTOHIDEBAR, &abd);
            ok(hwnd == NULL || IsWindow(hwnd), "ret %p which is not a window\n", hwnd);
            ok(abd.hWnd == foregnd, "hWnd overwritten\n");
            if(hwnd)
            {
                HMONITOR appbar_mon, foregnd_mon;
                appbar_mon = pMonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                foregnd_mon = pMonitorFromWindow(foregnd, MONITOR_DEFAULTTONEAREST);
                ok(appbar_mon == foregnd_mon, "Windows on different monitors\n");
            }
        }
    }

    memset(&abd, 0xcc, sizeof(abd));
    abd.cbSize = sizeof(abd);
    ret = SHAppBarMessage(ABM_GETTASKBARPOS, &abd);
    if(ret)
    {
        ok(abd.hWnd == (HWND)0xcccccccc, "hWnd overwritten\n");
todo_wine
{
        ok(abd.uEdge >= ABE_LEFT && abd.uEdge <= ABE_BOTTOM, "uEdge not returned\n");
        ok(abd.rc.left != 0xcccccccc, "rc not updated\n");
}
    }

    return;
}

START_TEST(systray)
{
    WNDCLASSA wc;
    MSG msg;
    RECT rc;
    HMODULE huser32, hshell32;

    skip("ROS-HACK: Skipping systray tests\n");
    return;

    hshell32 = GetModuleHandleA("shell32.dll");
    pShell_NotifyIconW = (void*)GetProcAddress(hshell32, "Shell_NotifyIconW");

    huser32 = GetModuleHandleA("user32.dll");
    pMonitorFromWindow = (void*)GetProcAddress(huser32, "MonitorFromWindow");

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = NULL;
    wc.hCursor = LoadCursorA(NULL, IDC_IBEAM);
    wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "MyTestWnd";
    wc.lpfnWndProc = DefWindowProc;
    RegisterClassA(&wc);

    hMainWnd = CreateWindowExA(0, "MyTestWnd", "Blah", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 680, 260, NULL, NULL, GetModuleHandleA(NULL), 0);
    GetClientRect(hMainWnd, &rc);
    ShowWindow(hMainWnd, SW_SHOW);

    test_cbsize();

    PostQuitMessage(0);
    while(GetMessageA(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    DestroyWindow(hMainWnd);

    test_SHAppBarMessage();
}
