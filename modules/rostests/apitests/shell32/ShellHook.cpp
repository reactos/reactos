/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for Shell Hook
 * COPYRIGHT:   Copyright 2020-2024 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include "shelltest.h"

struct TEST_ENTRY
{
    INT lineno;
    UINT cCreated;
    UINT cDestroyed;
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

static const TEST_ENTRY s_entries1[] =
{
    // TYPE_0
    { __LINE__, 0, 0, TYPE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, 0, TYPE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, 0, TYPE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 1, 1, TYPE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 1, 1, TYPE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 1, 1, TYPE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 1, 1, TYPE_0, STYLE_2, EXSTYLE_0 },
    { __LINE__, 1, 1, TYPE_0, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_0, STYLE_2, EXSTYLE_2 },
    { __LINE__, 1, 1, TYPE_0, STYLE_2, EXSTYLE_3 },

    // TYPE_1
    { __LINE__, 0, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_1, STYLE_1, EXSTYLE_0, STYLE_2, EXSTYLE_3 },
    { __LINE__, 1, 1, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_0 },
    { __LINE__, 1, 1, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_1 },
    { __LINE__, 1, 1, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_2 },
    { __LINE__, 1, 1, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_3 },
    { __LINE__, 1, 2, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_0 },
    { __LINE__, 1, 2, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_1 },
    { __LINE__, 1, 1, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_2 },
    { __LINE__, 1, 2, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_3 },
    { __LINE__, 1, 2, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_2, EXSTYLE_0 },
    { __LINE__, 1, 2, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_2, EXSTYLE_1 },
    { __LINE__, 1, 1, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_2, EXSTYLE_2 },
    { __LINE__, 1, 2, TYPE_1, STYLE_1, EXSTYLE_1, STYLE_2, EXSTYLE_3 },
    { __LINE__, 0, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_1, STYLE_1, EXSTYLE_2, STYLE_2, EXSTYLE_3 },
    { __LINE__, 1, 1, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_0 },
    { __LINE__, 1, 1, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_1 },
    { __LINE__, 1, 1, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_2 },
    { __LINE__, 1, 1, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_3 },
    { __LINE__, 1, 2, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_0 },
    { __LINE__, 1, 2, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_1 },
    { __LINE__, 1, 1, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_2 },
    { __LINE__, 1, 2, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_3 },
    { __LINE__, 1, 2, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_2, EXSTYLE_0 },
    { __LINE__, 1, 2, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_2, EXSTYLE_1 },
    { __LINE__, 1, 1, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_2, EXSTYLE_2 },
    { __LINE__, 1, 2, TYPE_1, STYLE_1, EXSTYLE_3, STYLE_2, EXSTYLE_3 },
    { __LINE__, 0, 0, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, 0, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, 0, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_1, STYLE_2, EXSTYLE_0, STYLE_2, EXSTYLE_3 },
    { __LINE__, 1, 1, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_0, EXSTYLE_0 },
    { __LINE__, 1, 1, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_0, EXSTYLE_1 },
    { __LINE__, 1, 1, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_0, EXSTYLE_2 },
    { __LINE__, 1, 1, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_0, EXSTYLE_3 },
    { __LINE__, 1, 2, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_1, EXSTYLE_0 },
    { __LINE__, 1, 2, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_1, EXSTYLE_1 },
    { __LINE__, 1, 1, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_1, EXSTYLE_2 },
    { __LINE__, 1, 2, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_1, EXSTYLE_3 },
    { __LINE__, 1, 2, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_2, EXSTYLE_0 },
    { __LINE__, 1, 2, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_2, EXSTYLE_1 },
    { __LINE__, 1, 1, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_2, EXSTYLE_2 },
    { __LINE__, 1, 2, TYPE_1, STYLE_2, EXSTYLE_1, STYLE_2, EXSTYLE_3 },
    { __LINE__, 0, 0, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, 0, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, 0, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_1, STYLE_2, EXSTYLE_2, STYLE_2, EXSTYLE_3 },
    { __LINE__, 1, 1, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_0, EXSTYLE_0 },
    { __LINE__, 1, 1, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_0, EXSTYLE_1 },
    { __LINE__, 1, 1, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_0, EXSTYLE_2 },
    { __LINE__, 1, 1, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_0, EXSTYLE_3 },
    { __LINE__, 1, 2, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_1, EXSTYLE_0 },
    { __LINE__, 1, 2, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_1, EXSTYLE_1 },
    { __LINE__, 1, 1, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_1, EXSTYLE_2 },
    { __LINE__, 1, 2, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_1, EXSTYLE_3 },
    { __LINE__, 1, 2, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_2, EXSTYLE_0 },
    { __LINE__, 1, 2, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_2, EXSTYLE_1 },
    { __LINE__, 1, 1, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_2, EXSTYLE_2 },
    { __LINE__, 1, 2, TYPE_1, STYLE_2, EXSTYLE_3, STYLE_2, EXSTYLE_3 },

    // TYPE_2
    { __LINE__, 0, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_2, STYLE_1, EXSTYLE_0, STYLE_2, EXSTYLE_3 },
    { __LINE__, 1, 1, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_0 },
    { __LINE__, 1, 1, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_1 },
    { __LINE__, 1, 1, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_2 },
    { __LINE__, 1, 1, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_0, EXSTYLE_3 },
    { __LINE__, 1, 2, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_0 },
    { __LINE__, 1, 2, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_1 },
    { __LINE__, 1, 1, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_2 },
    { __LINE__, 1, 2, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_1, EXSTYLE_3 },
    { __LINE__, 1, 2, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_2, EXSTYLE_0 },
    { __LINE__, 1, 2, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_2, EXSTYLE_1 },
    { __LINE__, 1, 1, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_2, EXSTYLE_2 },
    { __LINE__, 1, 2, TYPE_2, STYLE_1, EXSTYLE_1, STYLE_2, EXSTYLE_3 },
    { __LINE__, 0, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_2, STYLE_1, EXSTYLE_2, STYLE_2, EXSTYLE_3 },
    { __LINE__, 1, 1, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_0 },
    { __LINE__, 1, 1, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_1 },
    { __LINE__, 1, 1, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_2 },
    { __LINE__, 1, 1, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_0, EXSTYLE_3 },
    { __LINE__, 1, 2, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_0 },
    { __LINE__, 1, 2, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_1 },
    { __LINE__, 1, 1, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_2 },
    { __LINE__, 1, 2, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_1, EXSTYLE_3 },
    { __LINE__, 1, 2, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_2, EXSTYLE_0 },
    { __LINE__, 1, 2, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_2, EXSTYLE_1 },
    { __LINE__, 1, 1, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_2, EXSTYLE_2 },
    { __LINE__, 1, 2, TYPE_2, STYLE_1, EXSTYLE_3, STYLE_2, EXSTYLE_3 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_0, STYLE_2, EXSTYLE_3 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_1, STYLE_2, EXSTYLE_3 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_2, STYLE_2, EXSTYLE_3 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_0, EXSTYLE_0 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_0, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_0, EXSTYLE_2 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_0, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_1, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_1, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_1, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_1, EXSTYLE_3 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_2, EXSTYLE_0 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_2, EXSTYLE_1 },
    { __LINE__, 0, 0, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_2, EXSTYLE_2 },
    { __LINE__, 0, 1, TYPE_2, STYLE_2, EXSTYLE_3, STYLE_2, EXSTYLE_3 },
};

static HHOOK s_hShellHook = NULL;
static UINT s_cCreated = 0;
static UINT s_cDestroyed = 0;
static WCHAR s_szName[] = L"ReactOS ShellHook testcase";
static HWND s_hwndParent = NULL;
static HWND s_hwndTarget = NULL;
static HWND s_hwndMain = NULL;

static HWND
DoCreateWindow(HWND hwndParent, DWORD style, DWORD exstyle, BOOL bFullscreen = FALSE)
{
    INT x = CW_USEDEFAULT, y = CW_USEDEFAULT, cx = 300, cy = 100;
    if (bFullscreen)
    {
        x = y = 0;
        cx = GetSystemMetrics(SM_CXSCREEN);
        cy = GetSystemMetrics(SM_CYSCREEN);
    }
    return CreateWindowExW(exstyle, s_szName, s_szName, style, x, y, cx, cy,
                           hwndParent, NULL, GetModuleHandleW(NULL), NULL);
}

static void DoTestEntryPart1(const TEST_ENTRY *pEntry)
{
    ok(!pEntry->bIsChild || pEntry->bHasOwner,
       "Line %d: bIsChild && !bHasOwner\n", pEntry->lineno);

    s_hwndParent = NULL;
    if (pEntry->bIsChild || pEntry->bHasOwner)
        s_hwndParent = DoCreateWindow(NULL, pEntry->owner_style, pEntry->owner_exstyle);

    DWORD style = pEntry->style;
    DWORD exstyle = pEntry->exstyle;
    if (pEntry->bIsChild)
        style |= WS_CHILD;
    else
        style &= ~WS_CHILD;

    s_cCreated = s_cDestroyed = 0;
    s_hwndTarget = DoCreateWindow(s_hwndParent, style, exstyle);
}

static void DoTestEntryPart2(const TEST_ENTRY *pEntry)
{
    DestroyWindow(s_hwndTarget);
    s_hwndTarget = NULL;

    if (pEntry->bIsChild || pEntry->bHasOwner)
    {
        DestroyWindow(s_hwndParent);
        s_hwndParent = NULL;
    }

    ok(s_cCreated == pEntry->cCreated,
       "Line %d: cCreated expected %u but was %u\n",
       pEntry->lineno, pEntry->cCreated, s_cCreated);

    ok(s_cDestroyed == pEntry->cDestroyed,
       "Line %d: cDestroyed expected %u but was %u\n",
       pEntry->lineno, pEntry->cDestroyed, s_cDestroyed);
}

static
LRESULT CALLBACK
ShellProc(
    INT nCode,
    WPARAM wParam,
    LPARAM lParam)
{
    if (nCode < 0)
        return CallNextHookEx(s_hShellHook, nCode, wParam, lParam);

    switch (nCode)
    {
        case HSHELL_WINDOWCREATED:
            s_cCreated++;
            break;

        case HSHELL_WINDOWDESTROYED:
            s_cDestroyed++;
            break;
    }

    return CallNextHookEx(s_hShellHook, nCode, wParam, lParam);
}

static INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#define ID_TESTSTART 1000
#define ID_TESTEND 2000
    switch (uMsg)
    {
        case WM_INITDIALOG:
            ok_int(s_cCreated, 0);
            ok_int(s_cDestroyed, 0);
            PostMessageW(hwnd, WM_COMMAND, ID_TESTSTART, 0);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCANCEL:
                    DestroyWindow(hwnd);
                    break;

                default:
                {
                    if (ID_TESTSTART <= LOWORD(wParam) && LOWORD(wParam) < ID_TESTEND)
                    {
                        UINT_PTR i = (UINT_PTR)wParam - ID_TESTSTART;

                        if (i < _countof(s_entries1))
                        {
                            DoTestEntryPart1(&s_entries1[i]);
                            PostMessageW(hwnd, WM_COMMAND, ID_TESTEND + i, 0);
                        }
                        else
                        {
                            DestroyWindow(hwnd);
                        }
                    }
                    else if (ID_TESTEND <= LOWORD(wParam))
                    {
                        UINT_PTR i = (UINT_PTR)wParam - ID_TESTEND;

                        if (i < _countof(s_entries1))
                        {
                            DoTestEntryPart2(&s_entries1[i]);
                            PostMessageW(hwnd, WM_COMMAND, ID_TESTSTART + i + 1, 0);
                        }
                    }
                }
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;
    }

    return 0;
}

static LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            break;
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

START_TEST(ShellHook)
{
    WNDCLASSW wc = { CS_HREDRAW | CS_VREDRAW, WindowProc };
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(INT_PTR)(COLOR_3DFACE + 1);
    wc.lpszClassName = s_szName;
    if (!RegisterClassW(&wc))
    {
        skip("RegisterClassW failed\n");
        return;
    }

    s_hShellHook = SetWindowsHookEx(WH_SHELL, ShellProc, NULL, GetCurrentThreadId());
    if (!s_hShellHook)
    {
        skip("!s_hShellHook\n");
        return;
    }

    s_hwndMain = CreateDialogW(GetModuleHandleW(NULL), L"ShellHook", NULL, DialogProc);
    if (!s_hShellHook)
    {
        skip("!s_hwndMain\n");
        UnhookWindowsHookEx(s_hShellHook);
        return;
    }

    ShowWindow(s_hwndMain, SW_SHOWNOACTIVATE);
    UpdateWindow(s_hwndMain);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (IsDialogMessageW(s_hwndMain, &msg))
            continue;

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnhookWindowsHookEx(s_hShellHook);
}
