/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     debugging and analysis of message states
 * COPYRIGHT:   Copyright 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
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

/* variables */
static INT s_iStage;
static INT s_iStep;
static INT s_nLevel;
static BOOL s_bNextStage;
static INT s_nCounters[10];
static UINT s_msgStack[32];

/* macros */
#define TIMEOUT_TIMER   999
#define TOTAL_TIMEOUT   (5 * 1000)
#define WIDTH           300
#define HEIGHT          200
#define PARENT_MSG      s_msgStack[s_nLevel - 1]

static void DoInitialize(void)
{
    s_iStage = s_iStep = s_nLevel = 0;
    s_bNextStage = FALSE;
    ZeroMemory(s_nCounters, sizeof(s_nCounters));
    ZeroMemory(s_msgStack, sizeof(s_msgStack));
}

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

static const STAGE s_GeneralStages[] =
{
    /* Stage 0 */
    {
        __LINE__, WM_NULL, 1, STAGE_TYPE_SEQUENCE, 0,
        4,
        { WM_GETMINMAXINFO, WM_NCCREATE, WM_NCCALCSIZE, WM_CREATE },
        { 1, 2, 0, 0 },
    },
    /* Stage 1 */
    {
        __LINE__, WM_COMMAND, 2, STAGE_TYPE_SEQUENCE, 3,
        6,
        { WM_SHOWWINDOW, WM_WINDOWPOSCHANGING, WM_WINDOWPOSCHANGING,
          WM_ACTIVATEAPP, WM_NCACTIVATE, WM_ACTIVATE },
    },
    /* Stage 2 */
    {
        __LINE__, WM_ACTIVATE, 3, STAGE_TYPE_COUNTING, 0,
        1,
        { WM_IME_SETCONTEXT },
        { 4 },
        { 1 }
    },
    /* Stage 3 */
    {
        __LINE__, WM_IME_SETCONTEXT, 4, STAGE_TYPE_COUNTING, 0,
        1,
        { WM_IME_NOTIFY },
        { 4 },
        { 1 }
    },
    /* Stage 4 */
    {
        __LINE__, WM_COMMAND, 2, STAGE_TYPE_SEQUENCE, 5,
        7,
        { WM_WINDOWPOSCHANGING, WM_WINDOWPOSCHANGED, WM_NCACTIVATE,
          WM_ACTIVATE, WM_ACTIVATEAPP, WM_KILLFOCUS, WM_IME_SETCONTEXT },
    },
    /* Stage 5 */
    {
        __LINE__, WM_IME_SETCONTEXT, 3, STAGE_TYPE_SEQUENCE, 0,
        1,
        { WM_IME_NOTIFY },
    },
    /* Stage 6 */
    {
        __LINE__, WM_COMMAND, 2, STAGE_TYPE_SEQUENCE, 0,
        2,
        { WM_DESTROY, WM_NCDESTROY },
    },
};

static void
DoAction(HWND hwnd, INT iAction, WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    switch (iAction)
    {
        case 0:
            /* does nothing */
            break;
        case 1:
            ok_int(s_iStage, 0);
            GetWindowRect(hwnd, &rc);
            ok_long(rc.right - rc.left, 0);
            ok_long(rc.bottom - rc.top, 0);
            ok_int(IsWindowVisible(hwnd), FALSE);
            break;
        case 2:
            ok_int(s_iStage, 0);
            GetWindowRect(hwnd, &rc);
            ok_long(rc.right - rc.left, WIDTH);
            ok_long(rc.bottom - rc.top, HEIGHT);
            ok_int(IsWindowVisible(hwnd), FALSE);
            break;
        case 3:
            ok_int(s_iStage, 1);
            ShowWindow(hwnd, SW_SHOWNORMAL);
            break;
        case 4:
            ok(s_iStage == 2 || s_iStage == 3, "\n");
            s_bNextStage = TRUE;
            break;
        case 5:
            ok_int(s_iStage, 4);
            DestroyWindow(hwnd);
            break;
        case TIMEOUT_TIMER:
            DestroyWindow(hwnd);
            break;
    }
}

static void
DoStage(const STAGE *pStages, INT cStages,
        HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT i;
    const STAGE *pStage;
    INT iAction;
    s_bNextStage = FALSE;

    if (s_iStage >= cStages)
        return;

    pStage = &pStages[s_iStage];
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
                if (s_iStep == pStage->nCount)
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
                    break;
                }
            }
            break;
    }

    if (s_bNextStage)
    {
        if (pStage->nType == STAGE_TYPE_COUNTING)
        {
            /* check counters */
            for (i = 0; i < pStage->nCount; ++i)
            {
                if (pStage->nCounters[i])
                {
                    ok(pStage->nCounters[i] == s_nCounters[i],
                       "Line %d: s_nCounters[%d] expected %d but %d.\n",
                       pStage->nLine, i, pStage->nCounters[i], s_nCounters[i]);
                }
            }
        }

        /* go to next stage */
        ++s_iStage;
        if (s_iStage == cStages)
        {
            DestroyWindow(hwnd);
            return;
        }
        trace("Stage %d (Line %d)\n", s_iStage, pStages[s_iStage].nLine);

        s_iStep = 0;
        ZeroMemory(s_nCounters, sizeof(s_nCounters));

        iAction = pStages[s_iStage].iFirstAction;
        if (iAction)
            PostMessage(hwnd, WM_COMMAND, iAction, 0);
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

static LRESULT CALLBACK
General_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
        DoStage(s_GeneralStages, ARRAYSIZE(s_GeneralStages),
                hwnd, uMsg, wParam, lParam);
        lResult = InnerWindowProc(hwnd, uMsg, wParam, lParam);
    }
    --s_nLevel;

    /* message return */
    StringCbCopyA(s_prefix, sizeof(s_prefix), "R: ");
    MD_msgresult(hwnd, uMsg, wParam, lParam, lResult);
    return lResult;
}

static void General_Finish(void)
{
    ok_int(s_iStage, ARRAYSIZE(s_GeneralStages));
    if (s_iStage != ARRAYSIZE(s_GeneralStages))
    {
        skip("Some stage(s) skipped (Step: %d)\n", s_iStep);
    }
}

static void General_DoTest(void)
{
    WNDCLASSA wc;
    HWND hwnd;
    MSG msg;
    static const char s_szName[] = "MessageStateAnalyzerGeneral";

    DoInitialize();

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
