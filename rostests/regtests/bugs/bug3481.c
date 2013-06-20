/*
 * PROJECT:         ReactOS bug regression tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            rostests/regtests/bugs/bug3481.c
 * PURPOSE:         Test for bug 3481
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>

#define COUNT 26

void Test_bug3481()
{
    const char text[COUNT] = "abcdefghijklmnopqrstuvmxyz";
    WORD agi[COUNT];
    INT i, aiWidth1[COUNT], aiWidth2[COUNT];
    BOOL result;
    HDC hdc;
    SIZE size1, size2;

    /* Create a DC */
    hdc = CreateCompatibleDC(NULL);

    SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));

    /* Convert the charcaters into glyph indices */
    result = GetGlyphIndicesA(hdc, text, COUNT, agi, 0);
    ok(result != 0, "result=%d, GetLastError()=%ld\n", result, GetLastError());

    /* Get the size of the string */
    result = GetTextExtentPoint32A(hdc, text, COUNT, &size1);
    ok(result != 0, "result=%d, GetLastError()=%ld\n", result, GetLastError());

    /* Get the size from glyph indices */
    result = GetTextExtentPointI(hdc, agi, COUNT, &size2);
    ok(result != 0, "result=%d, GetLastError()=%ld\n", result, GetLastError());

    /* Compare sizes */
    ok(size1.cx == size2.cx, "Sizes don't match. size1.cx=%ld, size2.cx=%ld\n", size1.cx, size2.cx);
    ok(size1.cy == size2.cy, "Sizes don't match. size1.cy=%ld, size2.cy=%ld\n", size1.cy, size2.cy);

    /* Get the size of the string */
    result = GetTextExtentExPointA(hdc, text, COUNT, MAXLONG, NULL, aiWidth1, &size1);
    ok(result != 0, "result=%d, GetLastError()=%ld\n", result, GetLastError());

    /* Get the size from glyph indices */
    result = GetTextExtentExPointI(hdc, agi, COUNT, MAXLONG, NULL, aiWidth2, &size2);
    ok(result != 0, "result=%d, GetLastError()=%ld\n", result, GetLastError());

    /* Compare sizes */
    ok(size1.cx == size2.cx, "Sizes don't match. size1.cx=%ld, size2.cx=%ld\n", size1.cx, size2.cx);
    ok(size1.cy == size2.cy, "Sizes don't match. size1.cy=%ld, size2.cy=%ld\n", size1.cy, size2.cy);

    /* Loop all characters */
    for (i = 0; i < COUNT; i++)
    {
        /* Check if we got identical spacing values */
        ok(aiWidth1[i] == aiWidth2[i], "wrong spacing, i=%d, char:%d, index:%d\n", i, aiWidth1[i], aiWidth2[i]);
    }

    /* Cleanup */
    DeleteDC(hdc);
}

START_TEST(bug3481)
{
    Test_bug3481();
}

