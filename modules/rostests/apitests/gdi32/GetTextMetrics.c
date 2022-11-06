/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for GetTextMetrics and GetTextExtentPoint32
 * COPYRIGHT:   Copyright 2018 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

/* #define EMIT_TESTCASES */

typedef struct TEST_ENTRY
{
    int line;
    LONG lfHeight;
    LONG lfWidth;
    LONG cxText;
    LONG cyText;
    LONG tmHeight;
    LONG tmAscent;
    LONG tmDescent;
    LONG tmInternalLeading;
    LONG tmExternalLeading;
} TEST_ENTRY;

#ifdef EMIT_TESTCASES
static const TEST_ENTRY g_test_entries[] =
{
    { __LINE__, 400, 0 },
    { __LINE__, 300, 0 },
    { __LINE__, 200, 0 },
    { __LINE__, 100, 0 },
    { __LINE__, 64, 0 },
    { __LINE__, 32, 0 },
    { __LINE__, 16, 0 },
    { __LINE__, 15, 0 },
    { __LINE__, 14, 0 },
    { __LINE__, 13, 0 },
    { __LINE__, 12, 0 },
    { __LINE__, 11, 0 },
    { __LINE__, 10, 0 },
    { __LINE__, 9, 0 },
    { __LINE__, 8, 0 },
    { __LINE__, 7, 0 },
    { __LINE__, 6, 0 },
    { __LINE__, 5, 0 },
    { __LINE__, 4, 0 },
    { __LINE__, 3, 0 },
    { __LINE__, 2, 0 },
    { __LINE__, 1, 0 },
    { __LINE__, 0, 0 },
    { __LINE__, -1, 0 },
    { __LINE__, -2, 0 },
    { __LINE__, -3, 0 },
    { __LINE__, -4, 0 },
    { __LINE__, -5, 0 },
    { __LINE__, -6, 0 },
    { __LINE__, -7, 0 },
    { __LINE__, -8, 0 },
    { __LINE__, -9, 0 },
    { __LINE__, -10, 0 },
    { __LINE__, -11, 0 },
    { __LINE__, -12, 0 },
    { __LINE__, -13, 0 },
    { __LINE__, -14, 0 },
    { __LINE__, -15, 0 },
    { __LINE__, -16, 0 },
    { __LINE__, -32, 0 },
    { __LINE__, -64, 0 },
    { __LINE__, -100, 0 },
    { __LINE__, -200, 0 },
    { __LINE__, -300, 0 },
    { __LINE__, -400, 0 },
};
#define g_test_entry_count _countof(g_test_entries)
#else
static const TEST_ENTRY g_FREEMONO[] =
{
    { __LINE__, 400, 0, 3600, 400, 400, 320, 80, 0, 0 },
    { __LINE__, 300, 0, 2700, 300, 300, 240, 60, 0, 0 },
    { __LINE__, 200, 0, 1800, 200, 200, 160, 40, 0, 0 },
    { __LINE__, 100, 0, 900, 100, 100, 80, 20, 0, 0 },
    { __LINE__, 64, 0, 570, 64, 64, 51, 13, 0, 0 },
    { __LINE__, 32, 0, 285, 32, 32, 26, 6, 0, 0 },
    { __LINE__, 16, 0, 150, 16, 16, 13, 3, 0, 0 },
    { __LINE__, 15, 0, 135, 15, 15, 12, 3, 0, 0 },
    { __LINE__, 14, 0, 120, 14, 14, 11, 3, 0, 0 },
    { __LINE__, 13, 0, 120, 13, 13, 10, 3, 0, 0 },
    { __LINE__, 12, 0, 105, 12, 12, 10, 2, 0, 0 },
    { __LINE__, 11, 0, 105, 11, 11, 9, 2, 0, 0 },
    { __LINE__, 10, 0, 90, 10, 10, 8, 2, 0, 0 },
    { __LINE__, 9, 0, 77, 9, 9, 7, 2, 0, 0 },
    { __LINE__, 8, 0, 75, 8, 8, 6, 2, 0, 0 },
    { __LINE__, 7, 0, 60, 7, 7, 6, 1, 0, 0 },
    { __LINE__, 6, 0, 60, 6, 6, 5, 1, 0, 0 },
    { __LINE__, 5, 0, 45, 5, 5, 4, 1, 0, 0 },
    { __LINE__, 4, 0, 30, 4, 4, 3, 1, 0, 0 },
    { __LINE__, 3, 0, 30, 3, 3, 2, 1, 0, 0 },
    { __LINE__, 2, 0, 15, 2, 2, 2, 0, 0, 0 },
    { __LINE__, 1, 0, 15, 2, 2, 2, 0, 0, 0 },
    { __LINE__, 0, 0, -165, -18, -18, -14, -4, 0, 0 },
    { __LINE__, -1, 0, 15, 2, 2, 2, 0, 0, 0 },
    { __LINE__, -2, 0, 15, 2, 2, 2, 0, 0, 0 },
    { __LINE__, -3, 0, 30, 3, 3, 2, 1, 0, 0 },
    { __LINE__, -4, 0, 30, 4, 4, 3, 1, 0, 0 },
    { __LINE__, -5, 0, 45, 5, 5, 4, 1, 0, 0 },
    { __LINE__, -6, 0, 60, 6, 6, 5, 1, 0, 0 },
    { __LINE__, -7, 0, 60, 7, 7, 6, 1, 0, 0 },
    { __LINE__, -8, 0, 75, 8, 8, 6, 2, 0, 0 },
    { __LINE__, -9, 0, 77, 9, 9, 7, 2, 0, 0 },
    { __LINE__, -10, 0, 90, 10, 10, 8, 2, 0, 0 },
    { __LINE__, -11, 0, 105, 11, 11, 9, 2, 0, 0 },
    { __LINE__, -12, 0, 105, 12, 12, 10, 2, 0, 0 },
    { __LINE__, -13, 0, 120, 13, 13, 10, 3, 0, 0 },
    { __LINE__, -14, 0, 120, 14, 14, 11, 3, 0, 0 },
    { __LINE__, -15, 0, 135, 15, 15, 12, 3, 0, 0 },
    { __LINE__, -16, 0, 150, 16, 16, 13, 3, 0, 0 },
    { __LINE__, -32, 0, 285, 32, 32, 26, 6, 0, 0 },
    { __LINE__, -64, 0, 570, 64, 64, 51, 13, 0, 0 },
    { __LINE__, -100, 0, 900, 100, 100, 80, 20, 0, 0 },
    { __LINE__, -200, 0, 1800, 200, 200, 160, 40, 0, 0 },
    { __LINE__, -300, 0, 2700, 300, 300, 240, 60, 0, 0 },
    { __LINE__, -400, 0, 3600, 400, 400, 320, 80, 0, 0 },
};
#define g_FREEMONO_count _countof(g_FREEMONO)

