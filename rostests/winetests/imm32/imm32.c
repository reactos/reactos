/*
 * Unit tests for imm32
 *
 * Copyright (c) 2008 Michael Jung
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

#include <stdio.h>

#include "wine/test.h"
#include "winuser.h"
#include "imm.h"

#define NUMELEMS(array) (sizeof((array))/sizeof((array)[0]))

/*
 * msgspy - record and analyse message traces sent to a certain window
 */
static struct _msg_spy {
    HWND         hwnd;
    HHOOK        get_msg_hook;
    HHOOK        call_wnd_proc_hook;
    CWPSTRUCT    msgs[32];
    unsigned int i_msg;
} msg_spy;

static LRESULT CALLBACK get_msg_filter(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (HC_ACTION == nCode) {
        MSG *msg = (MSG*)lParam;

        if ((msg->hwnd == msg_spy.hwnd) &&
            (msg_spy.i_msg < NUMELEMS(msg_spy.msgs)))
        {
            msg_spy.msgs[msg_spy.i_msg].hwnd    = msg->hwnd;
            msg_spy.msgs[msg_spy.i_msg].message = msg->message;
            msg_spy.msgs[msg_spy.i_msg].wParam  = msg->wParam;
            msg_spy.msgs[msg_spy.i_msg].lParam  = msg->lParam;
            msg_spy.i_msg++;
        }
    }

    return CallNextHookEx(msg_spy.get_msg_hook, nCode, wParam, lParam);
}

static LRESULT CALLBACK call_wnd_proc_filter(int nCode, WPARAM wParam,
                                             LPARAM lParam)
{
    if (HC_ACTION == nCode) {
        CWPSTRUCT *cwp = (CWPSTRUCT*)lParam;

        if ((cwp->hwnd == msg_spy.hwnd) &&
            (msg_spy.i_msg < NUMELEMS(msg_spy.msgs)))
        {
            memcpy(&msg_spy.msgs[msg_spy.i_msg], cwp, sizeof(msg_spy.msgs[0]));
            msg_spy.i_msg++;
        }
    }

    return CallNextHookEx(msg_spy.call_wnd_proc_hook, nCode, wParam, lParam);
}

static void msg_spy_pump_msg_queue(void) {
    MSG msg;

    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return;
}

static void msg_spy_flush_msgs(void) {
    msg_spy_pump_msg_queue();
    msg_spy.i_msg = 0;
}

static CWPSTRUCT* msg_spy_find_msg(UINT message) {
    int i;

    msg_spy_pump_msg_queue();

    if (msg_spy.i_msg >= NUMELEMS(msg_spy.msgs))
        fprintf(stdout, "%s:%d: msg_spy: message buffer overflow!\n",
                __FILE__, __LINE__);

    for (i = 0; i < msg_spy.i_msg; i++)
        if (msg_spy.msgs[i].message == message)
            return &msg_spy.msgs[i];

    return NULL;
}

static void msg_spy_init(HWND hwnd) {
    msg_spy.hwnd = hwnd;
    msg_spy.get_msg_hook =
            SetWindowsHookEx(WH_GETMESSAGE, get_msg_filter, GetModuleHandle(0),
                             GetCurrentThreadId());
    msg_spy.call_wnd_proc_hook =
            SetWindowsHookEx(WH_CALLWNDPROC, call_wnd_proc_filter,
                             GetModuleHandle(0), GetCurrentThreadId());
    msg_spy.i_msg = 0;

    msg_spy_flush_msgs();
}

static void msg_spy_cleanup() {
    if (msg_spy.get_msg_hook)
        UnhookWindowsHookEx(msg_spy.get_msg_hook);
    if (msg_spy.call_wnd_proc_hook)
        UnhookWindowsHookEx(msg_spy.call_wnd_proc_hook);
    memset(&msg_spy, 0, sizeof(msg_spy));
}

/*
 * imm32 test cases - Issue some IMM commands on a dummy window and analyse the
 * messages being sent to this window in response.
 */
static const char wndcls[] = "winetest_imm32_wndcls";
static HWND hwnd;

static int init(void) {
    WNDCLASSEX wc;
    HIMC imc;

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = DefWindowProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = GetModuleHandle(0);
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = wndcls;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExA(&wc))
        return 0;

    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, wndcls, "Wine imm32.dll test",
                          WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                          240, 120, NULL, NULL, GetModuleHandle(0), NULL);
    if (!hwnd)
        return 0;

    imc = ImmGetContext(hwnd);
    if (!imc)
    {
        win_skip("IME support not implemented\n");
        return 0;
    }
    ImmReleaseContext(hwnd, imc);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    msg_spy_init(hwnd);

    return 1;
}

static void cleanup(void) {
    msg_spy_cleanup();
    if (hwnd)
        DestroyWindow(hwnd);
    UnregisterClass(wndcls, GetModuleHandle(0));
}

