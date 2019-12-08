/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test for DrawText
 * COPYRIGHT:   Copyright 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

typedef struct YY
{
    LONG top;
    LONG bottom;
} YY;

typedef struct TEST_ENTRY
{
    INT line;
    INT ret;
    LONG font_height;
    YY input;
    YY output;
} TEST_ENTRY;

static const TEST_ENTRY s_entries[] =
{
    { __LINE__, 5, -10, { 0, -2 }, { 0, 5 } },
    { __LINE__, 6, -10, { 0, -1 }, { 0, 6 } },
    { __LINE__, 6, -10, { 0, 0 }, { 0, 6 } },
    { __LINE__, 7, -10, { 0, 1 }, { 0, 7 } },
    { __LINE__, 7, -10, { 0, 2 }, { 0, 7 } },
    { __LINE__, 8, -10, { 0, 3 }, { 0, 8 } },
    { __LINE__, 8, -10, { 0, 4 }, { 0, 8 } },
    { __LINE__, 9, -10, { 0, 5 }, { 0, 9 } },
    { __LINE__, 9, -10, { 0, 6 }, { 0, 9 } },
    { __LINE__, 10, -10, { 0, 7 }, { 0, 10 } },
    { __LINE__, 10, -10, { 0, 8 }, { 0, 10 } },
    { __LINE__, 11, -10, { 0, 9 }, { 0, 11 } },
    { __LINE__, 11, -10, { 0, 10 }, { 0, 11 } },
    { __LINE__, 12, -10, { 0, 11 }, { 0, 12 } },
    { __LINE__, 12, -10, { 0, 12 }, { 0, 12 } },
    { __LINE__, 12, -10, { 0, 13 }, { 0, 12 } },
    { __LINE__, 13, -10, { 0, 14 }, { 0, 13 } },
    { __LINE__, 13, -10, { 0, 15 }, { 0, 13 } },

    { __LINE__, 5, -10, { 1, -2 }, { 1, 6 } },
    { __LINE__, 5, -10, { 1, -1 }, { 1, 6 } },
    { __LINE__, 6, -10, { 1, 0 }, { 1, 7 } },
    { __LINE__, 6, -10, { 1, 1 }, { 1, 7 } },
    { __LINE__, 7, -10, { 1, 2 }, { 1, 8 } },
    { __LINE__, 7, -10, { 1, 3 }, { 1, 8 } },
    { __LINE__, 8, -10, { 1, 4 }, { 1, 9 } },
    { __LINE__, 8, -10, { 1, 5 }, { 1, 9 } },
    { __LINE__, 9, -10, { 1, 6 }, { 1, 10 } },
    { __LINE__, 9, -10, { 1, 7 }, { 1, 10 } },
    { __LINE__, 10, -10, { 1, 8 }, { 1, 11 } },
    { __LINE__, 10, -10, { 1, 9 }, { 1, 11 } },
    { __LINE__, 11, -10, { 1, 10 }, { 1, 12 } },
    { __LINE__, 11, -10, { 1, 11 }, { 1, 12 } },
    { __LINE__, 12, -10, { 1, 12 }, { 1, 13 } },
    { __LINE__, 12, -10, { 1, 13 }, { 1, 13 } },
    { __LINE__, 12, -10, { 1, 14 }, { 1, 13 } },
    { __LINE__, 13, -10, { 1, 15 }, { 1, 14 } },

    { __LINE__, 6, -11, { 0, -2 }, { 0, 6 } },
    { __LINE__, 6, -11, { 0, -1 }, { 0, 6 } },
    { __LINE__, 7, -11, { 0, 0 }, { 0, 7 } },
    { __LINE__, 7, -11, { 0, 1 }, { 0, 7 } },
    { __LINE__, 8, -11, { 0, 2 }, { 0, 8 } },
    { __LINE__, 8, -11, { 0, 3 }, { 0, 8 } },
    { __LINE__, 9, -11, { 0, 4 }, { 0, 9 } },
    { __LINE__, 9, -11, { 0, 5 }, { 0, 9 } },
    { __LINE__, 10, -11, { 0, 6 }, { 0, 10 } },
    { __LINE__, 10, -11, { 0, 7 }, { 0, 10 } },
    { __LINE__, 11, -11, { 0, 8 }, { 0, 11 } },
    { __LINE__, 11, -11, { 0, 9 }, { 0, 11 } },
    { __LINE__, 12, -11, { 0, 10 }, { 0, 12 } },
    { __LINE__, 12, -11, { 0, 11 }, { 0, 12 } },
    { __LINE__, 13, -11, { 0, 12 }, { 0, 13 } },
    { __LINE__, 13, -11, { 0, 13 }, { 0, 13 } },
    { __LINE__, 13, -11, { 0, 13 }, { 0, 13 } },
    { __LINE__, 14, -11, { 0, 15 }, { 0, 14 } },

    { __LINE__, 5, -11, { 1, -2 }, { 1, 6 } },
    { __LINE__, 6, -11, { 1, -1 }, { 1, 7 } },
    { __LINE__, 6, -11, { 1, 0 }, { 1, 7 } },
    { __LINE__, 7, -11, { 1, 1 }, { 1, 8 } },
    { __LINE__, 7, -11, { 1, 2 }, { 1, 8 } },
    { __LINE__, 8, -11, { 1, 3 }, { 1, 9 } },
    { __LINE__, 8, -11, { 1, 4 }, { 1, 9 } },
    { __LINE__, 9, -11, { 1, 5 }, { 1, 10 } },
    { __LINE__, 9, -11, { 1, 6 }, { 1, 10 } },
    { __LINE__, 10, -11, { 1, 7 }, { 1, 11 } },
    { __LINE__, 10, -11, { 1, 8 }, { 1, 11 } },
    { __LINE__, 11, -11, { 1, 9 }, { 1, 12 } },
    { __LINE__, 11, -11, { 1, 10 }, { 1, 12 } },
    { __LINE__, 12, -11, { 1, 11 }, { 1, 13 } },
    { __LINE__, 12, -11, { 1, 12 }, { 1, 13 } },
    { __LINE__, 13, -11, { 1, 13 }, { 1, 14 } },
    { __LINE__, 13, -11, { 1, 14 }, { 1, 14 } },
    { __LINE__, 13, -11, { 1, 15 }, { 1, 14 } },

    { __LINE__, 6, -12, { 0, -2 }, { 0, 6 } },
    { __LINE__, 7, -12, { 0, -1 }, { 0, 7 } },
    { __LINE__, 7, -12, { 0, 0 }, { 0, 7 } },
    { __LINE__, 8, -12, { 0, 1 }, { 0, 8 } },
    { __LINE__, 8, -12, { 0, 2 }, { 0, 8 } },
    { __LINE__, 9, -12, { 0, 3 }, { 0, 9 } },
    { __LINE__, 9, -12, { 0, 4 }, { 0, 9 } },
    { __LINE__, 10, -12, { 0, 5 }, { 0, 10 } },
    { __LINE__, 10, -12, { 0, 6 }, { 0, 10 } },
    { __LINE__, 11, -12, { 0, 7 }, { 0, 11 } },
    { __LINE__, 11, -12, { 0, 8 }, { 0, 11 } },
    { __LINE__, 12, -12, { 0, 9 }, { 0, 12 } },
    { __LINE__, 12, -12, { 0, 10 }, { 0, 12 } },
    { __LINE__, 13, -12, { 0, 11 }, { 0, 13 } },
    { __LINE__, 13, -12, { 0, 12 }, { 0, 13 } },
    { __LINE__, 14, -12, { 0, 13 }, { 0, 14 } },
    { __LINE__, 14, -12, { 0, 14 }, { 0, 14 } },
    { __LINE__, 14, -12, { 0, 15 }, { 0, 14 } },

    { __LINE__, 6, -12, { 1, -2 }, { 1, 7 } },
    { __LINE__, 6, -12, { 1, -1 }, { 1, 7 } },
    { __LINE__, 7, -12, { 1, 0 }, { 1, 8 } },
    { __LINE__, 7, -12, { 1, 1 }, { 1, 8 } },
    { __LINE__, 8, -12, { 1, 2 }, { 1, 9 } },
    { __LINE__, 8, -12, { 1, 3 }, { 1, 9 } },
    { __LINE__, 9, -12, { 1, 4 }, { 1, 10 } },
    { __LINE__, 9, -12, { 1, 5 }, { 1, 10 } },
    { __LINE__, 10, -12, { 1, 6 }, { 1, 11 } },
    { __LINE__, 10, -12, { 1, 7 }, { 1, 11 } },
    { __LINE__, 11, -12, { 1, 8 }, { 1, 12 } },
    { __LINE__, 11, -12, { 1, 9 }, { 1, 12 } },
    { __LINE__, 12, -12, { 1, 10 }, { 1, 13 } },
    { __LINE__, 12, -12, { 1, 11 }, { 1, 13 } },
    { __LINE__, 13, -12, { 1, 12 }, { 1, 14 } },
    { __LINE__, 13, -12, { 1, 13 }, { 1, 14 } },
    { __LINE__, 14, -12, { 1, 14 }, { 1, 15 } },
    { __LINE__, 14, -12, { 1, 15 }, { 1, 15 } },
};

