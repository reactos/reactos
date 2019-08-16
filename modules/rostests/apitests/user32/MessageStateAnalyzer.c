/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     debugging and analysis of message states
 * COPYRIGHT:   Copyright 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include "undocuser.h"
#include "winxx.h"
#include <strsafe.h>

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

typedef enum STAGE_TYPE
{
    STAGE_TYPE_SEQUENCE,
    STAGE_TYPE_COUNTING
} STAGE_TYPE;

typedef struct STAGE
{
    INT nLine;
    UINT uParentMsg;
    INT nLevel;
    STAGE_TYPE nType;
    INT iFirstAction;
    INT nCount;
    UINT uMessages[10];
    INT iActions[10];
    INT nCounters[10];
} STAGE;

/* variables */
static INT s_iStage;
static INT s_iStep;
static INT s_nLevel;
static BOOL s_bNextStage;
static INT s_nCounters[10];
static UINT s_msgStack[32];
static const STAGE *s_pStages;
static INT s_cStages;

/* macros */
#define TIMEOUT_TIMER   999
#define TOTAL_TIMEOUT   (5 * 1000)
#define WIDTH           300
#define HEIGHT          200
#define PARENT_MSG      s_msgStack[s_nLevel - 1]

static void DoInitialize(const STAGE *pStages, INT cStages)
{
    s_iStage = s_iStep = s_nLevel = 0;
    s_bNextStage = FALSE;
    ZeroMemory(s_nCounters, sizeof(s_nCounters));
    ZeroMemory(s_msgStack, sizeof(s_msgStack));
    s_pStages = pStages;
    s_cStages = cStages;
}

static void DoFinish(void)
{
    ok_int(s_iStage, s_cStages);
    if (s_iStage != s_cStages)
    {
        skip("Some stage(s) skipped (Step: %d)\n", s_iStep);
    }
}

typedef enum ACTION
{
    ACTION_ZERO = 0,
    ACTION_FIRSTMINMAX,
    ACTION_NCCREATE,
    ACTION_SHOW,
    ACTION_IME_SETCONTEXT_OPEN,
    ACTION_IME_NOTIFY_OPEN,
    ACTION_DESTROY,
    ACTION_IME_SETCONTEXT_CLOSE,
    ACTION_IME_NOTIFY_CLOSE,
    ACTION_HIDE,
    ACTION_DEACTIVATE,
    ACTION_ACTIVATE
} ACTION;

static void DoAction(HWND hwnd, INT iAction, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    switch (iAction)
    {
        case ACTION_ZERO:
            /* does nothing */
            break;
        case ACTION_FIRSTMINMAX:
            GetWindowRect(hwnd, &rc);
            ok_long(rc.right - rc.left, 0);
            ok_long(rc.bottom - rc.top, 0);
            ok_int(IsWindowVisible(hwnd), FALSE);
            break;
        case ACTION_NCCREATE:
            GetWindowRect(hwnd, &rc);
            ok_long(rc.right - rc.left, WIDTH);
            ok_long(rc.bottom - rc.top, HEIGHT);
            ok_int(IsWindowVisible(hwnd), FALSE);
            break;
        case ACTION_SHOW:
            ShowWindow(hwnd, SW_SHOWNORMAL);
            break;
        case ACTION_IME_SETCONTEXT_OPEN:
            ok(wParam == 1, "Step %d: wParam was %p\n", s_iStep, (void *)wParam);
            ok(lParam == 0xC000000F, "Step %d: lParam was %p\n", s_iStep, (void *)lParam);
            break;
        case ACTION_IME_NOTIFY_OPEN:
            ok(wParam == 2, "Step %d: wParam was %p\n", s_iStep, (void *)wParam);
            ok(lParam == 0, "Step %d: lParam was %p\n", s_iStep, (void *)lParam);
            break;
        case ACTION_DESTROY:
            DestroyWindow(hwnd);
            break;
        case ACTION_IME_SETCONTEXT_CLOSE:
            ok(wParam == 0, "Step %d: wParam was %p\n", s_iStep, (void *)wParam);
            ok(lParam == 0xC000000F, "Step %d: lParam was %p\n", s_iStep, (void *)lParam);
            break;
        case ACTION_IME_NOTIFY_CLOSE:
            ok(wParam == 1, "Step %d: wParam was %p\n", s_iStep, (void *)wParam);
            ok(lParam == 0, "Step %d: lParam was %p\n", s_iStep, (void *)lParam);
            break;
        case ACTION_HIDE:
            ShowWindow(hwnd, SW_HIDE);
            break;
        case ACTION_DEACTIVATE:
            SetForegroundWindow(GetDesktopWindow());
            break;
        case ACTION_ACTIVATE:
            SetForegroundWindow(hwnd);
            break;
    }
}

