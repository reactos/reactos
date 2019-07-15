/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     debugging and analysis of message states
 * COPYRIGHT:   Copyright 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
#include <windows.h>
#include <shellapi.h>
#include <strsafe.h>
#include "undocuser.h"
#include "winxx.h"

static void MsgDumpPrintf(LPCTSTR fmt, ...)
{
    static char s_szText[1024];
    va_list va;
    va_start(va, fmt);
    StringCbVPrintfA(s_szText, sizeof(s_szText), fmt, va);
    trace("%s", s_szText);
    va_end(va);
}

static const char s_szName[] = "MessageStateAnalyzerGeneral";
static char s_prefix[16] = "";
#define MSGDUMP_TPRINTF MsgDumpPrintf
#define MSGDUMP_PREFIX s_prefix
#include "msgdump.h"

#define INTERVAL 500

typedef INT STAGE, ACTION;

static STAGE s_stage = 0;
UINT s_msg_stack[32] = { WM_NULL };
UINT s_prev_msg = WM_NULL;

typedef struct STATE
{
    /* counters */
    INT m_nWM_GETMINMAXINFO;
    INT m_nWM_NCCREATE;
    INT m_nWM_NCCALCSIZE;
    INT m_nWM_CREATE;
    INT m_nWM_SHOWWINDOW;
    INT m_nWM_WINDOWPOSCHANGING;
    INT m_nWM_ACTIVATEAPP;
    INT m_nWM_NCACTIVATE;
    INT m_nWM_ACTIVATE;
    INT m_nWM_IME_SETCONTEXT;
    INT m_nWM_IME_NOTIFY;
    INT m_nWM_SETFOCUS;
    INT m_nWM_NCPAINT;
    INT m_nWM_ERASEBKGND;
    INT m_nWM_WINDOWPOSCHANGED;
    INT m_nWM_DESTROY;
    INT m_nWM_NCDESTROY;
} STATE;

static STATE s_state = { 0 };
static BOOL s_bNewStage = FALSE;
static INT s_nLevel = 0;

static void NewStage(HWND hwnd)
{
    s_bNewStage = TRUE;
}

static void NewStageDoAction(HWND hwnd, ACTION action)
{
    s_bNewStage = TRUE;
    SetTimer(hwnd, action, INTERVAL, NULL);
}

static void DoAction(HWND hwnd, ACTION action)
{
    switch (action)
    {
    case 1:
        ShowWindow(hwnd, SW_SHOWNORMAL);
        break;
    case 2:
        DestroyWindow(hwnd);
        break;
    }
}

