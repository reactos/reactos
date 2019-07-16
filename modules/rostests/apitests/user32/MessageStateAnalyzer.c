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

static void MsgDumpPrintf(LPCSTR fmt, ...)
{
    static char s_szText[1024];
    va_list va;
    va_start(va, fmt);
    StringCbVPrintfA(s_szText, sizeof(s_szText), fmt, va);
    trace("%s", s_szText);
    va_end(va);
}
#define MSGDUMP_TPRINTF MsgDumpPrintf
static char s_prefix[16] = "";
#define MSGDUMP_PREFIX s_prefix
#include "msgdump.h"    /* msgdump.h needs MSGDUMP_TPRINTF and MSGDUMP_PREFIX */

typedef INT STAGE, ACTION;

typedef struct STATE
{
    STAGE m_nStage;
    UINT m_nPrevMsg;
    UINT m_msgStack[32];
    INT m_nLevel;
    BOOL s_bNewStage;

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

static STATE s_state;

/* macros */
#define THE_STAGE       s_state.m_nStage
#define THE_LEVEL       s_state.m_nLevel
#define THE_STACK       s_state.m_msgStack
#define PREV_MSG        s_state.m_nPrevMsg
#define NEW_STAGE       s_state.s_bNewStage
#define COUNTER(WM_)    s_state.m_n##WM_
#define PARENT_MSG      THE_STACK[THE_LEVEL - 1]
#define TIMEOUT_TIMER   999
#define TOTAL_TIMEOUT   (10 * 1000)
#define WIDTH           300
#define HEIGHT          200

static __inline void NewStage(HWND hwnd)
{
    NEW_STAGE = TRUE;
}

static __inline void NewStageDoAction(HWND hwnd, ACTION nAction)
{
#define INTERVAL 500
    NEW_STAGE = TRUE;
    SetTimer(hwnd, nAction, INTERVAL, NULL);
#undef INTERVAL
}

static void General_Initialize(void)
{
    ZeroMemory(&s_state, sizeof(s_state));
}

static void General_DoAction(HWND hwnd, ACTION nAction)
{
    switch (nAction)
    {
        case 1:
            ShowWindow(hwnd, SW_SHOWNORMAL);
            break;
        case 2:
            DestroyWindow(hwnd);
            break;
        case TIMEOUT_TIMER:
            DestroyWindow(hwnd);
            break;
    }
}

static void
General_DoStage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    NEW_STAGE = FALSE;
    switch (THE_STAGE)
    {
        case 0:
            SetTimer(hwnd, TIMEOUT_TIMER, TOTAL_TIMEOUT, NULL);
            switch (uMsg)
            {
                case WM_GETMINMAXINFO:
                    ++COUNTER(WM_GETMINMAXINFO);
                    ok_int(THE_LEVEL, 1);
                    ok_int(PARENT_MSG, WM_NULL);
                    ok_int(PREV_MSG, WM_NULL);
                    ok_str(s_prefix, "P: ");
                    NewStage(hwnd);
                    break;
            }
            break;
        case 1:
            switch (uMsg)
            {
                case WM_NCCREATE:
                    ++COUNTER(WM_NCCREATE);
                    ok_int(THE_LEVEL, 1);
                    ok_int(PARENT_MSG, WM_NULL);
                    ok_int(PREV_MSG, WM_GETMINMAXINFO);
                    ok_str(s_prefix, "P: ");
                    ok_int(GetWindowRect(hwnd, &rc), TRUE);
                    ok_long(rc.right - rc.left, WIDTH);
                    ok_long(rc.bottom - rc.top, HEIGHT);
                    ok_int(IsWindowVisible(hwnd), FALSE);
                    NewStage(hwnd);
                    break;
            }
            break;
        case 2:
            switch (uMsg)
            {
                case WM_NCCALCSIZE:
                    ++COUNTER(WM_NCCALCSIZE);
                    ok_int(THE_LEVEL, 1);
                    ok_int(PARENT_MSG, WM_NULL);
                    ok_int(PREV_MSG, WM_NCCREATE);
                    ok_str(s_prefix, "P: ");
                    NewStage(hwnd);
                    break;
            }
            break;
        case 3:
            switch (uMsg)
            {
                case WM_CREATE:
                    ++COUNTER(WM_CREATE);
                    ok_int(THE_LEVEL, 1);
                    ok_int(PARENT_MSG, WM_NULL);
                    ok_int(PREV_MSG, WM_NCCALCSIZE);
                    ok_str(s_prefix, "P: ");
                    NewStageDoAction(hwnd, 1);
                    break;
            }
            break;
        case 4:
            switch (uMsg)
            {
                case WM_SHOWWINDOW:
                    ++COUNTER(WM_SHOWWINDOW);
                    ok_int(THE_LEVEL, 2);
                    ok_int(PARENT_MSG, WM_TIMER);
                    ok_str(s_prefix, "P: ");
                    break;
                case WM_WINDOWPOSCHANGING:
                    ++COUNTER(WM_WINDOWPOSCHANGING);
                    ok_int(THE_LEVEL, 2);
                    ok_int(PARENT_MSG, WM_TIMER);
                    ok(PREV_MSG == WM_SHOWWINDOW || PREV_MSG == WM_WINDOWPOSCHANGING,
                       "PREV_MSG was %u\n.", PREV_MSG);
                    ok_str(s_prefix, "P: ");
                    break;
                case WM_ACTIVATEAPP:
                    ++COUNTER(WM_ACTIVATEAPP);
                    ok_int(THE_LEVEL, 2);
                    ok_int(PARENT_MSG, WM_TIMER);
                    ok_int(PREV_MSG, WM_WINDOWPOSCHANGING);
                    ok_str(s_prefix, "P: ");
                    break;
                case WM_NCACTIVATE:
                    ++COUNTER(WM_NCACTIVATE);
                    ok_int(THE_LEVEL, 2);
                    ok_int(PARENT_MSG, WM_TIMER);
                    ok_int(PREV_MSG, WM_ACTIVATEAPP);
                    ok_str(s_prefix, "P: ");
                    break;
                case WM_ACTIVATE:
                    ++COUNTER(WM_ACTIVATE);
                    ok_int(THE_LEVEL, 2);
                    ok_int(PARENT_MSG, WM_TIMER);
                    ok_int(PREV_MSG, WM_NCACTIVATE);
                    ok_str(s_prefix, "P: ");
                    NewStageDoAction(hwnd, 2);
                    break;
            }
            break;
        case 5:
            switch (uMsg)
            {
                case WM_IME_SETCONTEXT:
                    ++COUNTER(WM_IME_SETCONTEXT);
                    ok_int(THE_LEVEL, 3);
                    ok_int(PARENT_MSG, WM_ACTIVATE);
                    ok_int(PREV_MSG, WM_NCACTIVATE);
                    ok_str(s_prefix, "P: ");
                    break;
                case WM_IME_NOTIFY:
                    ++COUNTER(WM_IME_NOTIFY);
                    ok_int(THE_LEVEL, 4);
                    ok_int(PARENT_MSG, WM_IME_SETCONTEXT);
                    ok_int(PREV_MSG, WM_NCACTIVATE);
                    ok_str(s_prefix, "P: ");
                    break;
                case WM_SETFOCUS:
                    ++COUNTER(WM_SETFOCUS);
                    ok_int(THE_LEVEL, 3);
                    ok_int(PARENT_MSG, WM_ACTIVATE);
                    ok_int(PREV_MSG, WM_IME_SETCONTEXT);
                    ok_str(s_prefix, "P: ");
                    NewStage(hwnd);
                    break;
            }
        case 6:
            switch (uMsg)
            {
                case WM_NCPAINT:
                    ++COUNTER(WM_NCPAINT);
                    ok_int(THE_LEVEL, 2);
                    ok_int(PARENT_MSG, WM_TIMER);
                    ok_int(PREV_MSG, WM_ACTIVATE);
                    ok_str(s_prefix, "P: ");
                    break;
                case WM_ERASEBKGND:
                    ++COUNTER(WM_ERASEBKGND);
                    ok_int(THE_LEVEL, 2);
                    ok_int(PARENT_MSG, WM_TIMER);
                    ok_int(PREV_MSG, WM_NCPAINT);
                    ok_str(s_prefix, "P: ");
                    break;
                case WM_WINDOWPOSCHANGED:
                    ++COUNTER(WM_WINDOWPOSCHANGED);
                    ok_int(THE_LEVEL, 2);
                    ok_int(PARENT_MSG, WM_TIMER);
                    ok_int(PREV_MSG, WM_ERASEBKGND);
                    ok_str(s_prefix, "P: ");
                    NewStageDoAction(hwnd, 2);
                    break;
            }
            break;
        case 7:
            switch (uMsg)
            {
                case WM_DESTROY:
                    ++COUNTER(WM_DESTROY);
                    ok_int(THE_LEVEL, 2);
                    ok_int(PARENT_MSG, WM_TIMER);
                    ok_int(PREV_MSG, WM_IME_SETCONTEXT);
                    ok_str(s_prefix, "P: ");
                    NewStage(hwnd);
                    break;
            }
            break;
        case 8:
            switch (uMsg)
            {
                case WM_NCDESTROY:
                    ++COUNTER(WM_NCDESTROY);
                    ok_int(THE_LEVEL, 2);
                    ok_int(PARENT_MSG, WM_TIMER);
                    ok_int(PREV_MSG, WM_DESTROY);
                    ok_str(s_prefix, "P: ");
                    NewStage(hwnd);
                    break;
            }
            break;
#define LAST_STAGE 9
        case LAST_STAGE:
            break;
    }

    if (NEW_STAGE)
    {
        ++THE_STAGE;
        trace("Stage %d\n", THE_STAGE);
    }
}