static const TEST_ENTRY g_MSGOTHIC[] =
{
    { __LINE__, 400, 0, 3000, 400, 400, 344, 56, 0, 0 },
    { __LINE__, 300, 0, 2250, 300, 300, 258, 42, 0, 0 },
    { __LINE__, 200, 0, 1500, 200, 200, 172, 28, 0, 0 },
    { __LINE__, 100, 0, 750, 100, 100, 86, 14, 0, 0 },
    { __LINE__, 64, 0, 480, 64, 64, 55, 9, 0, 0 },
    { __LINE__, 32, 0, 240, 33, 33, 28, 5, 1, 0 },
    { __LINE__, 16, 0, 120, 16, 16, 14, 2, 0, 0 },
    { __LINE__, 15, 0, 120, 15, 15, 13, 2, 0, 0 },
    { __LINE__, 14, 0, 105, 14, 14, 12, 2, 0, 0 },
    { __LINE__, 13, 0, 105, 13, 13, 11, 2, 0, 0 },
    { __LINE__, 12, 0, 90, 12, 12, 10, 2, 0, 0 },
    { __LINE__, 11, 0, 90, 11, 11, 9, 2, 0, 0 },
    { __LINE__, 10, 0, 75, 10, 10, 9, 1, 0, 0 },
    { __LINE__, 9, 0, 75, 9, 9, 8, 1, 0, 0 },
    { __LINE__, 8, 0, 60, 8, 8, 7, 1, 0, 0 },
    { __LINE__, 7, 0, 60, 7, 7, 6, 1, 0, 0 },
    { __LINE__, 6, 0, 45, 6, 6, 5, 1, 0, 0 },
    { __LINE__, 5, 0, 45, 5, 5, 4, 1, 0, 0 },
    { __LINE__, 4, 0, 30, 4, 4, 3, 1, 0, 0 },
    { __LINE__, 3, 0, 30, 3, 3, 3, 0, 0, 0 },
    { __LINE__, 2, 0, 15, 2, 2, 2, 0, 0, 0 },
    { __LINE__, 1, 0, 15, 2, 2, 2, 0, 0, 0 },
    { __LINE__, 0, 0, -135, -18, -18, -15, -3, 0, 0 },
    { __LINE__, -1, 0, 15, 2, 2, 2, 0, 0, 0 },
    { __LINE__, -2, 0, 15, 2, 2, 2, 0, 0, 0 },
    { __LINE__, -3, 0, 30, 3, 3, 3, 0, 0, 0 },
    { __LINE__, -4, 0, 30, 4, 4, 3, 1, 0, 0 },
    { __LINE__, -5, 0, 45, 5, 5, 4, 1, 0, 0 },
    { __LINE__, -6, 0, 45, 6, 6, 5, 1, 0, 0 },
    { __LINE__, -7, 0, 60, 7, 7, 6, 1, 0, 0 },
    { __LINE__, -8, 0, 60, 8, 8, 7, 1, 0, 0 },
    { __LINE__, -9, 0, 75, 9, 9, 8, 1, 0, 0 },
    { __LINE__, -10, 0, 75, 10, 10, 9, 1, 0, 0 },
    { __LINE__, -11, 0, 90, 11, 11, 9, 2, 0, 0 },
    { __LINE__, -12, 0, 90, 12, 12, 10, 2, 0, 0 },
    { __LINE__, -13, 0, 105, 13, 13, 11, 2, 0, 0 },
    { __LINE__, -14, 0, 105, 14, 14, 12, 2, 0, 0 },
    { __LINE__, -15, 0, 120, 15, 15, 13, 2, 0, 0 },
    { __LINE__, -16, 0, 120, 16, 16, 14, 2, 0, 0 },
    { __LINE__, -32, 0, 240, 33, 33, 28, 5, 1, 0 },
    { __LINE__, -64, 0, 480, 64, 64, 55, 9, 0, 0 },
    { __LINE__, -100, 0, 750, 100, 100, 86, 14, 0, 0 },
    { __LINE__, -200, 0, 1500, 200, 200, 172, 28, 0, 0 },
    { __LINE__, -300, 0, 2250, 300, 300, 258, 42, 0, 0 },
    { __LINE__, -400, 0, 3000, 400, 400, 344, 56, 0, 0 },
};
#define g_MSGOTHIC_count _countof(g_MSGOTHIC)

