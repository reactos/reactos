/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     debugging and analysis of message states
 * COPYRIGHT:   Copyright 2019-2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include "undocuser.h"
#include "winxx.h"
#include <imm.h>
#include <strsafe.h>

#define MAX_MSGS 512

static MSG s_Msgs[MAX_MSGS];
static UINT s_cMsgs = 0;
static CHAR s_prefix[16] = "";
static HWND s_hMainWnd = NULL;
static HWND s_hwndEdit = NULL;
static HWND s_hImeWnd = NULL;
static WNDPROC s_fnOldEditWndProc = NULL;
static WNDPROC s_fnOldImeWndProc = NULL;

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
#define MSGDUMP_PREFIX s_prefix
#include "msgdump.h"    /* msgdump.h needs MSGDUMP_TPRINTF and MSGDUMP_PREFIX */

static void MD_build_prefix(void)
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

#define STAGE_1  1
#define STAGE_2  2
#define STAGE_3  3
#define STAGE_4  4
#define STAGE_5  5

static INT findMessage(INT iMsg, HWND hwnd, UINT uMsg)
{
    if (iMsg == -1)
        iMsg = 0;
    for (; iMsg < s_cMsgs; ++iMsg)
    {
        if (s_Msgs[iMsg].message == uMsg && s_Msgs[iMsg].hwnd == hwnd)
            return iMsg;
    }
    return -1;
}

typedef struct TEST_ENTRY
{
    INT line;
    LPCSTR name;
    UINT iFound;
} TEST_ENTRY, *PTEST_ENTRY;

static VOID DoAnalyzeEntries(size_t nCount, PTEST_ENTRY pEntries)
{
    size_t i;
    for (i = 0; i < nCount - 1; ++i)
    {
        PTEST_ENTRY entry1 = &pEntries[i];
        PTEST_ENTRY entry2 = &pEntries[i + 1];
        ok(entry1->iFound < entry2->iFound,
           "Line %d: message wrong order (%d >= %d): %s vs %s\n",
           entry1->line, entry1->iFound, entry2->iFound,
           entry1->name, entry2->name);
    }
}