static LRESULT CALLBACK
General_InnerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_TIMER:
            KillTimer(hwnd, (UINT)wParam);
            General_DoAction(hwnd, (ACTION)wParam);
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
General_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult;
    DWORD Flags = InSendMessageEx(NULL);
    INT i = 0;

    /* build s_prefix */
    if (Flags & ISMEX_CALLBACK)
        s_prefix[i++] = 'C';
    if (Flags & ISMEX_NOTIFY)
        s_prefix[i++] = 'N';
    if (Flags & ISMEX_REPLIED)
        s_prefix[i++] = 'R';
    if (Flags & ISMEX_SEND)
        s_prefix[i++] = 'S';
    if (i == 0)
        s_prefix[i++] = 'P';
    s_prefix[i++] = ':';
    s_prefix[i++] = ' ';
    s_prefix[i] = 0;

    /* message dump */
    MD_msgdump(hwnd, uMsg, wParam, lParam);

    ++THE_LEVEL;
    THE_STACK[THE_LEVEL] = uMsg;
    {
        /* do inner task */
        General_DoStage(hwnd, uMsg, wParam, lParam);
        lResult = General_InnerWindowProc(hwnd, uMsg, wParam, lParam);
    }
    --THE_LEVEL;
    PREV_MSG = uMsg;

    /* message return */
    StringCbCopyA(s_prefix, sizeof(s_prefix), "R: ");
    MD_msgresult(hwnd, uMsg, wParam, lParam, lResult);
    return lResult;
}

