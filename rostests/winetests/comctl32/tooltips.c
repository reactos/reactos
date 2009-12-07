/*
 * Copyright 2005 Dmitry Timoshkov
 * Copyright 2008 Jason Edmeades
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

#include <assert.h>
#include <windows.h>
#include <commctrl.h>

#include "wine/test.h"

static void test_create_tooltip(void)
{
    HWND parent, hwnd;
    DWORD style, exp_style;

    parent = CreateWindowEx(0, "static", NULL, WS_POPUP,
                          0, 0, 0, 0,
                          NULL, NULL, NULL, 0);
    assert(parent);

    hwnd = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, 0x7fffffff | WS_POPUP,
                          10, 10, 300, 100,
                          parent, NULL, NULL, 0);
    assert(hwnd);

    style = GetWindowLong(hwnd, GWL_STYLE);
    trace("style = %08x\n", style);
    exp_style = 0x7fffffff | WS_POPUP;
    exp_style &= ~(WS_CHILD | WS_MAXIMIZE | WS_BORDER | WS_DLGFRAME);
    ok(style == exp_style,"wrong style %08x/%08x\n", style, exp_style);

    DestroyWindow(hwnd);

    hwnd = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, 0,
                          10, 10, 300, 100,
                          parent, NULL, NULL, 0);
    assert(hwnd);

    style = GetWindowLong(hwnd, GWL_STYLE);
    trace("style = %08x\n", style);
    ok(style == (WS_POPUP | WS_CLIPSIBLINGS | WS_BORDER),
       "wrong style %08x\n", style);

    DestroyWindow(hwnd);

    DestroyWindow(parent);
}

/* try to make sure pending X events have been processed before continuing */
static void flush_events(int waitTime)
{
    MSG msg;
    int diff = waitTime;
    DWORD time = GetTickCount() + waitTime;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects( 0, NULL, FALSE, min(100,diff), QS_ALLEVENTS) == WAIT_TIMEOUT) break;
        while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessage( &msg );
        diff = time - GetTickCount();
    }
}

static int CD_Stages;
static LRESULT CD_Result;
static HWND g_hwnd;

#define TEST_CDDS_PREPAINT           0x00000001
#define TEST_CDDS_POSTPAINT          0x00000002
#define TEST_CDDS_PREERASE           0x00000004
#define TEST_CDDS_POSTERASE          0x00000008
#define TEST_CDDS_ITEMPREPAINT       0x00000010
#define TEST_CDDS_ITEMPOSTPAINT      0x00000020
#define TEST_CDDS_ITEMPREERASE       0x00000040
#define TEST_CDDS_ITEMPOSTERASE      0x00000080
#define TEST_CDDS_SUBITEM            0x00000100

static LRESULT CALLBACK CustomDrawWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_NOTIFY:
        if (((NMHDR *)lParam)->code == NM_CUSTOMDRAW) {
            NMTTCUSTOMDRAW *ttcd = (NMTTCUSTOMDRAW*) lParam;
            ok(ttcd->nmcd.hdr.hwndFrom == g_hwnd, "Unexpected hwnd source %p (%p)\n",
                 ttcd->nmcd.hdr.hwndFrom, g_hwnd);
            ok(ttcd->nmcd.hdr.idFrom == 0x1234ABCD, "Unexpected id %x\n", (int)ttcd->nmcd.hdr.idFrom);

            switch (ttcd->nmcd.dwDrawStage) {
            case CDDS_PREPAINT     : CD_Stages |= TEST_CDDS_PREPAINT; break;
            case CDDS_POSTPAINT    : CD_Stages |= TEST_CDDS_POSTPAINT; break;
            case CDDS_PREERASE     : CD_Stages |= TEST_CDDS_PREERASE; break;
            case CDDS_POSTERASE    : CD_Stages |= TEST_CDDS_POSTERASE; break;
            case CDDS_ITEMPREPAINT : CD_Stages |= TEST_CDDS_ITEMPREPAINT; break;
            case CDDS_ITEMPOSTPAINT: CD_Stages |= TEST_CDDS_ITEMPOSTPAINT; break;
            case CDDS_ITEMPREERASE : CD_Stages |= TEST_CDDS_ITEMPREERASE; break;
            case CDDS_ITEMPOSTERASE: CD_Stages |= TEST_CDDS_ITEMPOSTERASE; break;
            case CDDS_SUBITEM      : CD_Stages |= TEST_CDDS_SUBITEM; break;
            default: CD_Stages = -1;
            }

            if (ttcd->nmcd.dwDrawStage == CDDS_PREPAINT) return CD_Result;
        }
        /* drop through */

    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }

    return 0L;
}

