/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for Shell Hook
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include "shelltest.h"
#include "undocshell.h"

static UINT s_uShellHookMsg = 0;
static HWND s_hwndHookViewer = NULL;
static HWND s_hwndParent = NULL;
static HWND s_hwndTarget = NULL;
static DWORD s_dwFlags = 0;
static WCHAR s_szName[] = L"ReactOS ShellHook testcase";

static HWND
DoCreateWindow(HWND hwndParent, DWORD style, DWORD exstyle)
{
    return CreateWindowExW(exstyle, s_szName, s_szName, style,
                           CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
                           hwndParent, NULL, GetModuleHandleW(NULL), NULL);
}

struct TEST_ENTRY
{
    INT lineno;
    DWORD dwFlags;
    BOOL bIsChild;
    BOOL bHasOwner;
    DWORD style;
    DWORD exstyle;
    DWORD owner_style;
    DWORD owner_exstyle;
};

#define STYLE_0  WS_POPUP
#define STYLE_1  (WS_POPUP | WS_VISIBLE)

#define EXSTYLE_0  0
#define EXSTYLE_1  WS_EX_APPWINDOW
#define EXSTYLE_2  WS_EX_TOOLWINDOW
#define EXSTYLE_3  (WS_EX_APPWINDOW | WS_EX_TOOLWINDOW)

#define TYPE_0 FALSE, FALSE
#define TYPE_1 FALSE, TRUE
#define TYPE_2 TRUE, TRUE

static const TEST_ENTRY s_entries[] =
{
    // STYLE_0, EXSTYLE_0
    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_0, STYLE_0, EXSTYLE_0 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_0, STYLE_1, EXSTYLE_0 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_0, STYLE_0, EXSTYLE_1 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_0, STYLE_1, EXSTYLE_1 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_0, STYLE_0, EXSTYLE_2 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_0, STYLE_1, EXSTYLE_2 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_0, STYLE_0, EXSTYLE_3 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_0, STYLE_1, EXSTYLE_3 },

    // STYLE_1, EXSTYLE_0
    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_0 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_0 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_1 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_1 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_2 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_2 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_3 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_3 },

    // STYLE_0, EXSTYLE_1
    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_1, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_1, STYLE_0, EXSTYLE_0 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_1, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_1, STYLE_1, EXSTYLE_0 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_1, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_1, STYLE_0, EXSTYLE_1 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_1, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_1, STYLE_1, EXSTYLE_1 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_1, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_1, STYLE_0, EXSTYLE_2 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_1, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_1, STYLE_1, EXSTYLE_2 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_1, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_1, STYLE_0, EXSTYLE_3 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_1, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_1, STYLE_1, EXSTYLE_3 },

    // STYLE_1, EXSTYLE_1
    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 3, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_0 },
    { __LINE__, 3, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_0 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 3, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_0 },
    { __LINE__, 3, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_0 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 3, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_1 },
    { __LINE__, 3, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_1 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 3, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_1 },
    { __LINE__, 3, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_1 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 3, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_2 },
    { __LINE__, 3, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_2 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 3, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_2 },
    { __LINE__, 3, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_2 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 3, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_3 },
    { __LINE__, 3, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_3 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 3, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_3 },
    { __LINE__, 3, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_3 },

    // STYLE_0, EXSTYLE_2
    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_2, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_2, STYLE_0, EXSTYLE_0 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_2, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_2, STYLE_1, EXSTYLE_0 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_2, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_2, STYLE_0, EXSTYLE_2 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_2, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_2, STYLE_1, EXSTYLE_2 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_2, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_2, STYLE_0, EXSTYLE_2 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_2, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_2, STYLE_1, EXSTYLE_2 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_2, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_2, STYLE_0, EXSTYLE_3 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_2, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_2, STYLE_1, EXSTYLE_3 },

    // STYLE_1, EXSTYLE_2
    { __LINE__, 0, TYPE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_0 },

    { __LINE__, 0, TYPE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_0 },

    { __LINE__, 0, TYPE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_1 },

    { __LINE__, 0, TYPE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_1 },

    { __LINE__, 0, TYPE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_2 },

    { __LINE__, 0, TYPE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_2 },

    { __LINE__, 0, TYPE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_3 },

    { __LINE__, 0, TYPE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_3 },

    // STYLE_0, EXSTYLE_3
    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_3, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_3, STYLE_0, EXSTYLE_0 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_3, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_3, STYLE_1, EXSTYLE_0 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_3, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_3, STYLE_0, EXSTYLE_2 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_3, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_3, STYLE_1, EXSTYLE_2 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_3, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_3, STYLE_0, EXSTYLE_2 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_3, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_3, STYLE_1, EXSTYLE_2 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_3, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_3, STYLE_0, EXSTYLE_3 },

    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_0, EXSTYLE_3, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_0, EXSTYLE_3, STYLE_1, EXSTYLE_3 },

    // STYLE_1, EXSTYLE_3
    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 3, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_0 },
    { __LINE__, 3, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_0 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 3, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_0 },
    { __LINE__, 3, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_0 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 3, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_1 },
    { __LINE__, 3, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_1 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 3, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_1 },
    { __LINE__, 3, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_1 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 3, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_2 },
    { __LINE__, 3, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_2 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 3, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_2 },
    { __LINE__, 3, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_2 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 3, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_3 },
    { __LINE__, 3, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_3 },

    { __LINE__, 3, TYPE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 3, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_3 },
    { __LINE__, 3, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_3 },
};