static void General_Finish(void)
{
    ok_int(THE_STAGE, LAST_STAGE);

    ok_int(COUNTER(WM_GETMINMAXINFO), 1);
    ok_int(COUNTER(WM_NCCREATE), 1);
    ok_int(COUNTER(WM_CREATE), 1);
    ok_int(COUNTER(WM_SHOWWINDOW), 1);
    ok_int(COUNTER(WM_WINDOWPOSCHANGING), 2);
    ok_int(COUNTER(WM_ACTIVATEAPP), 1);
    ok_int(COUNTER(WM_NCACTIVATE), 1);
    ok_int(COUNTER(WM_ACTIVATE), 1);
    ok_int(COUNTER(WM_IME_SETCONTEXT), 1);
    ok_int(COUNTER(WM_IME_NOTIFY), 1);
    ok_int(COUNTER(WM_SETFOCUS), 1);
    ok_int(COUNTER(WM_NCPAINT), 1);
    ok_int(COUNTER(WM_ERASEBKGND), 1);
    ok_int(COUNTER(WM_WINDOWPOSCHANGED), 1);
    ok_int(COUNTER(WM_DESTROY), 1);
    ok_int(COUNTER(WM_NCDESTROY), 1);

    if (THE_STAGE != LAST_STAGE)
    {
        skip("Some stage(s) skipped.\n");
    }
}

static void General_DoTest(void)
{
    WNDCLASSA wc;
    HWND hwnd;
    MSG msg;
    static const char s_szName[] = "MessageStateAnalyzerGeneral";

    General_Initialize();

    /* register window class */
    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = General_WindowProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName = s_szName;
    if (!RegisterClassA(&wc))
    {
        skip("RegisterClassW failed.\n");
        return;
    }

    /* create a window */
    hwnd = CreateWindowA(s_szName, s_szName, WS_OVERLAPPEDWINDOW,
                         0, 0, WIDTH, HEIGHT, NULL, NULL,
                         GetModuleHandleW(NULL), NULL);
    if (!hwnd)
    {
        skip("CreateWindowW failed.\n");
        return;
    }

    /* message loop */
    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    ok_int(IsWindow(hwnd), FALSE);
    ok_int(UnregisterClassA(s_szName, GetModuleHandleA(NULL)), TRUE);

    General_Finish();
}

START_TEST(MessageStateAnalyzer)
{
    General_DoTest();
}
