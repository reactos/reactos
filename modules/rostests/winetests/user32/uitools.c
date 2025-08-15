/* Unit test suite for user interface functions
 *
 * Copyright 2009 Nikolay Sivov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WINE_NO_INLINE_RECT
#include "wine/test.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

static void test_FillRect(void)
{
    HDC hdc, hdcmem;
    DWORD bits[64];
    HBITMAP hbmp, oldhbmp;
    COLORREF col;
    HBRUSH old_brush;
    RECT r;

    /* fill bitmap data with white */
    memset(bits, 0xff, sizeof(bits));

    hdc = GetDC(0);
    ok( hdc != NULL, "CreateDC rets %p\n", hdc);
    /* create a memory dc */
    hdcmem = CreateCompatibleDC(hdc);
    ok(hdcmem != NULL, "CreateCompatibleDC rets %p\n", hdcmem);
    /* test monochrome bitmap: should always work */
    hbmp = CreateBitmap(32, 32, 1, 1, bits);
    ok(hbmp != NULL, "CreateBitmap returns %p\n", hbmp);
    oldhbmp = SelectObject(hdcmem, hbmp);
    ok(oldhbmp != NULL, "SelectObject returned NULL\n"); /* a memdc always has a bitmap selected */
    col = GetPixel(hdcmem, 0, 0);
    ok( col == 0xffffff, "GetPixel returned %08lx, expected 0xffffff\n", col);

    /* select black brush */
    old_brush = SelectObject(hdcmem, GetStockObject(BLACK_BRUSH));
    SetRect(&r, 0, 0, 5, 5);
    FillRect(hdcmem, &r, 0);
    SelectObject(hdcmem, old_brush);
    /* bitmap filled with last selected brush */
    col = GetPixel(hdcmem, 0, 0);
    ok(col == 0, "GetPixel returned %08lx, expected 0\n", col);

    SelectObject(hdcmem, oldhbmp);
    DeleteObject(hbmp);
    DeleteDC(hdcmem);
    ReleaseDC(0, hdc);
}

static void test_SubtractRect(void)
{
    RECT rect1;
    RECT rect2;
    RECT rectr;
    BOOL result;

    /* source rectangles don't intersect */
    SetRect(&rect1, 50, 50, 150, 100);
    SetRect(&rect2, 250, 200, 1500, 1000);
    result = SubtractRect(&rectr, &rect1, &rect2);
    ok(result, "SubtractRect returned FALSE but subtraction should not be empty\n");
    ok(result && rectr.left == 50 && rectr.top == 50 && rectr.right ==150
       && rectr.bottom == 100, "wrong rect subtraction of SubtractRect (dest rect=%s)\n",
       wine_dbgstr_rect(&rectr));

    /* source rect 2 partially overlaps rect 1 */
    SetRect(&rect1, 2431, 626, 3427, 1608);
    SetRect(&rect2, 2499, 626, 3427, 1608);
    result = SubtractRect(&rectr, &rect1, &rect2);
    ok(result, "SubtractRect returned FALSE but subtraction should not be empty\n");
    ok(result && rectr.left == 2431 && rectr.top == 626 && rectr.right == 2499
       && rectr.bottom == 1608, "wrong rect subtraction of SubtractRect (dest rect=%s)\n",
       wine_dbgstr_rect(&rectr));

    /* source rect 2 partially overlaps rect 1 - dest is src rect 2 */
    SetRect(&rect1, 2431, 626, 3427, 1608);
    SetRect(&rect2, 2499, 626, 3427, 1608);
    result = SubtractRect(&rect2, &rect1, &rect2);
    ok(result, "SubtractRect returned FALSE but subtraction should not be empty\n");
    ok(result && rectr.left == 2431 && rectr.top == 626 && rectr.right == 2499
       && rectr.bottom == 1608, "wrong rect subtraction of SubtractRect (dest rect=%s)\n",
       wine_dbgstr_rect(&rectr));

    /* source rect 2 completely overlaps rect 1 */
    SetRect(&rect1, 250, 250, 400, 500);
    SetRect(&rect2, 50, 50, 1500, 1000);
    result = SubtractRect(&rectr, &rect1, &rect2);
    ok(!result, "SubtractRect returned TRUE but subtraction should be empty (dest rect=%s)\n",
       wine_dbgstr_rect(&rectr));

    /* source rect 2 completely overlaps rect 1 - dest is src rect 2 */
    SetRect(&rect1, 250, 250, 400, 500);
    SetRect(&rect2, 50, 50, 1500, 1000);
    result = SubtractRect(&rect2, &rect1, &rect2);
    ok(!result, "SubtractRect returned TRUE but subtraction should be empty (dest rect=%s)\n",
       wine_dbgstr_rect(&rect2));
}