static void test_customdraw(void) {
    static struct {
        LRESULT FirstReturnValue;
        int ExpectedCalls;
    } expectedResults[] = {
        /* Valid notification responses */
        {CDRF_DODEFAULT, TEST_CDDS_PREPAINT},
        {CDRF_SKIPDEFAULT, TEST_CDDS_PREPAINT},
        {CDRF_NOTIFYPOSTPAINT, TEST_CDDS_PREPAINT | TEST_CDDS_POSTPAINT},

        /* Invalid notification responses */
        {CDRF_NOTIFYITEMDRAW, TEST_CDDS_PREPAINT},
        {CDRF_NOTIFYPOSTERASE, TEST_CDDS_PREPAINT},
        {CDRF_NOTIFYSUBITEMDRAW, TEST_CDDS_PREPAINT},
        {CDRF_NEWFONT, TEST_CDDS_PREPAINT}
    };

   int       iterationNumber;
   WNDCLASSA wc;
   LRESULT   lResult;

   /* Create a class to use the custom draw wndproc */
   wc.style = CS_HREDRAW | CS_VREDRAW;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = GetModuleHandleA(NULL);
   wc.hIcon = NULL;
   wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
   wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
   wc.lpszMenuName = NULL;
   wc.lpszClassName = "CustomDrawClass";
   wc.lpfnWndProc = CustomDrawWndProc;
   RegisterClass(&wc);

   for (iterationNumber = 0;
        iterationNumber < sizeof(expectedResults)/sizeof(expectedResults[0]);
        iterationNumber++) {

       HWND parent, hwndTip;
       TOOLINFO toolInfo = { 0 };

       /* Create a main window */
       parent = CreateWindowEx(0, "CustomDrawClass", NULL,
                               WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                               WS_MAXIMIZEBOX | WS_VISIBLE,
                               50, 50,
                               300, 300,
                               NULL, NULL, NULL, 0);
       ok(parent != NULL, "Creation of main window failed\n");

       /* Make it show */
       ShowWindow(parent, SW_SHOWNORMAL);
       flush_events(100);

       /* Create Tooltip */
       hwndTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS,
                                NULL, TTS_NOPREFIX | TTS_ALWAYSTIP,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                parent, NULL, GetModuleHandleA(NULL), 0);
       ok(hwndTip != NULL, "Creation of tooltip window failed\n");

       /* Set up parms for the wndproc to handle */
       CD_Stages = 0;
       CD_Result = expectedResults[iterationNumber].FirstReturnValue;
       g_hwnd    = hwndTip;

       /* Make it topmost, as per the MSDN */
       SetWindowPos(hwndTip, HWND_TOPMOST, 0, 0, 0, 0,
             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

       /* Create a tool */
       toolInfo.cbSize = sizeof(TOOLINFO);
       toolInfo.hwnd = parent;
       toolInfo.hinst = GetModuleHandleA(NULL);
       toolInfo.uFlags = TTF_SUBCLASS;
       toolInfo.uId = 0x1234ABCD;
       toolInfo.lpszText = (LPSTR)"This is a test tooltip";
       toolInfo.lParam = 0xdeadbeef;
       GetClientRect (parent, &toolInfo.rect);
       lResult = SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
       ok(lResult, "Adding the tool to the tooltip failed\n");

       /* Make tooltip appear quickly */
       SendMessage(hwndTip, TTM_SETDELAYTIME, TTDT_INITIAL, MAKELPARAM(1,0));

       /* Put cursor inside window, tooltip will appear immediately */
       SetCursorPos(100, 100);
       flush_events(200);

       /* Check CustomDraw results */
       ok(CD_Stages == expectedResults[iterationNumber].ExpectedCalls,
          "CustomDraw run %d stages %x, expected %x\n", iterationNumber, CD_Stages,
          expectedResults[iterationNumber].ExpectedCalls);

       /* Clean up */
       DestroyWindow(hwndTip);
       DestroyWindow(parent);
   }


}