static const size_t s_num_entries = sizeof(s_entries) / sizeof(s_entries[0]);

static void DoTestEntry(const TEST_ENTRY *pEntry)
{
    ok(!pEntry->bIsChild || pEntry->bHasOwner,
       "Line %d: bIsChild && !bHasOwner\n", pEntry->lineno);

    s_hwndParent = NULL;
    if (pEntry->bIsChild || pEntry->bHasOwner)
    {
        s_hwndParent = DoCreateWindow(NULL, pEntry->owner_style, pEntry->owner_exstyle);
    }

    DWORD style = pEntry->style;
    DWORD exstyle = pEntry->exstyle;
    if (pEntry->bIsChild)
        style |= WS_CHILD;
    else
        style &= ~WS_CHILD;

    s_dwFlags = 0;

    s_hwndTarget = DoCreateWindow(s_hwndParent, style, exstyle);

    ok(s_dwFlags == pEntry->dwFlags, "Line %d: s_dwFlags expected 0x%08lX but was 0x%08lX\n",
       pEntry->lineno, pEntry->dwFlags, s_dwFlags);

    DestroyWindow(s_hwndTarget);
    s_hwndTarget = NULL;

    if (pEntry->bIsChild || pEntry->bHasOwner)
    {
        DestroyWindow(s_hwndParent);
        s_hwndParent = NULL;
    }
}

static LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == s_uShellHookMsg && uMsg != 0)
    {
        DWORD style, exstyle, owner_style, owner_exstyle;
        HWND hwndOwner;
        DWORD dwFlags;
        switch (wParam)
        {
            case HSHELL_WINDOWCREATED:
                style = (LONG)GetWindowLongPtrW(hwnd, GWL_STYLE);
                exstyle = (LONG)GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
                hwndOwner = GetWindow(hwnd, GW_OWNER);
                owner_style = (LONG)GetWindowLongPtrW(hwndOwner, GWL_STYLE);
                owner_exstyle = (LONG)GetWindowLongPtrW(hwndOwner, GWL_EXSTYLE);
                dwFlags = (1 << 0);
                if (style & WS_VISIBLE)
                    dwFlags |= (1 << 1);
                if (exstyle & WS_EX_TOOLWINDOW)
                    dwFlags |= (1 << 2);
                if (exstyle & WS_EX_APPWINDOW)
                    dwFlags |= (1 << 3);
                if (owner_style & WS_VISIBLE)
                    dwFlags |= (1 << 4);
                if (owner_exstyle & WS_EX_TOOLWINDOW)
                    dwFlags |= (1 << 5);
                if (owner_exstyle & WS_EX_APPWINDOW)
                    dwFlags |= (1 << 6);
                s_dwFlags = dwFlags;
                break;
        }
    }
    switch (uMsg)
    {
        case WM_DESTROY:
            if (s_hwndHookViewer == hwnd)
                PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

static BOOL s_bQuit = FALSE;

static DWORD WINAPI ThreadProc(LPVOID)
{
    for (size_t i = 0; i < s_num_entries; ++i)
    {
        DoTestEntry(&s_entries[i]);
    }

    s_bQuit = TRUE;
    return 0;
}

START_TEST(ShellHook)
{
    WNDCLASSW wc;

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName = s_szName;
    if (!RegisterClassW(&wc))
    {
        skip("RegisterClassW failed\n");
        return;
    }

    s_hwndHookViewer = DoCreateWindow(NULL, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0);
    if (s_hwndHookViewer == NULL)
    {
        skip("CreateWindowExW failed\n");
        return;
    }

    s_uShellHookMsg = RegisterWindowMessageW(L"SHELLHOOK");
    RegisterShellHookWindow(s_hwndHookViewer);

    s_bQuit = FALSE;
    HANDLE hThread = CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL);
    CloseHandle(hThread);

    MSG msg;
    while (!s_bQuit && GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DeregisterShellHookWindow(s_hwndHookViewer);
    DestroyWindow(s_hwndHookViewer);
    s_hwndHookViewer = NULL;
}