static void test_EqualRect(void)
{
    RECT rect1, rect2;
    BOOL ret;

    SetRect(&rect1, 0, 0, 0, 0);
    SetRect(&rect2, 1, 1, 1, 1);

    ret = EqualRect(NULL, NULL);
    ok(!ret, "got %d\n", ret);

    ret = EqualRect(&rect1, NULL);
    ok(!ret, "got %d\n", ret);

    ret = EqualRect(NULL, &rect2);
    ok(!ret, "got %d\n", ret);

    ret = EqualRect(&rect1, &rect2);
    ok(!ret, "got %d\n", ret);

    SetRect(&rect1, 0, 0, 10, 10);
    SetRect(&rect2, 10, 10, 0, 0);

    ret = EqualRect(&rect1, &rect2);
    ok(!ret, "got %d\n", ret);

    ret = EqualRect(&rect1, &rect1);
    ok(ret, "got %d\n", ret);

    rect2 = rect1;
    ret = EqualRect(&rect1, &rect2);
    ok(ret, "got %d\n", ret);
}

static void test_IsRectEmpty(void)
{
    BOOL ret;
    unsigned int i;
    static const struct {
        RECT rect;
        BOOL ret;
    } rtest[] = {
        {{0, 0, 0, 0}, TRUE},
        {{127, 131, 127, 131}, TRUE},
        {{MAXLONG, MAXLONG, MAXLONG, MAXLONG}, TRUE},
        {{-1, -1, -1, -1}, TRUE},
        {{-2011, -2017, -2011, -2017}, TRUE},
        {{MINLONG, MINLONG, MINLONG, MINLONG}, TRUE},
        /* Only width or height are 0 */
        {{31, 37, 31, 41}, TRUE},
        {{881, 883, 887, 883}, TRUE},
        {{-1721, 1723, -1721, 7213}, TRUE},
        /* Negative width and/or height */
        {{11, 13, 5, 7}, TRUE},
        {{-11, -13, -19, -23}, TRUE},
        {{11, 13, -17, 19}, TRUE},
        {{11, 13, 17, 11}, TRUE},
        /* Non empty rects */
        {{101, 103, 107, 109}, FALSE},
        {{1, -9, 7, 3}, FALSE},
        {{-109, -107, -103, -101}, FALSE},
    };

    for (i = 0; i < ARRAY_SIZE(rtest); i++) {
        ret = IsRectEmpty(&rtest[i].rect);
        ok(ret == rtest[i].ret, "Test %d: IsRectEmpty returned %s for %s\n", i,
           ret ? "TRUE" : "FALSE", wine_dbgstr_rect(&rtest[i].rect));
    }
}

static void test_SetRect(void)
{
    RECT rect;
    BOOL ret;

    ret = SetRect(NULL, 0, 0, 0, 0);
    ok(!ret, "got %d\n", ret);

    ret = SetRect(&rect, 1, 2, 3, 4);
    ok(ret, "got %d\n", ret);
    ok(rect.left == 1 && rect.top == 2 && rect.right == 3 && rect.bottom == 4,
        "got wrong rectangle\n");

    ret = SetRect(&rect, 10, 10, 5, 5);
    ok(ret, "got %d\n", ret);
    ok(rect.left == 10 && rect.top == 10 && rect.right == 5 && rect.bottom == 5,
        "got wrong rectangle\n");
}

START_TEST(uitools)
{
    test_FillRect();
    test_SubtractRect();
    test_EqualRect();
    test_IsRectEmpty();
    test_SetRect();
}