static void test_ImmNotifyIME(void) {
    static const char string[] = "wine";
    char resstr[16] = "";
    HIMC imc;
    BOOL ret;

    imc = ImmGetContext(hwnd);
    msg_spy_flush_msgs();

    todo_wine
    {
        ok(!ImmNotifyIME(imc, NI_COMPOSITIONSTR, CPS_CANCEL, 0), "Canceling an "
           "empty composition string should fail.\n");
    }
    ok(!msg_spy_find_msg(WM_IME_COMPOSITION), "Windows does not post "
       "WM_IME_COMPOSITION in response to NI_COMPOSITIONSTR / CPS_CANCEL, if "
       "the composition string being canceled is empty.\n");

    ImmSetCompositionString(imc, SCS_SETSTR, string, sizeof(string), NULL, 0);
    msg_spy_flush_msgs();

    ImmNotifyIME(imc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    ok(!msg_spy_find_msg(WM_IME_COMPOSITION), "Windows does not post "
       "WM_IME_COMPOSITION in response to NI_COMPOSITIONSTR / CPS_CANCEL, if "
       "the composition string being canceled is non empty.\n");

    /* behavior differs between win9x and NT */
    ret = ImmGetCompositionString(imc, GCS_COMPSTR, resstr, sizeof(resstr));
    ok(ret || !ret, "You'll never read this.\n");

    msg_spy_flush_msgs();

    todo_wine
    {
        ok(!ImmNotifyIME(imc, NI_COMPOSITIONSTR, CPS_CANCEL, 0), "Canceling an "
           "empty composition string should fail.\n");
    }
    ok(!msg_spy_find_msg(WM_IME_COMPOSITION), "Windows does not post "
       "WM_IME_COMPOSITION in response to NI_COMPOSITIONSTR / CPS_CANCEL, if "
       "the composition string being canceled is empty.\n");

    msg_spy_flush_msgs();
    ImmReleaseContext(hwnd, imc);
}

static void test_ImmGetCompositionString(void)
{
    HIMC imc;
    static const WCHAR string[] = {'w','i','n','e',0x65e5,0x672c,0x8a9e};
    char cstring[20];
    WCHAR wstring[20];
    DWORD len;
    DWORD alen,wlen;

    imc = ImmGetContext(hwnd);
    ImmSetCompositionStringW(imc, SCS_SETSTR, string, sizeof(string), NULL,0);
    alen = ImmGetCompositionStringA(imc, GCS_COMPSTR, cstring, 20);
    wlen = ImmGetCompositionStringW(imc, GCS_COMPSTR, wstring, 20);
    /* windows machines without any IME installed just return 0 above */
    if( alen && wlen)
    {
        len = ImmGetCompositionStringW(imc, GCS_COMPATTR, NULL, 0);
        ok(len*sizeof(WCHAR)==wlen,"GCS_COMPATTR(W) not returning correct count\n");
        len = ImmGetCompositionStringA(imc, GCS_COMPATTR, NULL, 0);
        ok(len==alen,"GCS_COMPATTR(A) not returning correct count\n");
    }
    ImmReleaseContext(hwnd, imc);
}

static void test_ImmSetCompositionString(void)
{
    HIMC imc;
    BOOL ret;

    SetLastError(0xdeadbeef);
    imc = ImmGetContext(hwnd);
    ok(imc != 0, "ImmGetContext() failed. Last error: %u\n", GetLastError());
    if (!imc)
        return;

    ret = ImmSetCompositionStringW(imc, SCS_SETSTR, NULL, 0, NULL, 0);
    todo_wine
    ok(!ret, "ImmSetCompositionStringW() succeeded.\n");

    ret = ImmSetCompositionStringW(imc, SCS_SETSTR | SCS_CHANGEATTR,
        NULL, 0, NULL, 0);
    todo_wine
    ok(!ret, "ImmSetCompositionStringW() succeeded.\n");

    ret = ImmSetCompositionStringW(imc, SCS_SETSTR | SCS_CHANGECLAUSE,
        NULL, 0, NULL, 0);
    todo_wine
    ok(!ret, "ImmSetCompositionStringW() succeeded.\n");

    ret = ImmSetCompositionStringW(imc, SCS_CHANGEATTR | SCS_CHANGECLAUSE,
        NULL, 0, NULL, 0);
    todo_wine
    ok(!ret, "ImmSetCompositionStringW() succeeded.\n");

    ImmReleaseContext(hwnd, imc);
}

static void test_ImmIME(void)
{
    HIMC imc;

    imc = ImmGetContext(hwnd);
    if (imc)
    {
        BOOL rc;
        rc = ImmConfigureIMEA(imc, NULL, IME_CONFIG_REGISTERWORD, NULL);
        ok (rc == 0, "ImmConfigureIMEA did not fail\n");
        rc = ImmConfigureIMEW(imc, NULL, IME_CONFIG_REGISTERWORD, NULL);
        ok (rc == 0, "ImmConfigureIMEW did not fail\n");
    }
    ImmReleaseContext(hwnd,imc);
}

START_TEST(imm32) {
    if (init())
    {
        test_ImmNotifyIME();
        test_ImmGetCompositionString();
        test_ImmSetCompositionString();
        test_ImmIME();
    }
    cleanup();
}