static void NextStage(HWND hwnd)
{
    INT i, iAction;
    const STAGE *pStage = &s_pStages[s_iStage];

    if (pStage->nType == STAGE_TYPE_COUNTING)
    {
        /* check counters */
        for (i = 0; i < pStage->nCount; ++i)
        {
            if (pStage->nCounters[i] > 0)
            {
                ok(pStage->nCounters[i] == s_nCounters[i],
                   "Line %d: s_nCounters[%d] expected %d but %d.\n",
                   pStage->nLine, i, pStage->nCounters[i], s_nCounters[i]);
            }
        }
    }

    /* go to next stage */
    ++s_iStage;
    if (s_iStage >= s_cStages)
    {
        DestroyWindow(hwnd);
        return;
    }
    trace("Stage %d (Line %d)\n", s_iStage, s_pStages[s_iStage].nLine);

    s_iStep = 0;
    ZeroMemory(s_nCounters, sizeof(s_nCounters));

    iAction = s_pStages[s_iStage].iFirstAction;
    if (iAction)
        PostMessage(hwnd, WM_COMMAND, iAction, 0);
}

static void DoStage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT i, iAction;
    const STAGE *pStage;
    s_bNextStage = FALSE;

    if (s_iStage >= s_cStages)
        return;

    pStage = &s_pStages[s_iStage];
    switch (pStage->nType)
    {
        case STAGE_TYPE_SEQUENCE:
            if (pStage->uMessages[s_iStep] == uMsg)
            {
                ok_int(1, 1);
                ok(s_nLevel == pStage->nLevel,
                   "Line %d, Step %d: Level expected %d but %d.\n",
                   pStage->nLine, s_iStep, pStage->nLevel, s_nLevel);
                ok(PARENT_MSG == pStage->uParentMsg,
                   "Line %d, Step %d: PARENT_MSG expected %u but %u.\n",
                   pStage->nLine, s_iStep, pStage->uParentMsg, PARENT_MSG);

                iAction = pStage->iActions[s_iStep];
                if (iAction)
                    DoAction(hwnd, iAction, wParam, lParam);

                ++s_iStep;
                if (s_iStep >= pStage->nCount)
                    s_bNextStage = TRUE;
            }
            break;
        case STAGE_TYPE_COUNTING:
            for (i = 0; i < pStage->nCount; ++i)
            {
                if (pStage->uMessages[i] == uMsg)
                {
                    ok_int(1, 1);
                    ok(s_nLevel == pStage->nLevel,
                       "Line %d: Level expected %d but %d.\n",
                       pStage->nLine, pStage->nLevel, s_nLevel);
                    ok(PARENT_MSG == pStage->uParentMsg,
                       "Line %d: PARENT_MSG expected %u but %u.\n",
                       pStage->nLine, pStage->uParentMsg, PARENT_MSG);

                    iAction = pStage->iActions[i];
                    if (iAction)
                        DoAction(hwnd, iAction, wParam, lParam);

                    ++s_nCounters[i];

                    if (i == pStage->nCount - 1)
                        s_bNextStage = TRUE;
                    break;
                }
            }
            break;
    }

    if (s_bNextStage)
    {
        NextStage(hwnd);
    }
}

static LRESULT CALLBACK
InnerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_COMMAND:
            DoAction(hwnd, LOWORD(wParam), 0, 0);
            break;
        case WM_TIMER:
            KillTimer(hwnd, (UINT)wParam);
            if (wParam == TIMEOUT_TIMER)
                DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_NCCREATE:
            SetTimer(hwnd, TIMEOUT_TIMER, TOTAL_TIMEOUT, NULL);
            /* FALL THROUGH */
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