static const TEST_ENTRY g_MSMINCHO[] =
{
    { __LINE__, 400, 0, 3000, 400, 400, 344, 56, 0, 0 },
    { __LINE__, 300, 0, 2250, 300, 300, 258, 42, 0, 0 },
    { __LINE__, 200, 0, 1500, 200, 200, 172, 28, 0, 0 },
    { __LINE__, 100, 0, 750, 100, 100, 86, 14, 0, 0 },
    { __LINE__, 64, 0, 480, 64, 64, 55, 9, 0, 0 },
    { __LINE__, 32, 0, 240, 33, 33, 28, 5, 1, 0 },
    { __LINE__, 16, 0, 120, 16, 16, 14, 2, 0, 0 },
    { __LINE__, 15, 0, 120, 15, 15, 13, 2, 0, 0 },
    { __LINE__, 14, 0, 105, 14, 14, 12, 2, 0, 0 },
    { __LINE__, 13, 0, 105, 13, 13, 11, 2, 0, 0 },
    { __LINE__, 12, 0, 90, 12, 12, 10, 2, 0, 0 },
    { __LINE__, 11, 0, 90, 11, 11, 9, 2, 0, 0 },
    { __LINE__, 10, 0, 75, 10, 10, 9, 1, 0, 0 },
    { __LINE__, 9, 0, 75, 9, 9, 8, 1, 0, 0 },
    { __LINE__, 8, 0, 60, 8, 8, 7, 1, 0, 0 },
    { __LINE__, 7, 0, 60, 7, 7, 6, 1, 0, 0 },
    { __LINE__, 6, 0, 45, 6, 6, 5, 1, 0, 0 },
    { __LINE__, 5, 0, 45, 5, 5, 4, 1, 0, 0 },
    { __LINE__, 4, 0, 30, 4, 4, 3, 1, 0, 0 },
    { __LINE__, 3, 0, 30, 3, 3, 3, 0, 0, 0 },
    { __LINE__, 2, 0, 15, 2, 2, 2, 0, 0, 0 },
    { __LINE__, 1, 0, 15, 2, 2, 2, 0, 0, 0 },
    { __LINE__, 0, 0, -135, -18, -18, -15, -3, 0, 0 },
    { __LINE__, -1, 0, 15, 2, 2, 2, 0, 0, 0 },
    { __LINE__, -2, 0, 15, 2, 2, 2, 0, 0, 0 },
    { __LINE__, -3, 0, 30, 3, 3, 3, 0, 0, 0 },
    { __LINE__, -4, 0, 30, 4, 4, 3, 1, 0, 0 },
    { __LINE__, -5, 0, 45, 5, 5, 4, 1, 0, 0 },
    { __LINE__, -6, 0, 45, 6, 6, 5, 1, 0, 0 },
    { __LINE__, -7, 0, 60, 7, 7, 6, 1, 0, 0 },
    { __LINE__, -8, 0, 60, 8, 8, 7, 1, 0, 0 },
    { __LINE__, -9, 0, 75, 9, 9, 8, 1, 0, 0 },
    { __LINE__, -10, 0, 75, 10, 10, 9, 1, 0, 0 },
    { __LINE__, -11, 0, 90, 11, 11, 9, 2, 0, 0 },
    { __LINE__, -12, 0, 90, 12, 12, 10, 2, 0, 0 },
    { __LINE__, -13, 0, 105, 13, 13, 11, 2, 0, 0 },
    { __LINE__, -14, 0, 105, 14, 14, 12, 2, 0, 0 },
    { __LINE__, -15, 0, 120, 15, 15, 13, 2, 0, 0 },
    { __LINE__, -16, 0, 120, 16, 16, 14, 2, 0, 0 },
    { __LINE__, -32, 0, 240, 33, 33, 28, 5, 1, 0 },
    { __LINE__, -64, 0, 480, 64, 64, 55, 9, 0, 0 },
    { __LINE__, -100, 0, 750, 100, 100, 86, 14, 0, 0 },
    { __LINE__, -200, 0, 1500, 200, 200, 172, 28, 0, 0 },
    { __LINE__, -300, 0, 2250, 300, 300, 258, 42, 0, 0 },
    { __LINE__, -400, 0, 3000, 400, 400, 344, 56, 0, 0 },
};
#define g_MSMINCHO_count _countof(g_MSMINCHO)
#endif

