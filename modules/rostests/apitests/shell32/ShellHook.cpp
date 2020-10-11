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
    { __LINE__, 0xB, TYPE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0xB, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0xB, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_0 },

    { __LINE__, 0xB, TYPE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0x1B, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0x1B, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_0 },

    { __LINE__, 0xB, TYPE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0x4B, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0x4B, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_1 },

    { __LINE__, 0xB, TYPE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0x5B, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0x5B, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_1 },

    { __LINE__, 0xB, TYPE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0x2B, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0x2B, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_2 },

    { __LINE__, 0xB, TYPE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0x3B, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0x3B, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_2 },

    { __LINE__, 0xB, TYPE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0x6B, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0x6B, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_3 },

    { __LINE__, 0xB, TYPE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0x7B, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0x7B, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_3 },

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
    { __LINE__, 0xF, TYPE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0xF, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0xF, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_0 },

    { __LINE__, 0xF, TYPE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0x1F, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0x1F, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_0 },

    { __LINE__, 0xF, TYPE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0x4F, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0x4F, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_1 },

    { __LINE__, 0xF, TYPE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0x5F, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0x5F, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_1 },

    { __LINE__, 0xF, TYPE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0x2F, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0x2F, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_2 },

    { __LINE__, 0xF, TYPE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0x3F, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0x3F, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_2 },

    { __LINE__, 0xF, TYPE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0x6F, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0x6F, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_3 },

    { __LINE__, 0xF, TYPE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0x7F, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0x7F, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_3 },
};

static const size_t s_num_entries = sizeof(s_entries) / sizeof(s_entries[0]);

static void DoTestEntryPart1(const TEST_ENTRY *pEntry)
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
}

static void DoTestEntryPart2(const TEST_ENTRY *pEntry)
{
    ok(s_dwFlags == pEntry->dwFlags, "Line %d: s_dwFlags expected 0x%lX but was 0x%lX\n",
       pEntry->lineno, pEntry->dwFlags, s_dwFlags);

    PostMessageW(s_hwndTarget, WM_CLOSE, 0, 0);
    s_hwndTarget = NULL;

    if (pEntry->bIsChild || pEntry->bHasOwner)
    {
        PostMessageW(s_hwndParent, WM_CLOSE, 0, 0);
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
                if ((HWND)lParam != s_hwndTarget)
                    break;
                style = (LONG)GetWindowLongPtrW(s_hwndTarget, GWL_STYLE);
                exstyle = (LONG)GetWindowLongPtrW(s_hwndTarget, GWL_EXSTYLE);
                if (style & WS_CHILD)
                    hwndOwner = GetParent(s_hwndTarget);
                else
                    hwndOwner = GetWindow(s_hwndTarget, GW_OWNER);
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
        case WM_CREATE:
            PostMessageW(hwnd, WM_COMMAND, 1000, 0);
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
        case WM_COMMAND:
            if (hwnd == s_hwndHookViewer)
            {
                if (1000 <= wParam && wParam < 2000)
                {
                    INT i = (INT)wParam - 1000;
                    DoTestEntryPart1(&s_entries[i]);
                    PostMessageW(hwnd, WM_COMMAND, 2000 + i, 0);
                }
                else if (2000 <= wParam && wParam < 3000)
                {
                    INT i = (INT)wParam - 2000;
                    DoTestEntryPart2(&s_entries[i]);
                    ++i;
                    if (i == s_num_entries)
                    {
                        PostQuitMessage(0);
                        break;
                    }
                    PostMessageW(hwnd, WM_COMMAND, 1000 + i, 0);
                }
            }
            break;
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
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

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DeregisterShellHookWindow(s_hwndHookViewer);
    DestroyWindow(s_hwndHookViewer);
    s_hwndHookViewer = NULL;
}