static VOID DoAnalyzeAllMessages(VOID)
{
    size_t i;
    TEST_ENTRY entries1[] =
    {
        { __LINE__, "WM_NCCREATE", findMessage(0, s_hMainWnd, WM_NCCREATE) },
        { __LINE__, "WM_NCCALCSIZE", findMessage(0, s_hMainWnd, WM_NCCALCSIZE) },
        { __LINE__, "WM_CREATE", findMessage(0, s_hMainWnd, WM_CREATE) },
        { __LINE__, "WM_PARENTNOTIFY", findMessage(0, s_hMainWnd, WM_PARENTNOTIFY) },
        { __LINE__, "WM_WINDOWPOSCHANGING", findMessage(0, s_hMainWnd, WM_WINDOWPOSCHANGING) },
        { __LINE__, "WM_ACTIVATEAPP", findMessage(0, s_hMainWnd, WM_ACTIVATEAPP) },
        { __LINE__, "WM_NCACTIVATE", findMessage(0, s_hMainWnd, WM_NCACTIVATE) },
        { __LINE__, "WM_ACTIVATE", findMessage(0, s_hMainWnd, WM_ACTIVATE) },
        { __LINE__, "WM_IME_SETCONTEXT", findMessage(0, s_hMainWnd, WM_IME_SETCONTEXT) },
        { __LINE__, "WM_IME_NOTIFY", findMessage(0, s_hMainWnd, WM_IME_NOTIFY) },
        { __LINE__, "WM_SETFOCUS", findMessage(0, s_hMainWnd, WM_SETFOCUS) },
        { __LINE__, "WM_KILLFOCUS", findMessage(0, s_hMainWnd, WM_KILLFOCUS) },
    };
    INT iFound1 = entries1[_countof(entries1) - 1].iFound;
    TEST_ENTRY entries2[] =
    {
        { __LINE__, "WM_IME_SETCONTEXT", findMessage(iFound1, s_hMainWnd, WM_IME_SETCONTEXT) },
        { __LINE__, "WM_IME_SETCONTEXT", findMessage(iFound1, s_hwndEdit, WM_IME_SETCONTEXT) },
        { __LINE__, "WM_SETFOCUS", findMessage(iFound1, s_hwndEdit, WM_SETFOCUS) },
        { __LINE__, "WM_SHOWWINDOW", findMessage(iFound1, s_hMainWnd, WM_SHOWWINDOW) },
        { __LINE__, "WM_WINDOWPOSCHANGING", findMessage(iFound1, s_hMainWnd, WM_WINDOWPOSCHANGING) },
        { __LINE__, "WM_NCPAINT", findMessage(iFound1, s_hMainWnd, WM_NCPAINT) },
        { __LINE__, "WM_ERASEBKGND", findMessage(iFound1, s_hMainWnd, WM_ERASEBKGND) },
        { __LINE__, "WM_WINDOWPOSCHANGED", findMessage(iFound1, s_hMainWnd, WM_WINDOWPOSCHANGED) },
        { __LINE__, "WM_SIZE", findMessage(iFound1, s_hMainWnd, WM_SIZE) },
        { __LINE__, "WM_MOVE", findMessage(iFound1, s_hMainWnd, WM_MOVE) },
    };
    INT iFound2 = entries2[_countof(entries2) - 1].iFound;
    TEST_ENTRY entries3[] =
    {
        { __LINE__, "WM_IME_KEYDOWN", findMessage(iFound2, s_hwndEdit, WM_IME_KEYDOWN) },
        { __LINE__, "WM_KEYDOWN", findMessage(iFound2, s_hwndEdit, WM_KEYDOWN) },
        { __LINE__, "WM_IME_KEYUP", findMessage(iFound2, s_hwndEdit, WM_IME_KEYUP) },
        { __LINE__, "WM_CHAR", findMessage(iFound2, s_hwndEdit, WM_CHAR) },
        { __LINE__, "WM_IME_CHAR", findMessage(iFound2, s_hwndEdit, WM_IME_CHAR) },
    };
    INT iFound3 = entries3[_countof(entries3) - 1].iFound;
    TEST_ENTRY entries4[] =
    {
        { __LINE__, "WM_IME_NOTIFY", findMessage(iFound3, s_hwndEdit, WM_IME_NOTIFY) },
        { __LINE__, "WM_IME_NOTIFY", findMessage(iFound3, s_hImeWnd, WM_IME_NOTIFY) },
        { __LINE__, "WM_IME_SETCONTEXT", findMessage(iFound3, s_hwndEdit, WM_IME_SETCONTEXT) },
        { __LINE__, "WM_IME_SETCONTEXT", findMessage(iFound3, s_hImeWnd, WM_IME_SETCONTEXT) },
    };
    INT iFound4 = entries4[_countof(entries4) - 1].iFound;
    TEST_ENTRY entries5[] =
    {
        { __LINE__, "WM_DESTROY", findMessage(iFound4, s_hMainWnd, WM_DESTROY) },
        { __LINE__, "WM_DESTROY", findMessage(iFound4, s_hwndEdit, WM_DESTROY) },
        { __LINE__, "WM_NCDESTROY", findMessage(iFound4, s_hwndEdit, WM_NCDESTROY) },
        { __LINE__, "WM_NCDESTROY", findMessage(iFound4, s_hMainWnd, WM_NCDESTROY) },
    };
    DoAnalyzeEntries(_countof(entries1), entries1);
    DoAnalyzeEntries(_countof(entries2), entries2);
    DoAnalyzeEntries(_countof(entries3), entries3);
    //DoAnalyzeEntries(_countof(entries4), entries4); // No order
    DoAnalyzeEntries(_countof(entries5), entries5);

    ok(iFound1 < entries2[0].iFound, "%d vs %d\n", iFound1, entries2[0].iFound);
    ok(iFound2 < entries3[0].iFound, "%d vs %d\n", iFound2, entries3[0].iFound);
    ok(iFound3 < entries4[0].iFound, "%d vs %d\n", iFound3, entries4[0].iFound);
    ok(iFound4 < entries5[0].iFound, "%d vs %d\n", iFound4, entries5[0].iFound);

    for (i = 0; i < _countof(entries4); ++i)
    {
        ok(entries4[i].iFound != -1, "entries4[%d].iFound was -1\n", i);
    }
}

static LRESULT CALLBACK
EditWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret;
    MSG msg = { hwnd, uMsg, wParam, lParam };

    /* Build s_prefix */
    MD_build_prefix();

    /* message dump */
    MD_msgdump(hwnd, uMsg, wParam, lParam);

    /* Add message */
    if (s_cMsgs < MAX_MSGS)
        s_Msgs[s_cMsgs++] = msg;

    /* Do inner task */
    ret = CallWindowProc(s_fnOldEditWndProc, hwnd, uMsg, wParam, lParam);

    /* message return */
    StringCbCopyA(s_prefix, sizeof(s_prefix), "R: ");
    MD_msgresult(hwnd, uMsg, wParam, lParam, ret);

    return ret;
}