static void DoBuildPrefix(void)
{
    DWORD Flags = InSendMessageEx(NULL);
    INT i = 0;

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
}

static const STAGE s_GeneralStages[] =
{
    /* Stage 0 */
    {
        __LINE__, WM_NULL, 1, STAGE_TYPE_SEQUENCE, 0,
        4,
        { WM_GETMINMAXINFO, WM_NCCREATE, WM_NCCALCSIZE, WM_CREATE },
        { ACTION_FIRSTMINMAX, ACTION_NCCREATE, 0, 0 },
    },
    /* Stage 1 */
    {
        __LINE__, WM_COMMAND, 2, STAGE_TYPE_SEQUENCE, ACTION_SHOW,
        6,
        { WM_SHOWWINDOW, WM_WINDOWPOSCHANGING, WM_WINDOWPOSCHANGING,
          WM_ACTIVATEAPP, WM_NCACTIVATE, WM_ACTIVATE },
    },
    {
        __LINE__, WM_COMMAND, 2, STAGE_TYPE_SEQUENCE, ACTION_DESTROY,
        6,
        { WM_WINDOWPOSCHANGING, WM_WINDOWPOSCHANGED, WM_NCACTIVATE,
          WM_ACTIVATE, WM_ACTIVATEAPP, WM_KILLFOCUS },
    },
    {
        __LINE__, WM_COMMAND, 2, STAGE_TYPE_SEQUENCE, 0,
        2,
        { WM_DESTROY, WM_NCDESTROY },
    },
};

static LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult;

    /* build s_prefix */
    DoBuildPrefix();

    /* message dump */
    MD_msgdump(hwnd, uMsg, wParam, lParam);

    ++s_nLevel;
    s_msgStack[s_nLevel] = uMsg;
    {
        /* do inner task */
        DoStage(hwnd, uMsg, wParam, lParam);
        lResult = InnerWindowProc(hwnd, uMsg, wParam, lParam);
    }
    --s_nLevel;

    /* message return */
    StringCbCopyA(s_prefix, sizeof(s_prefix), "R: ");
    MD_msgresult(hwnd, uMsg, wParam, lParam, lResult);
    return lResult;
}

static void General_DoTest(void)
{
    WNDCLASSA wc;
    HWND hwnd;
    MSG msg;
    static const char s_szName[] = "MessageStateAnalyzerGeneral";

    trace("General_DoTest\n");
    DoInitialize(s_GeneralStages, ARRAYSIZE(s_GeneralStages));

    /* register window class */
    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
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

    ok_int(UnregisterClassA(s_szName, GetModuleHandleA(NULL)), TRUE);

    DoFinish();
}