static void test_gettext(void)
{
    HWND hwnd;
    TTTOOLINFOA toolinfoA;
    TTTOOLINFOW toolinfoW;
    LRESULT r;
    char bufA[10] = "";
    WCHAR bufW[10] = { 0 };

    /* For bug 14790 - lpszText is NULL */
    hwnd = CreateWindowExA(0, TOOLTIPS_CLASSA, NULL, 0,
                           10, 10, 300, 100,
                           NULL, NULL, NULL, 0);
    assert(hwnd);

    toolinfoA.cbSize = sizeof(TTTOOLINFOA);
    toolinfoA.hwnd = NULL;
    toolinfoA.hinst = GetModuleHandleA(NULL);
    toolinfoA.uFlags = 0;
    toolinfoA.uId = 0x1234ABCD;
    toolinfoA.lpszText = NULL;
    toolinfoA.lParam = 0xdeadbeef;
    GetClientRect(hwnd, &toolinfoA.rect);
    r = SendMessageA(hwnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfoA);
    ok(r, "Adding the tool to the tooltip failed\n");
    if (r)
    {
        toolinfoA.hwnd = NULL;
        toolinfoA.uId = 0x1234ABCD;
        toolinfoA.lpszText = bufA;
        SendMessageA(hwnd, TTM_GETTEXTA, 0, (LPARAM)&toolinfoA);
        ok(strcmp(toolinfoA.lpszText, "") == 0, "lpszText should be an empty string\n");
    }

    DestroyWindow(hwnd);

    SetLastError(0xdeadbeef);
    hwnd = CreateWindowExW(0, TOOLTIPS_CLASSW, NULL, 0,
                           10, 10, 300, 100,
                           NULL, NULL, NULL, 0);

    if (!hwnd && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
        win_skip("CreateWindowExW is not implemented\n");
        return;
    }

    assert(hwnd);

    toolinfoW.cbSize = sizeof(TTTOOLINFOW);
    toolinfoW.hwnd = NULL;
    toolinfoW.hinst = GetModuleHandleA(NULL);
    toolinfoW.uFlags = 0;
    toolinfoW.uId = 0x1234ABCD;
    toolinfoW.lpszText = NULL;
    toolinfoW.lParam = 0xdeadbeef;
    GetClientRect(hwnd, &toolinfoW.rect);
    r = SendMessageW(hwnd, TTM_ADDTOOL, 0, (LPARAM)&toolinfoW);
    ok(r, "Adding the tool to the tooltip failed\n");

    if (0)  /* crashes on NT4 */
    {
        toolinfoW.hwnd = NULL;
        toolinfoW.uId = 0x1234ABCD;
        toolinfoW.lpszText = bufW;
        SendMessageW(hwnd, TTM_GETTEXTW, 0, (LPARAM)&toolinfoW);
        ok(toolinfoW.lpszText[0] == 0, "lpszText should be an empty string\n");
    }

    DestroyWindow(hwnd);
}

START_TEST(tooltips)
{
    InitCommonControls();

    test_create_tooltip();
    test_customdraw();
    test_gettext();
}
