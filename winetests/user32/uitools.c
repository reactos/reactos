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
    ok( col == 0xffffff, "GetPixel returned %08x, expected 0xffffff\n", col);

    /* select black brush */
    old_brush = SelectObject(hdcmem, GetStockObject(BLACK_BRUSH));
    r.left  = r.top = 0;
    r.right = r.bottom = 5;
    FillRect(hdcmem, &r, 0);
    SelectObject(hdcmem, old_brush);
    /* bitmap filled with last selected brush */
    col = GetPixel(hdcmem, 0, 0);
    ok(col == 0, "GetPixel returned %08x, expected 0\n", col);

    SelectObject(hdcmem, oldhbmp);
    DeleteObject(hbmp);
    DeleteDC(hdcmem);
    ReleaseDC(0, hdc);
}

START_TEST(uitools)
{
    test_FillRect();
}