typedef struct FONT_ENTRY
{
    const char *entry_name;
    const char *font_name;
    const char *font_file;
    size_t test_count;
    const TEST_ENTRY *tests;
} FONT_ENTRY;

static FONT_ENTRY g_font_entries[] =
{
#ifdef EMIT_TESTCASES
    { "FREEMONO", "FreeMono", "FreeMono.ttf" },
    { "MSGOTHIC", "MS Gothic", "msgothic.ttc" },
    { "MSMINCHO", "MS Mincho", "msmincho.ttc" },
#else
    { "FREEMONO", "FreeMono", "FreeMono.ttf", g_FREEMONO_count, g_FREEMONO },
    { "MSGOTHIC", "MS Gothic", "msgothic.ttc", g_MSGOTHIC_count, g_MSGOTHIC },
    { "MSMINCHO", "MS Mincho", "msmincho.ttc", g_MSMINCHO_count, g_MSMINCHO },
#endif
};
static size_t g_font_entry_count = _countof(g_font_entries);

START_TEST(GetTextMetrics)
{
    size_t i, k;
    LOGFONTA lf;
    HFONT hFont;
    HDC hDC;
    HGDIOBJ hFontOld;
    SIZE siz;
    TEXTMETRIC tm;
    char szPath[MAX_PATH];
    static const char *text = "This is a test.";

    hDC = CreateCompatibleDC(NULL);
    for (i = 0; i < g_font_entry_count; ++i)
    {
        FONT_ENTRY *font = &g_font_entries[i];
        ZeroMemory(&lf, sizeof(lf));
        lf.lfCharSet = DEFAULT_CHARSET;
        lstrcpyA(lf.lfFaceName, font->font_name);

        GetWindowsDirectoryA(szPath, MAX_PATH);
        lstrcatA(szPath, "\\Fonts\\");
        lstrcatA(szPath, font->font_file);
        if (GetFileAttributesA(szPath) == 0xFFFFFFFF)
        {
            skip("Font file '%s' doesn't exists\n", font->font_file);
            continue;
        }

#ifdef EMIT_TESTCASES
        printf("static const TEST_ENTRY g_%s[] =\n", font->entry_name);
        printf("{\n");
        for (k = 0; k < g_test_entry_count; ++k)
        {
            const TEST_ENTRY *test = &g_test_entries[k];

            lf.lfHeight = test->lfHeight;
            lf.lfWidth = test->lfWidth;

            hFont = CreateFontIndirectA(&lf);
            hFontOld = SelectObject(hDC, hFont);
            {
                GetTextExtentPoint32A(hDC, text, lstrlenA(text), &siz);
                GetTextMetrics(hDC, &tm);
            }
            SelectObject(hDC, hFontOld);
            DeleteObject(hFont);

            printf("    { __LINE__, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld, %ld },\n",
                lf.lfHeight, lf.lfWidth,
                siz.cx, siz.cy,
                tm.tmHeight, tm.tmAscent, tm.tmDescent, tm.tmInternalLeading, tm.tmExternalLeading);
        }
        printf("};\n");
        printf("#define g_%s_count _countof(g_%s)\n\n", font->entry_name, font->entry_name);
#else
        for (k = 0; k < font->test_count; ++k)
        {
            const TEST_ENTRY *test = &font->tests[k];

            lf.lfHeight = test->lfHeight;
            lf.lfWidth = test->lfWidth;

            hFont = CreateFontIndirectA(&lf);
            hFontOld = SelectObject(hDC, hFont);
            {
                GetTextExtentPoint32A(hDC, text, lstrlenA(text), &siz);
                GetTextMetrics(hDC, &tm);
            }
            SelectObject(hDC, hFontOld);
            DeleteObject(hFont);

            if (test->cxText > 0)
            {
                ok_(__FILE__, test->line)(labs(test->cxText - siz.cx) <= 1, "%s (%ld): cxText: labs(%ld - %ld) > 1\n", font->entry_name, test->lfHeight, test->cxText, siz.cx);
                ok_(__FILE__, test->line)(labs(test->cxText - siz.cx) == 0, "%s (%ld): cxText: labs(%ld - %ld) != 0\n", font->entry_name, test->lfHeight, test->cxText, siz.cx);
            }
            if (test->cyText > 0)
            {
                ok_(__FILE__, test->line)(labs(test->cyText - siz.cy) <= 1, "%s (%ld): cyText: labs(%ld - %ld) > 1\n", font->entry_name, test->lfHeight, test->cyText, siz.cy);
                ok_(__FILE__, test->line)(labs(test->cyText - siz.cy) == 0, "%s (%ld): cyText: labs(%ld - %ld) != 0\n", font->entry_name, test->lfHeight, test->cyText, siz.cy);
            }
            if (test->tmHeight > 0)
            {
                ok_(__FILE__, test->line)(labs(test->tmHeight - tm.tmHeight) <= 1, "%s (%ld): tmHeight: labs(%ld - %ld) > 1\n", font->entry_name, test->lfHeight, test->tmHeight, tm.tmHeight);
                ok_(__FILE__, test->line)(labs(test->tmHeight - tm.tmHeight) == 0, "%s (%ld): tmHeight: labs(%ld - %ld) != 0\n", font->entry_name, test->lfHeight, test->tmHeight, tm.tmHeight);
            }
            if (test->tmAscent > 0)
            {
                ok_(__FILE__, test->line)(labs(test->tmAscent - tm.tmAscent) <= 1, "%s (%ld): tmAscent: labs(%ld - %ld) > 1\n", font->entry_name, test->lfHeight, test->tmAscent, tm.tmAscent);
                ok_(__FILE__, test->line)(labs(test->tmAscent - tm.tmAscent) == 0, "%s (%ld): tmAscent: labs(%ld - %ld) != 0\n", font->entry_name, test->lfHeight, test->tmAscent, tm.tmAscent);
            }
            if (test->tmDescent > 0)
            {
                ok_(__FILE__, test->line)(labs(test->tmDescent - tm.tmDescent) <= 1, "%s (%ld): tmDescent: labs(%ld - %ld) > 1\n", font->entry_name, test->lfHeight, test->tmDescent, tm.tmDescent);
                ok_(__FILE__, test->line)(labs(test->tmDescent - tm.tmDescent) == 0, "%s (%ld): tmDescent: labs(%ld - %ld) != 0\n", font->entry_name, test->lfHeight, test->tmDescent, tm.tmDescent);
            }
            if (test->tmInternalLeading > 0)
            {
                ok_(__FILE__, test->line)(labs(test->tmInternalLeading - tm.tmInternalLeading) <= 1, "%s (%ld): tmInternalLeading: labs(%ld - %ld) > 1\n", font->entry_name, test->lfHeight, test->tmInternalLeading, tm.tmInternalLeading);
                ok_(__FILE__, test->line)(labs(test->tmInternalLeading - tm.tmInternalLeading) == 0, "%s (%ld): tmInternalLeading: labs(%ld - %ld) != 0\n", font->entry_name, test->lfHeight, test->tmInternalLeading, tm.tmInternalLeading);
            }
            if (test->tmExternalLeading > 0)
            {
                ok_(__FILE__, test->line)(labs(test->tmExternalLeading - tm.tmExternalLeading) <= 1, "%s (%ld): tmExternalLeading: labs(%ld - %ld) > 1\n", font->entry_name, test->lfHeight, test->tmExternalLeading, tm.tmExternalLeading);
                ok_(__FILE__, test->line)(labs(test->tmExternalLeading - tm.tmExternalLeading) == 0, "%s (%ld): tmExternalLeading: labs(%ld - %ld) != 0\n", font->entry_name, test->lfHeight, test->tmExternalLeading, tm.tmExternalLeading);
            }
        }
#endif
    }
    DeleteDC(hDC);
}