static void DoStage(STAGE stage, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    s_bNewStage = FALSE;
    switch (stage)
    {
        case 0:
            SetTimer(hwnd, 999, 10 * 1000, NULL);
            switch (uMsg)
            {
                case WM_GETMINMAXINFO:
                    ++s_state.m_nWM_GETMINMAXINFO;
                    ok_int(s_nLevel, 1);
                    ok_int(s_msg_stack[s_nLevel - 1], WM_NULL);
                    ok_int(s_prev_msg, WM_NULL);
                    ok_str(s_prefix, "P: ");
                    NewStage(hwnd);
                    break;
            }
            break;
        case 1:
            switch (uMsg)
            {
                case WM_NCCREATE:
                    ok_int(s_nLevel, 1);
                    ok_int(s_msg_stack[s_nLevel - 1], WM_NULL);
                    ok_int(s_prev_msg, WM_GETMINMAXINFO);
                    ok_str(s_prefix, "P: ");
                    ok_int(GetWindowRect(hwnd, &rc), TRUE);
                    ok_long(rc.right - rc.left, 300);
                    ok_long(rc.bottom - rc.top, 200);
                    ok_int(IsWindowVisible(hwnd), FALSE);
                    ++s_state.m_nWM_NCCREATE;
                    NewStage(hwnd);
                    break;
            }
            break;
        case 2:
            switch (uMsg)
            {
                case WM_NCCALCSIZE:
                    ++s_state.m_nWM_NCCALCSIZE;
                    ok_int(s_nLevel, 1);
                    ok_int(s_msg_stack[s_nLevel - 1], WM_NULL);
                    ok_int(s_prev_msg, WM_NCCREATE);
                    ok_str(s_prefix, "P: ");
                    NewStage(hwnd);
                    break;
            }
            break;
        case 3:
            switch (uMsg)
            {
                case WM_CREATE:
                    ++s_state.m_nWM_CREATE;
                    ok_int(s_nLevel, 1);
                    ok_int(s_msg_stack[s_nLevel - 1], WM_NULL);
                    ok_int(s_prev_msg, WM_NCCALCSIZE);
                    ok_str(s_prefix, "P: ");
                    NewStageDoAction(hwnd, 1);
                    break;
            }
            break;
        case 4:
            switch (uMsg)
            {
                case WM_SHOWWINDOW:
                    ++s_state.m_nWM_SHOWWINDOW;
                    ok_int(s_nLevel, 2);
                    ok_int(s_msg_stack[s_nLevel - 1], WM_TIMER);
                    ok_str(s_prefix, "P: ");
                    break;
                case WM_WINDOWPOSCHANGING:
                    ++s_state.m_nWM_WINDOWPOSCHANGING;
                    ok_int(s_nLevel, 2);
                    ok_int(s_msg_stack[s_nLevel - 1], WM_TIMER);
                    ok(s_prev_msg == WM_SHOWWINDOW || s_prev_msg == WM_WINDOWPOSCHANGING,
                       "s_prev_msg was %u\n.", s_prev_msg);
                    ok_str(s_prefix, "P: ");
                    break;
                case WM_ACTIVATEAPP:
                    ++s_state.m_nWM_ACTIVATEAPP;
                    ok_int(s_nLevel, 2);
                    ok_int(s_msg_stack[s_nLevel - 1], WM_TIMER);
                    ok_int(s_prev_msg, WM_WINDOWPOSCHANGING);
                    ok_str(s_prefix, "P: ");
                    break;
                case WM_NCACTIVATE:
                    ++s_state.m_nWM_NCACTIVATE;
                    ok_int(s_nLevel, 2);
                    ok_int(s_msg_stack[s_nLevel - 1], WM_TIMER);
                    ok_int(s_prev_msg, WM_ACTIVATEAPP);
                    ok_str(s_prefix, "P: ");
                    break;
                case WM_ACTIVATE:
                    ++s_state.m_nWM_ACTIVATE;
                    ok_int(s_nLevel, 2);
                    ok_int(s_msg_stack[s_nLevel - 1], WM_TIMER);
                    ok_int(s_prev_msg, WM_NCACTIVATE);
                    ok_str(s_prefix, "P: ");
                    NewStageDoAction(hwnd, 2);
                    break;
            }
            break;
        case 5:
            switch (uMsg)
            {
                case WM_IME_SETCONTEXT:
                    ++s_state.m_nWM_IME_SETCONTEXT;
                    ok_int(s_nLevel, 3);
                    ok_int(s_msg_stack[s_nLevel - 1], WM_ACTIVATE);
                    ok_int(s_prev_msg, WM_NCACTIVATE);
                    ok_str(s_prefix, "P: ");
                    break;
                case WM_IME_NOTIFY:
                    ++s_state.m_nWM_IME_NOTIFY;
                    ok_int(s_nLevel, 4);
                    ok_int(s_msg_stack[s_nLevel - 1], WM_IME_SETCONTEXT);
                    ok_int(s_prev_msg, WM_NCACTIVATE);
                    ok_str(s_prefix, "P: ");
                    break;
                case WM_SETFOCUS:
                    ++s_state.m_nWM_SETFOCUS;
                    ok_int(s_nLevel, 3);
                    ok_int(s_msg_stack[s_nLevel - 1], WM_ACTIVATE);
                    ok_int(s_prev_msg, WM_IME_SETCONTEXT);
                    ok_str(s_prefix, "P: ");
                    NewStage(hwnd);
                    break;
            }
        case 6:
            switch (uMsg)
            {
                case WM_NCPAINT:
                    ++s_state.m_nWM_NCPAINT;
                    ok_int(s_nLevel, 2);
                    ok_int(s_msg_stack[s_nLevel - 1], WM_TIMER);
                    ok_int(s_prev_msg, WM_ACTIVATE);
                    ok_str(s_prefix, "P: ");
                    break;
                case WM_ERASEBKGND:
                    ++s_state.m_nWM_ERASEBKGND;
                    ok_int(s_nLevel, 2);
                    ok_int(s_msg_stack[s_nLevel - 1], WM_TIMER);
                    ok_int(s_prev_msg, WM_NCPAINT);
                    ok_str(s_prefix, "P: ");
                    break;
                case WM_WINDOWPOSCHANGED:
                    ++s_state.m_nWM_WINDOWPOSCHANGED;
                    ok_int(s_nLevel, 2);
                    ok_int(s_msg_stack[s_nLevel - 1], WM_TIMER);
                    ok_int(s_prev_msg, WM_ERASEBKGND);
                    ok_str(s_prefix, "P: ");
                    NewStageDoAction(hwnd, 2);
                    break;
            }
            break;
        case 7:
            switch (uMsg)
            {
                case WM_DESTROY:
                    ++s_state.m_nWM_DESTROY;
                    ok_int(s_nLevel, 2);
                    ok_int(s_msg_stack[s_nLevel - 1], WM_TIMER);
                    ok_int(s_prev_msg, WM_IME_SETCONTEXT);
                    ok_str(s_prefix, "P: ");
                    NewStage(hwnd);
                    break;
            }
            break;
        case 8:
            switch (uMsg)
            {
                case WM_NCDESTROY:
                    ++s_state.m_nWM_NCDESTROY;
                    ok_int(s_nLevel, 2);
                    ok_int(s_msg_stack[s_nLevel - 1], WM_TIMER);
                    ok_int(s_prev_msg, WM_DESTROY);
                    ok_str(s_prefix, "P: ");
                    NewStage(hwnd);
                    break;
            }
            break;
#define LAST_STAGE 9
        case LAST_STAGE:
            break;
    }

    if (s_bNewStage)
    {
        ++s_stage;
        printf("Stage %d\n", s_stage);
    }
}