static LRESULT CALLBACK
ImeWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret;
    MSG msg = { hwnd, uMsg, wParam, lParam };

    /* Build s_prefix */
    MD_build_prefix();

    /* message dump */
    MD_msgdump(hwnd, uMsg, wParam, lParam);

    /* Add message */
    if (s_cMsgs < MAX_MSGS)
        s_Msgs[s_cMsgs++] = msg;

    /* Do inner task */
    ret = CallWindowProc(s_fnOldImeWndProc, hwnd, uMsg, wParam, lParam);

    /* message return */
    StringCbCopyA(s_prefix, sizeof(s_prefix), "R: ");
    MD_msgresult(hwnd, uMsg, wParam, lParam, ret);

    return ret;
}

static LRESULT CALLBACK
InnerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_NCCREATE:
            s_hMainWnd = hwnd;
            trace("s_hMainWnd: %p\n", s_hMainWnd);
            break;
        case WM_CREATE:
            s_hwndEdit = CreateWindowW(L"EDIT", NULL, WS_CHILD | WS_VISIBLE,
                                       0, 0, 100, 20, hwnd, NULL, GetModuleHandleW(NULL), NULL);
            ok(s_hwndEdit != NULL, "s_hwndEdit was NULL\n");
            trace("s_hwndEdit: %p\n", s_hwndEdit);
            s_fnOldEditWndProc =
                (WNDPROC)SetWindowLongPtrW(s_hwndEdit, GWLP_WNDPROC, (LONG_PTR)EditWindowProc);
            SetFocus(s_hwndEdit);
            PostMessageW(hwnd, WM_COMMAND, STAGE_1, 0);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case STAGE_1:
                    s_hImeWnd = ImmGetDefaultIMEWnd(hwnd);
                    ok(s_hImeWnd != NULL, "s_hImeWnd was NULL\n");
                    trace("s_hImeWnd: %p\n", s_hImeWnd );
                    s_fnOldImeWndProc = (WNDPROC)SetWindowLongPtrW(s_hImeWnd, GWLP_WNDPROC,
                                                                   (LONG_PTR)ImeWindowProc);
                    PostMessageW(hwnd, WM_COMMAND, STAGE_2, 0);
                    break;
                case STAGE_2:
                    PostMessageW(s_hwndEdit, WM_IME_KEYDOWN, 'A', 0);
                    PostMessageW(hwnd, WM_COMMAND, STAGE_3, 0);
                    break;
                case STAGE_3:
                    PostMessageW(s_hwndEdit, WM_IME_KEYUP, 'A', 0);
                    PostMessageW(hwnd, WM_COMMAND, STAGE_4, 0);
                    break;
                case STAGE_4:
                    PostMessageW(s_hwndEdit, WM_IME_CHAR, 'A', 0);
                    PostMessageW(hwnd, WM_COMMAND, STAGE_5, 0);
                    break;
                case STAGE_5:
                    PostMessageW(hwnd, WM_CLOSE, 0, 0);
                    break;
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret;
    MSG msg = { hwnd, uMsg, wParam, lParam };

    /* Build s_prefix */
    MD_build_prefix();

    /* message dump */
    MD_msgdump(hwnd, uMsg, wParam, lParam);

    /* Add message */
    if (s_cMsgs < MAX_MSGS)
        s_Msgs[s_cMsgs++] = msg;

    /* Do inner task */
    ret = InnerWindowProc(hwnd, uMsg, wParam, lParam);

    /* message return */
    StringCbCopyA(s_prefix, sizeof(s_prefix), "R: ");
    MD_msgresult(hwnd, uMsg, wParam, lParam, ret);

    return ret;
}

START_TEST(MessageStateAnalyzer)
{
    WNDCLASSW wc;
    HWND hwnd;
    MSG msg;
    static const WCHAR s_szName[] = L"MessageStateAnalyzer";

    /* register window class */
    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName = s_szName;
    if (!RegisterClassW(&wc))
    {
        skip("RegisterClassW failed.\n");
        return;
    }

    /* create a window */
    hwnd = CreateWindowW(s_szName, s_szName, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                         CW_USEDEFAULT, CW_USEDEFAULT, 300, 200, NULL, NULL,
                         GetModuleHandleW(NULL), NULL);
    if (!hwnd)
    {
        skip("CreateWindowW failed.\n");
        return;
    }

    /* message loop */
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DoAnalyzeAllMessages();
}
