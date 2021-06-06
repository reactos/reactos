/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for Shell Hook
 * COPYRIGHT:   Copyright 2020-2021 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include "shelltest.h"
#include "undocshell.h"

static UINT s_uShellHookMsg = 0;
static HWND s_hwndHookViewer = NULL;
static HWND s_hwndParent = NULL;
static HWND s_hwndTarget = NULL;
static UINT s_nWindowCreatedCount = 0;
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
    UINT nCount;
    BOOL bIsChild;
    BOOL bHasOwner;
    DWORD style;
    DWORD exstyle;
    DWORD owner_style;
    DWORD owner_exstyle;
};

#define STYLE_0  WS_POPUP
#define STYLE_1  (WS_POPUP | WS_VISIBLE)
#define STYLE_2  (WS_OVERLAPPED | WS_VISIBLE)

#define EXSTYLE_0  0
#define EXSTYLE_1  WS_EX_APPWINDOW
#define EXSTYLE_2  WS_EX_TOOLWINDOW
#define EXSTYLE_3  (WS_EX_APPWINDOW | WS_EX_TOOLWINDOW)

#define TYPE_0 FALSE, FALSE
#define TYPE_1 FALSE, TRUE
#define TYPE_2 TRUE, TRUE

static const TEST_ENTRY s_entries[] =
{
    // TYPE_0
    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 1, TYPE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 1, TYPE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, TYPE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 1, TYPE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 1, TYPE_0, STYLE_2, EXSTYLE_0 },
    { __LINE__, 1, TYPE_0, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, TYPE_0, STYLE_2, EXSTYLE_2 },
    { __LINE__, 1, TYPE_0, STYLE_2, EXSTYLE_3 },

    // TYPE_1
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_2, EXSTYLE_3 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_0 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_1 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_2 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_3 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_0 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_1 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_2 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_3 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_2, EXSTYLE_0 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_2, EXSTYLE_1 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_2, EXSTYLE_2 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_2, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_2, EXSTYLE_3 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_0 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_1 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_2 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_3 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_0 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_1 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_2 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_3 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_2, EXSTYLE_0 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_2, EXSTYLE_1 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_2, EXSTYLE_2 },
    { __LINE__, 1, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_2, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_2, EXSTYLE_3 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_0, EXSTYLE_0 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_0, EXSTYLE_1 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_0, EXSTYLE_2 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_0, EXSTYLE_3 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_1, EXSTYLE_0 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_1, EXSTYLE_1 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_1, EXSTYLE_2 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_1, EXSTYLE_3 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_2, EXSTYLE_0 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_2, EXSTYLE_1 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_2, EXSTYLE_2 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_2, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_2, EXSTYLE_3 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_0, EXSTYLE_0 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_0, EXSTYLE_1 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_0, EXSTYLE_2 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_0, EXSTYLE_3 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_1, EXSTYLE_0 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_1, EXSTYLE_1 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_1, EXSTYLE_2 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_1, EXSTYLE_3 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_2, EXSTYLE_0 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_2, EXSTYLE_1 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_2, EXSTYLE_2 },
    { __LINE__, 1, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_2, EXSTYLE_3 },

    // TYPE_2
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_2, EXSTYLE_3 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_0 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_1 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_2 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_3 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_0 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_1 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_2 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_3 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_2, EXSTYLE_0 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_2, EXSTYLE_1 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_2, EXSTYLE_2 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_2, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_2, EXSTYLE_3 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_0 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_1 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_2 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_3 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_0 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_1 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_2 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_3 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_2, EXSTYLE_0 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_2, EXSTYLE_1 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_2, EXSTYLE_2 },
    { __LINE__, 1, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_2, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_2, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_2, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_2, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_2, EXSTYLE_3 },
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

    s_nWindowCreatedCount = 0;
    s_hwndTarget = DoCreateWindow(s_hwndParent, style, exstyle);
}

static void DoTestEntryPart2(const TEST_ENTRY *pEntry)
{
    ok(s_nWindowCreatedCount == pEntry->nCount,
       "Line %d: s_nWindowCreatedCount expected %u but was %u\n",
       pEntry->lineno, pEntry->nCount, s_nWindowCreatedCount);

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
        switch (wParam)
        {
            case HSHELL_WINDOWCREATED:
                if ((HWND)lParam != s_hwndTarget)
                    break;
                ++s_nWindowCreatedCount;
                break;
        }
    }
#define ID_IGNITION 1000
#define ID_BURNING 2000
    switch (uMsg)
    {
        case WM_CREATE:
            PostMessageW(hwnd, WM_COMMAND, ID_IGNITION, 0);
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
        case WM_COMMAND:
            if (hwnd != s_hwndHookViewer)
                break;

            if (ID_IGNITION <= wParam && wParam < ID_BURNING)
            {
                INT i = (INT)wParam - ID_IGNITION;
                DoTestEntryPart1(&s_entries[i]);
                PostMessageW(hwnd, WM_COMMAND, ID_BURNING + i, 0);
            }
            else if (ID_BURNING <= wParam)
            {
                INT i = (INT)wParam - ID_BURNING;
                DoTestEntryPart2(&s_entries[i]);
                ++i;
                if (i == (INT)s_num_entries)
                {
                    PostQuitMessage(0);
                    break;
                }
                PostMessageW(hwnd, WM_COMMAND, ID_IGNITION + i, 0);
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