static LRESULT CALLBACK
InnerGeneralWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_TIMER:
            KillTimer(hwnd, 999);
            if (wParam == 999)
            {
                DestroyWindow(hwnd);
            }
            else
            {
                DoAction(hwnd, (ACTION)wParam);
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

static LRESULT CALLBACK
GeneralWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult;
    DWORD flags;
    int i;

    i = 0;
    flags = InSendMessageEx(NULL);
    if (flags & ISMEX_CALLBACK)
        s_prefix[i++] = 'C';
    if (flags & ISMEX_NOTIFY)
        s_prefix[i++] = 'N';
    if (flags & ISMEX_REPLIED)
        s_prefix[i++] = 'R';
    if (flags & ISMEX_SEND)
        s_prefix[i++] = 'S';
    if (i == 0)
        s_prefix[i++] = 'P';
    s_prefix[i++] = ':';
    s_prefix[i++] = ' ';
    s_prefix[i] = 0;

    MD_msgdump(hwnd, uMsg, wParam, lParam);
    ++s_nLevel;
    s_msg_stack[s_nLevel] = uMsg;
    {
        DoStage(s_stage, hwnd, uMsg, wParam, lParam);
        lResult = InnerGeneralWindowProc(hwnd, uMsg, wParam, lParam);
    }

    i = 0;
    s_prefix[i++] = 'R';
    s_prefix[i++] = ':';
    s_prefix[i++] = ' ';
    s_prefix[i] = 0;

    --s_nLevel;
    s_prev_msg = uMsg;
    MD_msgresult(hwnd, uMsg, wParam, lParam, lResult);

    return lResult;
}

static void Test_General(void)
{
    WNDCLASS wc;
    HWND hwnd;
    MSG msg;

    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = GeneralWindowProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName = s_szName;
    if (!RegisterClassA(&wc))
    {
        skip("RegisterClassW failed\n");
        return;
    }

    hwnd = CreateWindowA(s_szName, s_szName, WS_OVERLAPPEDWINDOW,
        0, 0, 300, 200, NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (!hwnd)
    {
        skip("CreateWindowW failed\n");
        return;
    }

    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    ok_int(s_stage, LAST_STAGE);

    ok_int(s_state.m_nWM_GETMINMAXINFO, 1);
    ok_int(s_state.m_nWM_NCCREATE, 1);
    ok_int(s_state.m_nWM_CREATE, 1);
    ok_int(s_state.m_nWM_SHOWWINDOW, 1);
    ok_int(s_state.m_nWM_WINDOWPOSCHANGING, 2);
    ok_int(s_state.m_nWM_ACTIVATEAPP, 1);
    ok_int(s_state.m_nWM_NCACTIVATE, 1);
    ok_int(s_state.m_nWM_ACTIVATE, 1);
    ok_int(s_state.m_nWM_IME_SETCONTEXT, 1);
    ok_int(s_state.m_nWM_IME_NOTIFY, 1);
    ok_int(s_state.m_nWM_SETFOCUS, 1);
    ok_int(s_state.m_nWM_NCPAINT, 1);
    ok_int(s_state.m_nWM_ERASEBKGND, 1);
    ok_int(s_state.m_nWM_WINDOWPOSCHANGED, 1);
    ok_int(s_state.m_nWM_DESTROY, 1);
    ok_int(s_state.m_nWM_NCDESTROY, 1);

    ok_int(IsWindow(hwnd), FALSE);

    ok_int(UnregisterClassA(s_szName, GetModuleHandleA(NULL)), TRUE);
}

START_TEST(MessageStateAnalyzer)
{
    Test_General();
}
