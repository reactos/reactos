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

/* variables */
INT s_nStage;
INT s_nSeqIndex;
UINT s_msgStack[32];
INT s_nLevel;
BOOL s_bNextStage;
INT s_nCounters[8];

/* macros */
#define TIMEOUT_TIMER   999
#define TOTAL_TIMEOUT   (5 * 1000)
#define WIDTH           300
#define HEIGHT          200
#define PARENT_MSG      s_msgStack[s_nLevel - 1]
#define NEXT_STAGE()    s_bNextStage = TRUE

static void General_Initialize(void)
{
    s_nStage = s_nSeqIndex = 0;
    ZeroMemory(s_msgStack, sizeof(s_msgStack));
    s_nLevel = 0;
    s_bNextStage = FALSE;
    ZeroMemory(s_nCounters, sizeof(s_nCounters));
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
    INT nFirstAction;
    STAGE_TYPE nType;
    INT nCount;
    UINT Messages[16];
    INT Actions[16];
    INT Counters[16];
} STAGE;

static const STAGE s_GeneralStages[] =
{
    {
        __LINE__, WM_NULL, 1, 0, STAGE_TYPE_SEQUENCE,
        4, { WM_GETMINMAXINFO, WM_NCCREATE, WM_NCCALCSIZE, WM_CREATE },
           { 1, 2, 0, 0 },
    },
    {
        __LINE__, WM_COMMAND, 2, 3, STAGE_TYPE_SEQUENCE,
        6, { WM_SHOWWINDOW, WM_WINDOWPOSCHANGING, WM_WINDOWPOSCHANGING,
             WM_ACTIVATEAPP, WM_NCACTIVATE, WM_ACTIVATE },
    },
    {
        __LINE__, WM_ACTIVATE, 3, 0, STAGE_TYPE_COUNTING, 
        1, { WM_IME_SETCONTEXT }, { 4 }, { 1 }
    },
    {
        __LINE__, WM_IME_SETCONTEXT, 4, 0, STAGE_TYPE_COUNTING,
        1, { WM_IME_NOTIFY }, { 4 }, { 1 }
    },
    {
        __LINE__, WM_COMMAND, 2, 5, STAGE_TYPE_SEQUENCE,
        7, { WM_WINDOWPOSCHANGING, WM_WINDOWPOSCHANGED, WM_NCACTIVATE,
             WM_ACTIVATE, WM_ACTIVATEAPP, WM_KILLFOCUS, WM_IME_SETCONTEXT },
    },
    {
        __LINE__, WM_IME_SETCONTEXT, 3, 0, STAGE_TYPE_SEQUENCE,
        1, { WM_IME_NOTIFY },
    },
    {
        __LINE__, WM_COMMAND, 2, 0, STAGE_TYPE_SEQUENCE,
        2, { WM_DESTROY, WM_NCDESTROY },
    },
};

static void General_DoAction(HWND hwnd, INT nAction)
{
    RECT rc;
    switch (nAction)
    {
        case 1:
            ok_int(s_nStage, 0);
            GetWindowRect(hwnd, &rc);
            ok_long(rc.right - rc.left, 0);
            ok_long(rc.bottom - rc.top, 0);
            ok_int(IsWindowVisible(hwnd), FALSE);
            break;
        case 2:
            ok_int(s_nStage, 0);
            GetWindowRect(hwnd, &rc);
            ok_long(rc.right - rc.left, WIDTH);
            ok_long(rc.bottom - rc.top, HEIGHT);
            ok_int(IsWindowVisible(hwnd), FALSE);
            break;
        case 3:
            ok_int(s_nStage, 1);
            ShowWindow(hwnd, SW_SHOWNORMAL);
            break;
        case 4:
            ok(s_nStage == 2 || s_nStage == 3, "\n");
            NEXT_STAGE();
            break;
        case 5:
            ok_int(s_nStage, 4);
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
    INT i;
    const STAGE *pStage;
    s_bNextStage = FALSE;

    if (s_nStage >= ARRAYSIZE(s_GeneralStages))
        return;

    pStage = &s_GeneralStages[s_nStage];
    switch (pStage->nType)
    {
        case STAGE_TYPE_SEQUENCE:
            if (pStage->Messages[s_nSeqIndex] == uMsg)
            {
                ok_int(1, 1);
                ok(s_nLevel == pStage->nLevel,
                   "Line %d: Level expected %d but %d.\n",
                   pStage->nLine, pStage->nLevel, s_nLevel);
                ok(PARENT_MSG == pStage->uParentMsg,
                   "Line %d: PARENT_MSG expected %u but %u.\n",
                   pStage->nLine, pStage->uParentMsg, PARENT_MSG);
                General_DoAction(hwnd, pStage->Actions[s_nSeqIndex]);
                ++s_nSeqIndex;
                if (s_nSeqIndex == pStage->nCount)
                    NEXT_STAGE();
            }
            break;
        case STAGE_TYPE_COUNTING:
            for (i = 0; i < pStage->nCount; ++i)
            {
                if (pStage->Messages[i] == uMsg)
                {
                    ok_int(1, 1);
                    ok(s_nLevel == pStage->nLevel,
                       "Line %d: Level expected %d but %d.\n",
                       pStage->nLine, pStage->nLevel, s_nLevel);
                    ok(PARENT_MSG == pStage->uParentMsg,
                       "Line %d: PARENT_MSG expected %u but %u.\n",
                       pStage->nLine, pStage->uParentMsg, PARENT_MSG);
                    General_DoAction(hwnd, pStage->Actions[i]);
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
            for (i = 0; i < pStage->nCount; ++i)
            {
                if (pStage->Counters[i])
                {
                    ok(pStage->Counters[i] == s_nCounters[i],
                       "Line %d: s_nCounters[%d] expected %d but %d.\n",
                       pStage->nLine, i, pStage->Counters[i], s_nCounters[i]);
                }
            }
        }

        ++s_nStage;
        if (s_nStage == ARRAYSIZE(s_GeneralStages))
        {
            DestroyWindow(hwnd);
            return;
        }

        PostMessage(hwnd, WM_COMMAND, s_GeneralStages[s_nStage].nFirstAction, 0);
        trace("Stage %d (Line %d)\n", s_nStage, s_GeneralStages[s_nStage].nLine);

        s_nSeqIndex = 0;
        ZeroMemory(s_nCounters, sizeof(s_nCounters));
    }
}

static LRESULT CALLBACK
General_InnerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_COMMAND:
            General_DoAction(hwnd, LOWORD(wParam));
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

    ++s_nLevel;
    s_msgStack[s_nLevel] = uMsg;
    {
        /* do inner task */
        General_DoStage(hwnd, uMsg, wParam, lParam);
        lResult = General_InnerWindowProc(hwnd, uMsg, wParam, lParam);
    }
    --s_nLevel;

    /* message return */
    StringCbCopyA(s_prefix, sizeof(s_prefix), "R: ");
    MD_msgresult(hwnd, uMsg, wParam, lParam, lResult);
    return lResult;
}

static void General_Finish(void)
{
    ok_int(s_nStage, ARRAYSIZE(s_GeneralStages));
    if (s_nStage != ARRAYSIZE(s_GeneralStages))
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
