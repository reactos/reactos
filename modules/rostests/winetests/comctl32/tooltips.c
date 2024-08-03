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

#include <windows.h>
#include <commctrl.h>

#include "resources.h"

#include "wine/test.h"

#include "v6util.h"
#include "msg.h"

#define expect(expected, got) ok(got == expected, "Expected %d, got %d\n", expected, got)

enum seq_index
{
    PARENT_SEQ_INDEX = 0,
    NUM_MSG_SEQUENCES
};

static struct msg_sequence *sequences[NUM_MSG_SEQUENCES];

static void test_create_tooltip(BOOL is_v6)
{
    HWND parent, hwnd;
    DWORD style, exp_style;

    parent = CreateWindowExA(0, "static", NULL, WS_POPUP,
                          0, 0, 0, 0,
                          NULL, NULL, NULL, 0);
    ok(parent != NULL, "failed to create parent wnd\n");

    hwnd = CreateWindowExA(0, TOOLTIPS_CLASSA, NULL, 0x7fffffff | WS_POPUP,
                          10, 10, 300, 100,
                          parent, NULL, NULL, 0);
    ok(hwnd != NULL, "failed to create tooltip wnd\n");

    style = GetWindowLongA(hwnd, GWL_STYLE);
    exp_style = 0x7fffffff | WS_POPUP;
    exp_style &= ~(WS_CHILD | WS_MAXIMIZE | WS_BORDER | WS_DLGFRAME);
    ok(style == exp_style || broken(style == (exp_style | WS_BORDER)), /* nt4 */
       "wrong style %08x/%08x\n", style, exp_style);

    DestroyWindow(hwnd);

    hwnd = CreateWindowExA(0, TOOLTIPS_CLASSA, NULL, 0,
                          10, 10, 300, 100,
                          parent, NULL, NULL, 0);
    ok(hwnd != NULL, "failed to create tooltip wnd\n");

    style = GetWindowLongA(hwnd, GWL_STYLE);
    exp_style = WS_POPUP | WS_CLIPSIBLINGS;
    if (!is_v6)
        exp_style |= WS_BORDER;
todo_wine_if(is_v6)
    ok(style == exp_style || broken(style == (exp_style | WS_BORDER)) /* XP */,
        "Unexpected window style %#x.\n", style);

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
        while (PeekMessageA( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
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

static LRESULT CALLBACK custom_draw_wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
        {CDRF_NEWFONT, TEST_CDDS_PREPAINT}
    };

   DWORD       iterationNumber;
   WNDCLASSA wc;
   POINT orig_pos;
   LRESULT ret;

   /* Create a class to use the custom draw wndproc */
   wc.style = CS_HREDRAW | CS_VREDRAW;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = GetModuleHandleA(NULL);
   wc.hIcon = NULL;
   wc.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
   wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
   wc.lpszMenuName = NULL;
   wc.lpszClassName = "CustomDrawClass";
   wc.lpfnWndProc = custom_draw_wnd_proc;
   RegisterClassA(&wc);

   GetCursorPos(&orig_pos);

   for (iterationNumber = 0;
        iterationNumber < ARRAY_SIZE(expectedResults);
        iterationNumber++) {

       HWND parent, hwndTip;
       RECT rect;
       TTTOOLINFOA toolInfo = { 0 };

       /* Create a main window */
       parent = CreateWindowExA(0, "CustomDrawClass", NULL,
                               WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                               WS_MAXIMIZEBOX | WS_VISIBLE,
                               50, 50,
                               300, 300,
                               NULL, NULL, NULL, 0);
       ok(parent != NULL, "%d: Creation of main window failed\n", iterationNumber);

       /* Make it show */
       ShowWindow(parent, SW_SHOWNORMAL);
       flush_events(100);

       /* Create Tooltip */
       hwndTip = CreateWindowExA(WS_EX_TOPMOST, TOOLTIPS_CLASSA,
                                NULL, TTS_NOPREFIX | TTS_ALWAYSTIP,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                parent, NULL, GetModuleHandleA(NULL), 0);
       ok(hwndTip != NULL, "%d: Creation of tooltip window failed\n", iterationNumber);

       /* Set up parms for the wndproc to handle */
       CD_Stages = 0;
       CD_Result = expectedResults[iterationNumber].FirstReturnValue;
       g_hwnd    = hwndTip;

       /* Make it topmost, as per the MSDN */
       SetWindowPos(hwndTip, HWND_TOPMOST, 0, 0, 0, 0,
             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

       /* Create a tool */
       toolInfo.cbSize = TTTOOLINFOA_V1_SIZE;
       toolInfo.hwnd = parent;
       toolInfo.hinst = GetModuleHandleA(NULL);
       toolInfo.uFlags = TTF_SUBCLASS;
       toolInfo.uId = 0x1234ABCD;
       toolInfo.lpszText = (LPSTR)"This is a test tooltip";
       toolInfo.lParam = 0xdeadbeef;
       GetClientRect (parent, &toolInfo.rect);
       ret = SendMessageA(hwndTip, TTM_ADDTOOLA, 0, (LPARAM)&toolInfo);
       ok(ret, "%d: Failed to add the tool.\n", iterationNumber);

       /* Make tooltip appear quickly */
       SendMessageA(hwndTip, TTM_SETDELAYTIME, TTDT_INITIAL, MAKELPARAM(1,0));

       /* Put cursor inside window, tooltip will appear immediately */
       GetWindowRect( parent, &rect );
       SetCursorPos( (rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2 );
       flush_events(200);

       if (CD_Stages)
       {
           /* Check CustomDraw results */
           ok(CD_Stages == expectedResults[iterationNumber].ExpectedCalls ||
              broken(CD_Stages == (expectedResults[iterationNumber].ExpectedCalls & ~TEST_CDDS_POSTPAINT)), /* nt4 */
              "%d: CustomDraw stages %x, expected %x\n", iterationNumber, CD_Stages,
              expectedResults[iterationNumber].ExpectedCalls);
       }

       ret = SendMessageA(hwndTip, TTM_GETCURRENTTOOLA, 0, 0);
       ok(ret, "%d: Failed to get current tool %#lx.\n", iterationNumber, ret);

       memset(&toolInfo, 0xcc, sizeof(toolInfo));
       toolInfo.cbSize = sizeof(toolInfo);
       toolInfo.lpszText = NULL;
       toolInfo.lpReserved = (void *)0xdeadbeef;
       SendMessageA(hwndTip, TTM_GETCURRENTTOOLA, 0, (LPARAM)&toolInfo);
       ok(toolInfo.hwnd == parent, "%d: Unexpected hwnd %p.\n", iterationNumber, toolInfo.hwnd);
       ok(toolInfo.hinst == GetModuleHandleA(NULL), "%d: Unexpected hinst %p.\n", iterationNumber, toolInfo.hinst);
       ok(toolInfo.uId == 0x1234abcd, "%d: Unexpected uId %lx.\n", iterationNumber, toolInfo.uId);
       ok(toolInfo.lParam == 0, "%d: Unexpected lParam %lx.\n", iterationNumber, toolInfo.lParam);
       ok(toolInfo.lpReserved == (void *)0xdeadbeef, "%d: Unexpected lpReserved %p.\n", iterationNumber, toolInfo.lpReserved);

       /* Clean up */
       DestroyWindow(hwndTip);
       DestroyWindow(parent);
   }

   SetCursorPos(orig_pos.x, orig_pos.y);
}

static const CHAR testcallbackA[]  = "callback";

static RECT g_ttip_rect;
static LRESULT WINAPI parent_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LONG defwndproc_counter = 0;
    struct message msg;
    LRESULT ret;

    if (message == WM_NOTIFY && lParam)
    {
        NMTTDISPINFOA *ttnmdi = (NMTTDISPINFOA*)lParam;
        NMHDR *hdr = (NMHDR *)lParam;
        RECT rect;

        if (hdr->code != NM_CUSTOMDRAW)
        {
            msg.message = message;
            msg.flags = sent|wparam|lparam;
            if (defwndproc_counter) msg.flags |= defwinproc;
            msg.wParam = wParam;
            msg.lParam = lParam;
            msg.id = hdr->code;
            add_message(sequences, PARENT_SEQ_INDEX, &msg);
        }

        switch (hdr->code)
        {
        case TTN_GETDISPINFOA:
            lstrcpyA(ttnmdi->lpszText, testcallbackA);
            break;
        case TTN_SHOW:
            GetWindowRect(hdr->hwndFrom, &rect);
            ok(!EqualRect(&g_ttip_rect, &rect), "Unexpected window rectangle.\n");
            break;
        }
    }

    defwndproc_counter++;
    if (IsWindowUnicode(hwnd))
        ret = DefWindowProcW(hwnd, message, wParam, lParam);
    else
        ret = DefWindowProcA(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static void register_parent_wnd_class(void)
{
    WNDCLASSA cls;
    BOOL ret;

    cls.style = 0;
    cls.lpfnWndProc = parent_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "Tooltips test parent class";
    ret = RegisterClassA(&cls);
    ok(ret, "Failed to register test parent class.\n");
}

static HWND create_parent_window(void)
{
    return CreateWindowExA(0, "Tooltips test parent class",
                          "Tooltips test parent window",
                          WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                          WS_MAXIMIZEBOX | WS_VISIBLE,
                          0, 0, 100, 100,
                          GetDesktopWindow(), NULL, GetModuleHandleA(NULL), NULL);
}

static void test_gettext(void)
{
    static const CHAR testtip2A[] = "testtip\ttest2";
    static const CHAR testtipA[] = "testtip";
    HWND hwnd, notify;
    TTTOOLINFOA toolinfoA;
    TTTOOLINFOW toolinfoW;
    LRESULT r;
    CHAR bufA[16] = "";
    WCHAR bufW[10] = { 0 };
    DWORD length, style;

    notify = create_parent_window();
    ok(notify != NULL, "Expected notification window to be created\n");

    /* For bug 14790 - lpszText is NULL */
    hwnd = CreateWindowExA(0, TOOLTIPS_CLASSA, NULL, 0,
                           10, 10, 300, 100,
                           NULL, NULL, NULL, 0);
    ok(hwnd != NULL, "failed to create tooltip wnd\n");

    /* use sizeof(TTTOOLINFOA) instead of TTTOOLINFOA_V1_SIZE so that adding it fails on Win9x */
    /* otherwise it crashes on the NULL lpszText */
    toolinfoA.cbSize = sizeof(TTTOOLINFOA);
    toolinfoA.hwnd = NULL;
    toolinfoA.hinst = GetModuleHandleA(NULL);
    toolinfoA.uFlags = 0;
    toolinfoA.uId = 0x1234ABCD;
    toolinfoA.lpszText = NULL;
    toolinfoA.lParam = 0xdeadbeef;
    GetClientRect(hwnd, &toolinfoA.rect);
    r = SendMessageA(hwnd, TTM_ADDTOOLA, 0, (LPARAM)&toolinfoA);
    ok(r, "got %ld\n", r);

    toolinfoA.hwnd = NULL;
    toolinfoA.uId = 0x1234abcd;
    toolinfoA.lpszText = bufA;
    r = SendMessageA(hwnd, TTM_GETTEXTA, 0, (LPARAM)&toolinfoA);
    ok(!r, "got %ld\n", r);
    ok(!*toolinfoA.lpszText, "lpszText should be empty, got %s\n", toolinfoA.lpszText);

    toolinfoA.lpszText = bufA;
    r = SendMessageA(hwnd, TTM_GETTOOLINFOA, 0, (LPARAM)&toolinfoA);
todo_wine
    ok(!r, "got %ld\n", r);
    ok(toolinfoA.lpszText == NULL, "expected NULL, got %p\n", toolinfoA.lpszText);

    /* NULL hinst, valid resource id for text */
    toolinfoA.cbSize = sizeof(TTTOOLINFOA);
    toolinfoA.hwnd = NULL;
    toolinfoA.hinst = NULL;
    toolinfoA.uFlags = 0;
    toolinfoA.uId = 0x1233abcd;
    toolinfoA.lpszText = MAKEINTRESOURCEA(IDS_TBADD1);
    toolinfoA.lParam = 0xdeadbeef;
    GetClientRect(hwnd, &toolinfoA.rect);
    r = SendMessageA(hwnd, TTM_ADDTOOLA, 0, (LPARAM)&toolinfoA);
    ok(r, "failed to add a tool\n");

    toolinfoA.hwnd = NULL;
    toolinfoA.uId = 0x1233abcd;
    toolinfoA.lpszText = bufA;
    r = SendMessageA(hwnd, TTM_GETTEXTA, 0, (LPARAM)&toolinfoA);
    ok(!r, "got %ld\n", r);
    ok(!strcmp(toolinfoA.lpszText, "abc"), "got wrong text, %s\n", toolinfoA.lpszText);

    toolinfoA.hinst = (HINSTANCE)0xdeadbee;
    r = SendMessageA(hwnd, TTM_GETTOOLINFOA, 0, (LPARAM)&toolinfoA);
todo_wine
    ok(!r, "got %ld\n", r);
    ok(toolinfoA.hinst == NULL, "expected NULL, got %p\n", toolinfoA.hinst);

    r = SendMessageA(hwnd, TTM_DELTOOLA, 0, (LPARAM)&toolinfoA);
    ok(!r, "got %ld\n", r);

    /* add another tool with text */
    toolinfoA.cbSize = sizeof(TTTOOLINFOA);
    toolinfoA.hwnd = NULL;
    toolinfoA.hinst = GetModuleHandleA(NULL);
    toolinfoA.uFlags = 0;
    toolinfoA.uId = 0x1235ABCD;
    strcpy(bufA, testtipA);
    toolinfoA.lpszText = bufA;
    toolinfoA.lParam = 0xdeadbeef;
    GetClientRect(hwnd, &toolinfoA.rect);
    r = SendMessageA(hwnd, TTM_ADDTOOLA, 0, (LPARAM)&toolinfoA);
    ok(r, "Adding the tool to the tooltip failed\n");

    length = SendMessageA(hwnd, WM_GETTEXTLENGTH, 0, 0);
    ok(length == 0, "Expected 0, got %d\n", length);

    toolinfoA.hwnd = NULL;
    toolinfoA.uId = 0x1235abcd;
    toolinfoA.lpszText = bufA;
    r = SendMessageA(hwnd, TTM_GETTEXTA, 0, (LPARAM)&toolinfoA);
    ok(!r, "got %ld\n", r);
    ok(!strcmp(toolinfoA.lpszText, testtipA), "expected %s, got %p\n", testtipA, toolinfoA.lpszText);

    memset(bufA, 0x1f, sizeof(bufA));
    toolinfoA.lpszText = bufA;
    r = SendMessageA(hwnd, TTM_GETTOOLINFOA, 0, (LPARAM)&toolinfoA);
todo_wine
    ok(!r, "got %ld\n", r);
    ok(!strcmp(toolinfoA.lpszText, testtipA), "expected %s, got %p\n", testtipA, toolinfoA.lpszText);

    length = SendMessageA(hwnd, WM_GETTEXTLENGTH, 0, 0);
    ok(length == 0, "Expected 0, got %d\n", length);

    /* add another with callback text */
    toolinfoA.cbSize = sizeof(TTTOOLINFOA);
    toolinfoA.hwnd = notify;
    toolinfoA.hinst = GetModuleHandleA(NULL);
    toolinfoA.uFlags = 0;
    toolinfoA.uId = 0x1236ABCD;
    toolinfoA.lpszText = LPSTR_TEXTCALLBACKA;
    toolinfoA.lParam = 0xdeadbeef;
    GetClientRect(hwnd, &toolinfoA.rect);
    r = SendMessageA(hwnd, TTM_ADDTOOLA, 0, (LPARAM)&toolinfoA);
    ok(r, "Adding the tool to the tooltip failed\n");

    toolinfoA.hwnd = notify;
    toolinfoA.uId = 0x1236abcd;
    toolinfoA.lpszText = bufA;
    r = SendMessageA(hwnd, TTM_GETTEXTA, 0, (LPARAM)&toolinfoA);
    ok(!r, "got %ld\n", r);
    ok(!strcmp(toolinfoA.lpszText, testcallbackA), "lpszText should be an (%s) string\n", testcallbackA);

    toolinfoA.lpszText = bufA;
    r = SendMessageA(hwnd, TTM_GETTOOLINFOA, 0, (LPARAM)&toolinfoA);
todo_wine
    ok(!r, "got %ld\n", r);
    ok(toolinfoA.lpszText == LPSTR_TEXTCALLBACKA, "expected LPSTR_TEXTCALLBACKA, got %p\n", toolinfoA.lpszText);

    DestroyWindow(hwnd);
    DestroyWindow(notify);

    hwnd = CreateWindowExW(0, TOOLTIPS_CLASSW, NULL, 0,
                           10, 10, 300, 100,
                           NULL, NULL, NULL, 0);
    ok(hwnd != NULL, "failed to create tooltip wnd\n");

    toolinfoW.cbSize = TTTOOLINFOW_V2_SIZE + 1;
    toolinfoW.hwnd = NULL;
    toolinfoW.hinst = GetModuleHandleA(NULL);
    toolinfoW.uFlags = 0;
    toolinfoW.uId = 0x1234ABCD;
    toolinfoW.lpszText = NULL;
    toolinfoW.lParam = 0xdeadbeef;
    GetClientRect(hwnd, &toolinfoW.rect);
    r = SendMessageW(hwnd, TTM_ADDTOOLW, 0, (LPARAM)&toolinfoW);
    /* Wine currently checks for V3 structure size, which matches what V6 control does.
       Older implementation was never updated to support lpReserved field. */
todo_wine
    ok(!r, "Adding the tool to the tooltip succeeded!\n");

    if (0)  /* crashes on NT4 */
    {
        toolinfoW.hwnd = NULL;
        toolinfoW.uId = 0x1234ABCD;
        toolinfoW.lpszText = bufW;
        SendMessageW(hwnd, TTM_GETTEXTW, 0, (LPARAM)&toolinfoW);
        ok(toolinfoW.lpszText[0] == 0, "lpszText should be an empty string\n");
    }

    /* text with embedded tabs */
    toolinfoA.cbSize = sizeof(TTTOOLINFOA);
    toolinfoA.hwnd = NULL;
    toolinfoA.hinst = GetModuleHandleA(NULL);
    toolinfoA.uFlags = 0;
    toolinfoA.uId = 0x1235abce;
    strcpy(bufA, testtip2A);
    toolinfoA.lpszText = bufA;
    toolinfoA.lParam = 0xdeadbeef;
    GetClientRect(hwnd, &toolinfoA.rect);
    r = SendMessageA(hwnd, TTM_ADDTOOLA, 0, (LPARAM)&toolinfoA);
    ok(r, "got %ld\n", r);

    toolinfoA.hwnd = NULL;
    toolinfoA.uId = 0x1235abce;
    toolinfoA.lpszText = bufA;
    r = SendMessageA(hwnd, TTM_GETTEXTA, 0, (LPARAM)&toolinfoA);
    ok(!r, "got %ld\n", r);
    ok(!strcmp(toolinfoA.lpszText, testtipA), "expected %s, got %s\n", testtipA, toolinfoA.lpszText);

    /* enable TTS_NOPREFIX, original text is retained */
    style = GetWindowLongA(hwnd, GWL_STYLE);
    SetWindowLongA(hwnd, GWL_STYLE, style | TTS_NOPREFIX);

    toolinfoA.hwnd = NULL;
    toolinfoA.uId = 0x1235abce;
    toolinfoA.lpszText = bufA;
    r = SendMessageA(hwnd, TTM_GETTEXTA, 0, (LPARAM)&toolinfoA);
    ok(!r, "got %ld\n", r);
    ok(!strcmp(toolinfoA.lpszText, testtip2A), "expected %s, got %s\n", testtip2A, toolinfoA.lpszText);

    DestroyWindow(hwnd);
}

static void test_ttm_gettoolinfo(void)
{
    TTTOOLINFOA ti;
    TTTOOLINFOW tiW;
    HWND hwnd;
    DWORD r;

    hwnd = CreateWindowExA(0, TOOLTIPS_CLASSA, NULL, 0, 10, 10, 300, 100, NULL, NULL, NULL, 0);
    ok(hwnd != NULL, "Failed to create tooltip control.\n");

    ti.cbSize = TTTOOLINFOA_V2_SIZE;
    ti.hwnd = NULL;
    ti.hinst = GetModuleHandleA(NULL);
    ti.uFlags = 0;
    ti.uId = 0x1234ABCD;
    ti.lpszText = NULL;
    ti.lParam = 0x1abe11ed;
    GetClientRect(hwnd, &ti.rect);
    r = SendMessageA(hwnd, TTM_ADDTOOLA, 0, (LPARAM)&ti);
    ok(r, "Adding the tool to the tooltip failed\n");

    ti.cbSize = TTTOOLINFOA_V2_SIZE;
    ti.lParam = 0xaaaaaaaa;
    r = SendMessageA(hwnd, TTM_GETTOOLINFOA, 0, (LPARAM)&ti);
    ok(r, "Getting tooltip info failed\n");
    ok(0x1abe11ed == ti.lParam ||
       broken(0x1abe11ed != ti.lParam), /* comctl32 < 5.81 */
       "Expected 0x1abe11ed, got %lx\n", ti.lParam);

    tiW.cbSize = TTTOOLINFOW_V2_SIZE;
    tiW.hwnd = NULL;
    tiW.uId = 0x1234ABCD;
    tiW.lParam = 0xaaaaaaaa;
    tiW.lpszText = NULL;
    r = SendMessageA(hwnd, TTM_GETTOOLINFOW, 0, (LPARAM)&tiW);
    ok(r, "Getting tooltip info failed\n");
    ok(0x1abe11ed == tiW.lParam ||
       broken(0x1abe11ed != tiW.lParam), /* comctl32 < 5.81 */
       "Expected 0x1abe11ed, got %lx\n", tiW.lParam);

    ti.cbSize = TTTOOLINFOA_V2_SIZE;
    ti.uId = 0x1234ABCD;
    ti.lParam = 0x55555555;
    SendMessageA(hwnd, TTM_SETTOOLINFOA, 0, (LPARAM)&ti);

    ti.cbSize = TTTOOLINFOA_V2_SIZE;
    ti.lParam = 0xdeadbeef;
    r = SendMessageA(hwnd, TTM_GETTOOLINFOA, 0, (LPARAM)&ti);
    ok(r, "Getting tooltip info failed\n");
    ok(0x55555555 == ti.lParam ||
       broken(0x55555555 != ti.lParam), /* comctl32 < 5.81 */
       "Expected 0x55555555, got %lx\n", ti.lParam);

    DestroyWindow(hwnd);

    /* 1. test size parameter validation rules (ansi messages) */
    hwnd = CreateWindowExA(0, TOOLTIPS_CLASSA, NULL, 0,
                           10, 10, 300, 100,
                           NULL, NULL, NULL, 0);

    ti.cbSize = TTTOOLINFOA_V1_SIZE - 1;
    ti.hwnd = NULL;
    ti.hinst = GetModuleHandleA(NULL);
    ti.uFlags = 0;
    ti.uId = 0x1234ABCD;
    ti.lpszText = NULL;
    ti.lParam = 0xdeadbeef;
    GetClientRect(hwnd, &ti.rect);
    r = SendMessageA(hwnd, TTM_ADDTOOLA, 0, (LPARAM)&ti);
    ok(r, "Adding the tool to the tooltip failed\n");
    r = SendMessageA(hwnd, TTM_GETTOOLCOUNT, 0, 0);
    expect(1, r);

    ti.cbSize = TTTOOLINFOA_V1_SIZE - 1;
    ti.hwnd = NULL;
    ti.uId = 0x1234ABCD;
    SendMessageA(hwnd, TTM_DELTOOLA, 0, (LPARAM)&ti);
    r = SendMessageA(hwnd, TTM_GETTOOLCOUNT, 0, 0);
    expect(0, r);

    ti.cbSize = TTTOOLINFOA_V2_SIZE - 1;
    ti.hwnd = NULL;
    ti.hinst = GetModuleHandleA(NULL);
    ti.uFlags = 0;
    ti.uId = 0x1234ABCD;
    ti.lpszText = NULL;
    ti.lParam = 0xdeadbeef;
    GetClientRect(hwnd, &ti.rect);
    r = SendMessageA(hwnd, TTM_ADDTOOLA, 0, (LPARAM)&ti);
    ok(r, "Adding the tool to the tooltip failed\n");
    r = SendMessageA(hwnd, TTM_GETTOOLCOUNT, 0, 0);
    expect(1, r);

    ti.cbSize = TTTOOLINFOA_V2_SIZE - 1;
    ti.hwnd = NULL;
    ti.uId = 0x1234ABCD;
    SendMessageA(hwnd, TTM_DELTOOLA, 0, (LPARAM)&ti);
    r = SendMessageA(hwnd, TTM_GETTOOLCOUNT, 0, 0);
    expect(0, r);

    ti.cbSize = TTTOOLINFOA_V2_SIZE + 1;
    ti.hwnd = NULL;
    ti.hinst = GetModuleHandleA(NULL);
    ti.uFlags = 0;
    ti.uId = 0x1234ABCD;
    ti.lpszText = NULL;
    ti.lParam = 0xdeadbeef;
    GetClientRect(hwnd, &ti.rect);
    r = SendMessageA(hwnd, TTM_ADDTOOLA, 0, (LPARAM)&ti);
    ok(r, "Adding the tool to the tooltip failed\n");
    r = SendMessageA(hwnd, TTM_GETTOOLCOUNT, 0, 0);
    expect(1, r);

    ti.cbSize = TTTOOLINFOA_V2_SIZE + 1;
    ti.hwnd = NULL;
    ti.uId = 0x1234ABCD;
    SendMessageA(hwnd, TTM_DELTOOLA, 0, (LPARAM)&ti);
    r = SendMessageA(hwnd, TTM_GETTOOLCOUNT, 0, 0);
    expect(0, r);

    DestroyWindow(hwnd);

    /* 2. test size parameter validation rules (w-messages) */
    hwnd = CreateWindowExW(0, TOOLTIPS_CLASSW, NULL, 0,
                           10, 10, 300, 100,
                           NULL, NULL, NULL, 0);
    ok(hwnd != NULL, "Failed to create tooltip window.\n");

    tiW.cbSize = TTTOOLINFOW_V1_SIZE - 1;
    tiW.hwnd = NULL;
    tiW.hinst = GetModuleHandleA(NULL);
    tiW.uFlags = 0;
    tiW.uId = 0x1234ABCD;
    tiW.lpszText = NULL;
    tiW.lParam = 0xdeadbeef;
    GetClientRect(hwnd, &tiW.rect);
    r = SendMessageW(hwnd, TTM_ADDTOOLW, 0, (LPARAM)&tiW);
    ok(r, "Adding the tool to the tooltip failed\n");
    r = SendMessageW(hwnd, TTM_GETTOOLCOUNT, 0, 0);
    expect(1, r);

    tiW.cbSize = TTTOOLINFOW_V1_SIZE - 1;
    tiW.hwnd = NULL;
    tiW.uId = 0x1234ABCD;
    SendMessageW(hwnd, TTM_DELTOOLW, 0, (LPARAM)&tiW);
    r = SendMessageW(hwnd, TTM_GETTOOLCOUNT, 0, 0);
    expect(0, r);

    tiW.cbSize = TTTOOLINFOW_V2_SIZE - 1;
    tiW.hwnd = NULL;
    tiW.hinst = GetModuleHandleA(NULL);
    tiW.uFlags = 0;
    tiW.uId = 0x1234ABCD;
    tiW.lpszText = NULL;
    tiW.lParam = 0xdeadbeef;
    GetClientRect(hwnd, &tiW.rect);
    r = SendMessageW(hwnd, TTM_ADDTOOLW, 0, (LPARAM)&tiW);
    ok(r, "Adding the tool to the tooltip failed\n");
    r = SendMessageW(hwnd, TTM_GETTOOLCOUNT, 0, 0);
    expect(1, r);

    tiW.cbSize = TTTOOLINFOW_V2_SIZE - 1;
    tiW.hwnd = NULL;
    tiW.uId = 0x1234ABCD;
    SendMessageW(hwnd, TTM_DELTOOLW, 0, (LPARAM)&tiW);
    r = SendMessageW(hwnd, TTM_GETTOOLCOUNT, 0, 0);
    expect(0, r);

    tiW.cbSize = TTTOOLINFOW_V2_SIZE + 1;
    tiW.hwnd = NULL;
    tiW.hinst = GetModuleHandleA(NULL);
    tiW.uFlags = 0;
    tiW.uId = 0x1234ABCD;
    tiW.lpszText = NULL;
    tiW.lParam = 0xdeadbeef;
    GetClientRect(hwnd, &tiW.rect);
    r = SendMessageW(hwnd, TTM_ADDTOOLA, 0, (LPARAM)&tiW);
    ok(r, "Adding the tool to the tooltip failed\n");
    r = SendMessageW(hwnd, TTM_GETTOOLCOUNT, 0, 0);
    expect(1, r);
    /* looks like TTM_DELTOOLW doesn't work with invalid size */
    tiW.cbSize = TTTOOLINFOW_V2_SIZE + 1;
    tiW.hwnd = NULL;
    tiW.uId = 0x1234ABCD;
    SendMessageW(hwnd, TTM_DELTOOLW, 0, (LPARAM)&tiW);
    r = SendMessageW(hwnd, TTM_GETTOOLCOUNT, 0, 0);
    expect(1, r);

    tiW.cbSize = TTTOOLINFOW_V2_SIZE;
    tiW.hwnd = NULL;
    tiW.uId = 0x1234ABCD;
    SendMessageW(hwnd, TTM_DELTOOLW, 0, (LPARAM)&tiW);
    r = SendMessageW(hwnd, TTM_GETTOOLCOUNT, 0, 0);
    expect(0, r);

    DestroyWindow(hwnd);
}

static void test_longtextA(void)
{
    static const char longtextA[] =
        "According to MSDN, TTM_ENUMTOOLS claims that TOOLINFO's lpszText is maximum "
        "80 chars including null. In fact, the buffer is not null-terminated.";
    HWND hwnd;
    TTTOOLINFOA toolinfoA = { 0 };
    CHAR bufA[256];
    LRESULT r;

    hwnd = CreateWindowExA(0, TOOLTIPS_CLASSA, NULL, 0,
                           10, 10, 300, 100,
                           NULL, NULL, NULL, 0);
    toolinfoA.cbSize = sizeof(TTTOOLINFOA);
    toolinfoA.hwnd = NULL;
    toolinfoA.hinst = GetModuleHandleA(NULL);
    toolinfoA.uFlags = 0;
    toolinfoA.uId = 0x1234ABCD;
    strcpy(bufA, longtextA);
    toolinfoA.lpszText = bufA;
    toolinfoA.lParam = 0xdeadbeef;
    GetClientRect(hwnd, &toolinfoA.rect);
    r = SendMessageA(hwnd, TTM_ADDTOOLA, 0, (LPARAM)&toolinfoA);
    if (r)
    {
        int textlen;
        memset(bufA, 0, sizeof(bufA));
        toolinfoA.hwnd = NULL;
        toolinfoA.uId = 0xABCD1234;
        toolinfoA.lpszText = bufA;
        SendMessageA(hwnd, TTM_ENUMTOOLSA, 0, (LPARAM)&toolinfoA);
        textlen = lstrlenA(toolinfoA.lpszText);
        ok(textlen == 80, "lpszText has %d chars\n", textlen);
        ok(toolinfoA.uId == 0x1234ABCD,
           "uId should be retrieved, got %p\n", (void*)toolinfoA.uId);

        memset(bufA, 0, sizeof(bufA));
        toolinfoA.hwnd = NULL;
        toolinfoA.uId = 0x1234ABCD;
        toolinfoA.lpszText = bufA;
        SendMessageA(hwnd, TTM_GETTOOLINFOA, 0, (LPARAM)&toolinfoA);
        textlen = lstrlenA(toolinfoA.lpszText);
        ok(textlen == 80, "lpszText has %d chars\n", textlen);

        memset(bufA, 0, sizeof(bufA));
        toolinfoA.hwnd = NULL;
        toolinfoA.uId = 0x1234ABCD;
        toolinfoA.lpszText = bufA;
        SendMessageA(hwnd, TTM_GETTEXTA, 0, (LPARAM)&toolinfoA);
        textlen = lstrlenA(toolinfoA.lpszText);
        ok(textlen == 80, "lpszText has %d chars\n", textlen);
    }

    DestroyWindow(hwnd);
}

static void test_longtextW(void)
{
    static const char longtextA[] =
        "According to MSDN, TTM_ENUMTOOLS claims that TOOLINFO's lpszText is maximum "
        "80 chars including null. Actually, this is not applied for wide version.";
    HWND hwnd;
    TTTOOLINFOW toolinfoW = { 0 };
    WCHAR bufW[256];
    LRESULT r;
    int lenW;

    hwnd = CreateWindowExW(0, TOOLTIPS_CLASSW, NULL, 0,
                           10, 10, 300, 100,
                           NULL, NULL, NULL, 0);
    ok(hwnd != NULL, "Failed to create tooltip window.\n");

    toolinfoW.cbSize = TTTOOLINFOW_V2_SIZE;
    toolinfoW.hwnd = NULL;
    toolinfoW.hinst = GetModuleHandleW(NULL);
    toolinfoW.uFlags = 0;
    toolinfoW.uId = 0x1234ABCD;
    MultiByteToWideChar(CP_ACP, 0, longtextA, -1, bufW, ARRAY_SIZE(bufW));
    lenW = lstrlenW(bufW);
    toolinfoW.lpszText = bufW;
    toolinfoW.lParam = 0xdeadbeef;
    GetClientRect(hwnd, &toolinfoW.rect);
    r = SendMessageW(hwnd, TTM_ADDTOOLW, 0, (LPARAM)&toolinfoW);
    if (r)
    {
        int textlen;
        memset(bufW, 0, sizeof(bufW));
        toolinfoW.hwnd = NULL;
        toolinfoW.uId = 0xABCD1234;
        toolinfoW.lpszText = bufW;
        SendMessageW(hwnd, TTM_ENUMTOOLSW, 0, (LPARAM)&toolinfoW);
        textlen = lstrlenW(toolinfoW.lpszText);
        ok(textlen == lenW, "lpszText has %d chars\n", textlen);
        ok(toolinfoW.uId == 0x1234ABCD,
           "uId should be retrieved, got %p\n", (void*)toolinfoW.uId);

        memset(bufW, 0, sizeof(bufW));
        toolinfoW.hwnd = NULL;
        toolinfoW.uId = 0x1234ABCD;
        toolinfoW.lpszText = bufW;
        SendMessageW(hwnd, TTM_GETTOOLINFOW, 0, (LPARAM)&toolinfoW);
        textlen = lstrlenW(toolinfoW.lpszText);
        ok(textlen == lenW, "lpszText has %d chars\n", textlen);

        memset(bufW, 0, sizeof(bufW));
        toolinfoW.hwnd = NULL;
        toolinfoW.uId = 0x1234ABCD;
        toolinfoW.lpszText = bufW;
        SendMessageW(hwnd, TTM_GETTEXTW, 0, (LPARAM)&toolinfoW);
        textlen = lstrlenW(toolinfoW.lpszText);
        ok(textlen == lenW ||
           broken(textlen == 0 && toolinfoW.lpszText == NULL), /* nt4, kb186177 */
           "lpszText has %d chars\n", textlen);
    }

    DestroyWindow(hwnd);
}

static BOOL almost_eq(int a, int b)
{
    return a-5<b && a+5>b;
}

static void test_track(void)
{
    WCHAR textW[] = {'t','e','x','t',0};
    TTTOOLINFOW info = { 0 };
    HWND parent, tt;
    LRESULT res;
    RECT pos;

    parent = CreateWindowExW(0, WC_STATICW, NULL, WS_CAPTION | WS_VISIBLE,
            50, 50, 300, 300, NULL, NULL, NULL, 0);
    ok(parent != NULL, "creation of parent window failed\n");

    ShowWindow(parent, SW_SHOWNORMAL);
    flush_events(100);

    tt = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL, TTS_NOPREFIX | TTS_ALWAYSTIP,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            parent, NULL, GetModuleHandleW(NULL), 0);
    ok(tt != NULL, "creation of tooltip window failed\n");

    info.cbSize = TTTOOLINFOW_V1_SIZE;
    info.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
    info.hwnd = parent;
    info.hinst = GetModuleHandleW(NULL);
    info.lpszText = textW;
    info.uId = (UINT_PTR)parent;
    GetClientRect(parent, &info.rect);

    res = SendMessageW(tt, TTM_ADDTOOLW, 0, (LPARAM)&info);
    ok(res, "adding the tool to the tooltip failed\n");

    SendMessageW(tt, TTM_SETDELAYTIME, TTDT_INITIAL, MAKELPARAM(1,0));
    SendMessageW(tt, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&info);
    SendMessageW(tt, TTM_TRACKPOSITION, 0, MAKELPARAM(10, 10));

    GetWindowRect(tt, &pos);
    ok(almost_eq(pos.left, 10), "pos.left = %d\n", pos.left);
    ok(almost_eq(pos.top, 10), "pos.top = %d\n", pos.top);

    info.uFlags = TTF_IDISHWND | TTF_ABSOLUTE;
    SendMessageW(tt, TTM_SETTOOLINFOW, 0, (LPARAM)&info);
    SendMessageW(tt, TTM_TRACKPOSITION, 0, MAKELPARAM(10, 10));

    GetWindowRect(tt, &pos);
    ok(!almost_eq(pos.left, 10), "pos.left = %d\n", pos.left);
    ok(!almost_eq(pos.top, 10), "pos.top = %d\n", pos.top);

    DestroyWindow(tt);
    DestroyWindow(parent);
}

static LRESULT CALLBACK info_wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcA(hWnd, msg, wParam, lParam);
    }
    return 0;
}

static void test_setinfo(BOOL is_v6)
{
   WNDCLASSA wc;
   LRESULT   lResult;
   HWND parent, parent2, hwndTip, hwndTip2;
   TTTOOLINFOA toolInfo = { 0 };
   TTTOOLINFOA toolInfo2 = { 0 };
   WNDPROC wndProc;

   /* Create a class to use the custom draw wndproc */
   wc.style = CS_HREDRAW | CS_VREDRAW;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = GetModuleHandleA(NULL);
   wc.hIcon = NULL;
   wc.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
   wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
   wc.lpszMenuName = NULL;
   wc.lpszClassName = "SetInfoClass";
   wc.lpfnWndProc = info_wnd_proc;
   RegisterClassA(&wc);

   /* Create a main window */
   parent = CreateWindowExA(0, "SetInfoClass", NULL,
                           WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                           WS_MAXIMIZEBOX | WS_VISIBLE,
                           50, 50,
                           300, 300,
                           NULL, NULL, NULL, 0);
   ok(parent != NULL, "Creation of main window failed\n");

   parent2 = CreateWindowExA(0, "SetInfoClass", NULL,
                           WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX |
                           WS_MAXIMIZEBOX | WS_VISIBLE,
                           50, 50,
                           300, 300,
                           NULL, NULL, NULL, 0);
   ok(parent2 != NULL, "Creation of main window failed\n");

   /* Make it show */
   ShowWindow(parent2, SW_SHOWNORMAL);
   flush_events(100);

   /* Create Tooltip */
   hwndTip = CreateWindowExA(WS_EX_TOPMOST, TOOLTIPS_CLASSA,
                            NULL, TTS_NOPREFIX | TTS_ALWAYSTIP,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            parent, NULL, GetModuleHandleA(NULL), 0);
   ok(hwndTip != NULL, "Creation of tooltip window failed\n");

   hwndTip2 = CreateWindowExA(WS_EX_TOPMOST, TOOLTIPS_CLASSA,
                            NULL, TTS_NOPREFIX | TTS_ALWAYSTIP,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            parent, NULL, GetModuleHandleA(NULL), 0);
   ok(hwndTip2 != NULL, "Creation of tooltip window failed\n");


   /* Make it topmost, as per the MSDN */
   SetWindowPos(hwndTip, HWND_TOPMOST, 0, 0, 0, 0,
         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

   /* Create a tool */
   toolInfo.cbSize = TTTOOLINFOA_V1_SIZE;
   toolInfo.hwnd = parent;
   toolInfo.hinst = GetModuleHandleA(NULL);
   toolInfo.uFlags = TTF_SUBCLASS;
   toolInfo.uId = 0x1234ABCD;
   toolInfo.lpszText = (LPSTR)"This is a test tooltip";
   toolInfo.lParam = 0xdeadbeef;
   GetClientRect (parent, &toolInfo.rect);
   lResult = SendMessageA(hwndTip, TTM_ADDTOOLA, 0, (LPARAM)&toolInfo);
   ok(lResult, "Adding the tool to the tooltip failed\n");

   toolInfo.cbSize = TTTOOLINFOA_V1_SIZE;
   toolInfo.hwnd = parent2;
   toolInfo.hinst = GetModuleHandleA(NULL);
   toolInfo.uFlags = 0;
   toolInfo.uId = 0x1234ABCE;
   toolInfo.lpszText = (LPSTR)"This is a test tooltip";
   toolInfo.lParam = 0xdeadbeef;
   GetClientRect (parent, &toolInfo.rect);
   lResult = SendMessageA(hwndTip, TTM_ADDTOOLA, 0, (LPARAM)&toolInfo);
   ok(lResult, "Adding the tool to the tooltip failed\n");

   /* Try to Remove Subclass */
   toolInfo2.cbSize = TTTOOLINFOA_V1_SIZE;
   toolInfo2.hwnd = parent;
   toolInfo2.uId = 0x1234ABCD;
   lResult = SendMessageA(hwndTip, TTM_GETTOOLINFOA, 0, (LPARAM)&toolInfo2);
   ok(lResult, "GetToolInfo failed\n");
   ok(toolInfo2.uFlags & TTF_SUBCLASS || broken(is_v6 && !(toolInfo2.uFlags & TTF_SUBCLASS)) /* XP */,
       "uFlags does not have subclass\n");
   wndProc = (WNDPROC)GetWindowLongPtrA(parent, GWLP_WNDPROC);
   ok (wndProc != info_wnd_proc, "Window Proc is wrong\n");

   toolInfo2.uFlags &= ~TTF_SUBCLASS;
   SendMessageA(hwndTip, TTM_SETTOOLINFOA, 0, (LPARAM)&toolInfo2);
   lResult = SendMessageA(hwndTip, TTM_GETTOOLINFOA, 0, (LPARAM)&toolInfo2);
   ok(lResult, "GetToolInfo failed\n");
   ok(!(toolInfo2.uFlags & TTF_SUBCLASS), "uFlags has subclass\n");
   wndProc = (WNDPROC)GetWindowLongPtrA(parent, GWLP_WNDPROC);
   ok (wndProc != info_wnd_proc, "Window Proc is wrong\n");

   /* Try to Add Subclass */

   /* Make it topmost, as per the MSDN */
   SetWindowPos(hwndTip2, HWND_TOPMOST, 0, 0, 0, 0,
         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

   toolInfo2.cbSize = TTTOOLINFOA_V1_SIZE;
   toolInfo2.hwnd = parent2;
   toolInfo2.uId = 0x1234ABCE;
   lResult = SendMessageA(hwndTip, TTM_GETTOOLINFOA, 0, (LPARAM)&toolInfo2);
   ok(lResult, "GetToolInfo failed\n");
   ok(!(toolInfo2.uFlags & TTF_SUBCLASS), "uFlags has subclass\n");
   wndProc = (WNDPROC)GetWindowLongPtrA(parent2, GWLP_WNDPROC);
   ok (wndProc == info_wnd_proc, "Window Proc is wrong\n");

   toolInfo2.uFlags |= TTF_SUBCLASS;
   SendMessageA(hwndTip, TTM_SETTOOLINFOA, 0, (LPARAM)&toolInfo2);
   lResult = SendMessageA(hwndTip, TTM_GETTOOLINFOA, 0, (LPARAM)&toolInfo2);
   ok(lResult, "GetToolInfo failed\n");
   ok(toolInfo2.uFlags & TTF_SUBCLASS, "uFlags does not have subclass\n");
   wndProc = (WNDPROC)GetWindowLongPtrA(parent2, GWLP_WNDPROC);
   ok (wndProc == info_wnd_proc, "Window Proc is wrong\n");

   /* Clean up */
   DestroyWindow(hwndTip);
   DestroyWindow(hwndTip2);
   DestroyWindow(parent);
   DestroyWindow(parent2);
}

static void test_margin(void)
{
    RECT r, r1;
    HWND hwnd;
    DWORD ret;

    hwnd = CreateWindowExA(0, TOOLTIPS_CLASSA, NULL, 0,
                           10, 10, 300, 100,
                           NULL, NULL, NULL, 0);
    ok(hwnd != NULL, "failed to create tooltip wnd\n");

    ret = SendMessageA(hwnd, TTM_SETMARGIN, 0, 0);
    ok(!ret, "got %d\n", ret);

    SetRect(&r, -1, -1, 1, 1);
    ret = SendMessageA(hwnd, TTM_SETMARGIN, 0, (LPARAM)&r);
    ok(!ret, "got %d\n", ret);

    SetRectEmpty(&r1);
    ret = SendMessageA(hwnd, TTM_GETMARGIN, 0, (LPARAM)&r1);
    ok(!ret, "got %d\n", ret);
    ok(EqualRect(&r, &r1), "got %s, was %s\n", wine_dbgstr_rect(&r1), wine_dbgstr_rect(&r));

    ret = SendMessageA(hwnd, TTM_SETMARGIN, 0, 0);
    ok(!ret, "got %d\n", ret);

    SetRectEmpty(&r1);
    ret = SendMessageA(hwnd, TTM_GETMARGIN, 0, (LPARAM)&r1);
    ok(!ret, "got %d\n", ret);
    ok(EqualRect(&r, &r1), "got %s, was %s\n", wine_dbgstr_rect(&r1), wine_dbgstr_rect(&r));

    ret = SendMessageA(hwnd, TTM_GETMARGIN, 0, 0);
    ok(!ret, "got %d\n", ret);

    DestroyWindow(hwnd);
}

static void test_TTM_ADDTOOL(BOOL is_v6)
{
    static const WCHAR testW[] = {'T','e','s','t',0};
    TTTOOLINFOW tiW;
    TTTOOLINFOA ti;
    int ret, size;
    HWND hwnd, parent;
    UINT max_size;

    parent = CreateWindowExA(0, "Static", NULL, WS_POPUP, 0, 0, 0, 0, NULL, NULL, NULL, 0);
    ok(parent != NULL, "failed to create parent wnd\n");

    hwnd = CreateWindowExA(WS_EX_TOPMOST, TOOLTIPS_CLASSA, NULL, TTS_NOPREFIX | TTS_ALWAYSTIP,
        0, 0, 100, 100, NULL, NULL, GetModuleHandleA(NULL), 0);
    ok(hwnd != NULL, "Failed to create tooltip window.\n");

    for (size = 0; size <= TTTOOLINFOW_V3_SIZE + 1; size++)
    {
        ti.cbSize = size;
        ti.hwnd = NULL;
        ti.hinst = GetModuleHandleA(NULL);
        ti.uFlags = 0;
        ti.uId = 0x1234abce;
        ti.lpszText = (LPSTR)"Test";
        ti.lParam = 0xdeadbeef;
        GetClientRect(hwnd, &ti.rect);

        ret = SendMessageA(hwnd, TTM_ADDTOOLA, 0, (LPARAM)&ti);
        ok(ret, "Failed to add a tool, size %d.\n", size);

        ret = SendMessageA(hwnd, TTM_GETTOOLCOUNT, 0, 0);
        ok(ret == 1, "Unexpected tool count %d, size %d.\n", ret, size);

        ret = SendMessageA(hwnd, TTM_DELTOOLA, 0, (LPARAM)&ti);
        ok(!ret, "Unexpected ret value %d.\n", ret);

        ret = SendMessageA(hwnd, TTM_GETTOOLCOUNT, 0, 0);
        ok(ret == 0, "Unexpected tool count %d, size %d.\n", ret, size);
    }

    /* W variant checks cbSize. */
    max_size = is_v6 ? TTTOOLINFOW_V3_SIZE : TTTOOLINFOW_V2_SIZE;
    for (size = 0; size <= max_size + 1; size++)
    {
        tiW.cbSize = size;
        tiW.hwnd = NULL;
        tiW.hinst = GetModuleHandleA(NULL);
        tiW.uFlags = 0;
        tiW.uId = 0x1234abce;
        tiW.lpszText = (LPWSTR)testW;
        tiW.lParam = 0xdeadbeef;
        GetClientRect(hwnd, &tiW.rect);

        ret = SendMessageA(hwnd, TTM_ADDTOOLW, 0, (LPARAM)&tiW);
    todo_wine_if(!is_v6 && size > max_size)
        ok(size <= max_size ? ret : !ret, "%d: Unexpected ret value %d, size %d, max size %d.\n", is_v6, ret, size, max_size);
        if (ret)
        {
            ret = SendMessageA(hwnd, TTM_GETTOOLCOUNT, 0, 0);
            ok(ret == 1, "Unexpected tool count %d.\n", ret);

            ret = SendMessageA(hwnd, TTM_DELTOOLA, 0, (LPARAM)&tiW);
            ok(!ret, "Unexpected ret value %d.\n", ret);

            ret = SendMessageA(hwnd, TTM_GETTOOLCOUNT, 0, 0);
            ok(ret == 0, "Unexpected tool count %d.\n", ret);
        }
    }

    DestroyWindow(hwnd);
}

static const struct message ttn_show_parent_seq[] =
{
    { WM_NOTIFY, sent|id, 0, 0, TTN_SHOW },
    { 0 }
};

static void test_TTN_SHOW(void)
{
    HWND hwndTip, hwnd;
    TTTOOLINFOA ti;
    RECT rect;
    BOOL ret;

    hwnd = create_parent_window();
    ok(hwnd != NULL, "Failed to create parent window.\n");

    /* Put cursor outside the window */
    GetWindowRect(hwnd, &rect);
    SetCursorPos(rect.right + 200, 0);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    flush_events(100);

    /* Create tooltip */
    hwndTip = CreateWindowExA(WS_EX_TOPMOST, TOOLTIPS_CLASSA, NULL, TTS_ALWAYSTIP, 10, 10, 300, 300,
        hwnd, NULL, NULL, 0);
    ok(hwndTip != NULL, "Failed to create tooltip window.\n");

    SetWindowPos(hwndTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    ti.cbSize = sizeof(TTTOOLINFOA);
    ti.hwnd = hwnd;
    ti.hinst = GetModuleHandleA(NULL);
    ti.uFlags = TTF_SUBCLASS;
    ti.uId = 0x1234abcd;
    ti.lpszText = (LPSTR)"This is a test tooltip";
    ti.lParam = 0xdeadbeef;
    GetClientRect(hwnd, &ti.rect);
    ret = SendMessageA(hwndTip, TTM_ADDTOOLA, 0, (LPARAM)&ti);
    ok(ret, "Failed to add a tool.\n");

    /* Make tooltip appear quickly */
    SendMessageA(hwndTip, TTM_SETDELAYTIME, TTDT_INITIAL, MAKELPARAM(1, 0));

    flush_sequences(sequences, NUM_MSG_SEQUENCES);

    /* Put cursor inside window, tooltip will appear immediately */
    GetWindowRect(hwnd, &rect);
    SetCursorPos((rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2);
    flush_events(200);

    ok_sequence(sequences, PARENT_SEQ_INDEX, ttn_show_parent_seq, "TTN_SHOW parent seq", FALSE);

    DestroyWindow(hwndTip);
    DestroyWindow(hwnd);
}

START_TEST(tooltips)
{
    ULONG_PTR ctx_cookie;
    HANDLE hCtx;

    init_msg_sequences(sequences, NUM_MSG_SEQUENCES);

    LoadLibraryA("comctl32.dll");

    register_parent_wnd_class();

    test_create_tooltip(FALSE);
    test_customdraw();
    test_gettext();
    test_ttm_gettoolinfo();
    test_longtextA();
    test_longtextW();
    test_track();
    test_setinfo(FALSE);
    test_margin();
    test_TTM_ADDTOOL(FALSE);
    test_TTN_SHOW();

    if (!load_v6_module(&ctx_cookie, &hCtx))
        return;

    test_create_tooltip(TRUE);
    test_customdraw();
    test_longtextW();
    test_track();
    test_setinfo(TRUE);
    test_margin();
    test_TTM_ADDTOOL(TRUE);
    test_TTN_SHOW();

    unload_v6_module(ctx_cookie, hCtx);
}