static const STAGE s_IMEStages[] =
{
    /* Stage 0 */
    {
        __LINE__, WM_NULL, 1, STAGE_TYPE_SEQUENCE, 0,
        4,
        { WM_GETMINMAXINFO, WM_NCCREATE, WM_NCCALCSIZE, WM_CREATE },
        { ACTION_FIRSTMINMAX, ACTION_NCCREATE, 0, 0 },
    },
    /* Stage 1 */
    // show
    {
        __LINE__, WM_COMMAND, 2, STAGE_TYPE_SEQUENCE, ACTION_SHOW,
        6,
        { WM_SHOWWINDOW, WM_WINDOWPOSCHANGING, WM_WINDOWPOSCHANGING,
          WM_ACTIVATEAPP, WM_NCACTIVATE, WM_ACTIVATE },
    },
    {
        __LINE__, WM_ACTIVATE, 3, STAGE_TYPE_SEQUENCE, 0,
        1,
        { WM_IME_SETCONTEXT },
        { ACTION_IME_SETCONTEXT_OPEN },
    },
    {
        __LINE__, WM_IME_SETCONTEXT, 4, STAGE_TYPE_SEQUENCE, 0,
        1,
        { WM_IME_NOTIFY },
        { ACTION_IME_NOTIFY_OPEN },
    },
    // hide
    {
        __LINE__, WM_COMMAND, 2, STAGE_TYPE_SEQUENCE, ACTION_HIDE,
        8,
        { WM_SHOWWINDOW, WM_WINDOWPOSCHANGING, WM_WINDOWPOSCHANGED,
          WM_NCACTIVATE, WM_ACTIVATE, WM_ACTIVATEAPP, WM_KILLFOCUS,
          WM_IME_SETCONTEXT },
        { 0, 0, 0, 0, 0, 0, 0, ACTION_IME_SETCONTEXT_CLOSE }
    },
    {
        __LINE__, WM_IME_SETCONTEXT, 3, STAGE_TYPE_SEQUENCE, 0,
        1,
        { WM_IME_NOTIFY },
        { ACTION_IME_NOTIFY_CLOSE }
    },
    // show again
    {
        __LINE__, WM_COMMAND, 2, STAGE_TYPE_SEQUENCE, 3,
        6,
        { WM_SHOWWINDOW, WM_WINDOWPOSCHANGING, WM_WINDOWPOSCHANGING,
          WM_ACTIVATEAPP, WM_NCACTIVATE, WM_ACTIVATE },
    },
    {
        __LINE__, WM_ACTIVATE, 3, STAGE_TYPE_SEQUENCE, 0,
        1,
        { WM_IME_SETCONTEXT },
        { ACTION_IME_SETCONTEXT_OPEN },
    },
    {
        __LINE__, WM_IME_SETCONTEXT, 4, STAGE_TYPE_SEQUENCE, 0,
        1,
        { WM_IME_NOTIFY },
        { ACTION_IME_NOTIFY_OPEN },
    },
    // deactivate
    {
        __LINE__, WM_COMMAND, 2, STAGE_TYPE_SEQUENCE, ACTION_DEACTIVATE,
        4,
        { WM_NCACTIVATE, WM_ACTIVATE, WM_ACTIVATEAPP, WM_KILLFOCUS },
    },
    {
        __LINE__, WM_COMMAND, 2, STAGE_TYPE_SEQUENCE, 0,
        1,
        { WM_IME_SETCONTEXT },
        { ACTION_IME_SETCONTEXT_CLOSE }
    },
    {
        __LINE__, WM_IME_SETCONTEXT, 3, STAGE_TYPE_SEQUENCE, 0,
        1,
        { WM_IME_NOTIFY },
        { ACTION_IME_NOTIFY_CLOSE }
    },
    // activate
    {
        __LINE__, WM_ACTIVATE, 3, STAGE_TYPE_SEQUENCE, ACTION_ACTIVATE,
        1,
        { WM_IME_SETCONTEXT },
        { ACTION_IME_SETCONTEXT_OPEN }
    },
    {
        __LINE__, WM_IME_SETCONTEXT, 4, STAGE_TYPE_SEQUENCE, 0,
        1,
        { WM_IME_NOTIFY },
        { ACTION_IME_NOTIFY_OPEN },
    },
    // destroy
    {
        __LINE__, WM_COMMAND, 2, STAGE_TYPE_SEQUENCE, ACTION_DESTROY,
        2,
        { WM_WINDOWPOSCHANGING, WM_WINDOWPOSCHANGED },
    },
    {
        __LINE__, WM_COMMAND, 2, STAGE_TYPE_SEQUENCE, 0,
        1,
        { WM_IME_SETCONTEXT },
        { ACTION_IME_SETCONTEXT_CLOSE }
    },
    {
        __LINE__, WM_IME_SETCONTEXT, 3, STAGE_TYPE_SEQUENCE, 0,
        1,
        { WM_IME_NOTIFY },
        { ACTION_IME_NOTIFY_CLOSE }
    },
    {
        __LINE__, WM_COMMAND, 2, STAGE_TYPE_SEQUENCE, 0,
        2,
        { WM_DESTROY, WM_NCDESTROY },
    },
};

static void IME_DoTest(void)
{
    WNDCLASSA wc;
    HWND hwnd;
    MSG msg;
    static const char s_szName[] = "MessageStateAnalyzerIME";

    trace("IME_DoTest\n");
    DoInitialize(s_IMEStages, ARRAYSIZE(s_IMEStages));

    /* register window class */
    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
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

    ok_int(UnregisterClassA(s_szName, GetModuleHandleA(NULL)), TRUE);

    DoFinish();
}

START_TEST(MessageStateAnalyzer)
{
    General_DoTest();
    IME_DoTest();
}