static void DoEntry(HDC hdc, const TEST_ENTRY *pEntry)
{
    static const WCHAR szText[] = L"ABCabc123g";
    RECT rc;
    INT ret;
    HFONT hFont;
    HGDIOBJ hFontOld;
    UINT uFormat = DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_CALCRECT;
    LOGFONTW lf = { pEntry->font_height };
    lstrcpyW(lf.lfFaceName, L"Tahoma");

    hFont = CreateFontIndirectW(&lf);
    ok(hFont != NULL, "hFont is NULL\n");

    hFontOld = SelectObject(hdc, hFont);
    {
        SetRect(&rc, 0, pEntry->input.top, 0, pEntry->input.bottom);

        ret = DrawTextW(hdc, szText, lstrlenW(szText), &rc, uFormat);
        ok(ret == pEntry->ret,
           "Line %d: ret %d vs %d\n", pEntry->line, ret, pEntry->ret);

        ok(rc.top == pEntry->output.top,
           "Line %d: top %ld vs %ld\n", pEntry->line, rc.top, pEntry->output.top);

        ok(rc.bottom == pEntry->output.bottom,
           "Line %d: bottom %ld vs %ld\n", pEntry->line, rc.bottom, pEntry->output.bottom);
    }
    SelectObject(hdc, hFontOld);
    DeleteObject(hFont);
}

START_TEST(DrawText)
{
    SIZE_T i;
    HDC hdc = CreateCompatibleDC(NULL);
    ok(hdc != NULL, "hdc was NULL\n");

    for (i = 0; i < ARRAYSIZE(s_entries); ++i)
    {
        DoEntry(hdc, &s_entries[i]);
    }

    DeleteDC(hdc);
}
