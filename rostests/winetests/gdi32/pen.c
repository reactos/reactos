/*
 * Unit test suite for pens
 *
 * Copyright 2006 Dmitry Timoshkov
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

#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/test.h"

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)
#define expect2(expected, alt, got) ok(got == expected || got == alt, \
                                       "Expected %.8x or %.8x, got %.8x\n", expected, alt, got)

static void test_logpen(void)
{
    static const struct
    {
        UINT style;
        INT width;
        COLORREF color;
        UINT ret_style;
        INT ret_width;
        COLORREF ret_color;
    } pen[] = {
        { PS_SOLID, -123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) },
        { PS_SOLID, 0, RGB(0x12,0x34,0x56), PS_SOLID, 0, RGB(0x12,0x34,0x56) },
        { PS_SOLID, 123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) },
        { PS_DASH, 123, RGB(0x12,0x34,0x56), PS_DASH, 123, RGB(0x12,0x34,0x56) },
        { PS_DOT, 123, RGB(0x12,0x34,0x56), PS_DOT, 123, RGB(0x12,0x34,0x56) },
        { PS_DASHDOT, 123, RGB(0x12,0x34,0x56), PS_DASHDOT, 123, RGB(0x12,0x34,0x56) },
        { PS_DASHDOTDOT, 123, RGB(0x12,0x34,0x56), PS_DASHDOTDOT, 123, RGB(0x12,0x34,0x56) },
        { PS_NULL, -123, RGB(0x12,0x34,0x56), PS_NULL, 1, 0 },
        { PS_NULL, 123, RGB(0x12,0x34,0x56), PS_NULL, 1, 0 },
        { PS_INSIDEFRAME, 123, RGB(0x12,0x34,0x56), PS_INSIDEFRAME, 123, RGB(0x12,0x34,0x56) },
        { PS_USERSTYLE, 123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) },
        { PS_ALTERNATE, 123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) }
    };
    INT i, size;
    HPEN hpen;
    LOGPEN lp;
    EXTLOGPEN elp;
    LOGBRUSH lb;
    DWORD obj_type, user_style[2] = { 0xabc, 0xdef };
    struct
    {
        EXTLOGPEN elp;
        DWORD style_data[10];
    } ext_pen;

    for (i = 0; i < sizeof(pen)/sizeof(pen[0]); i++)
    {
        trace("%d: testing style %u\n", i, pen[i].style);

        /********************** cosmetic pens **********************/
        /* CreatePenIndirect behaviour */
        lp.lopnStyle = pen[i].style,
        lp.lopnWidth.x = pen[i].width;
        lp.lopnWidth.y = 11; /* just in case */
        lp.lopnColor = pen[i].color;
        SetLastError(0xdeadbeef);
        hpen = CreatePenIndirect(&lp);
        ok(hpen != 0, "CreatePen error %d\n", GetLastError());

        obj_type = GetObjectType(hpen);
        ok(obj_type == OBJ_PEN, "wrong object type %u\n", obj_type);

        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(lp), &lp);
        ok(size == sizeof(lp), "GetObject returned %d, error %d\n", size, GetLastError());

        ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
        ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %d\n", pen[i].ret_width, lp.lopnWidth.x);
        ok(lp.lopnWidth.y == 0, "expected 0, got %d\n", lp.lopnWidth.y);
        ok(lp.lopnColor == pen[i].ret_color, "expected %08x, got %08x\n", pen[i].ret_color, lp.lopnColor);

        DeleteObject(hpen);

        /* CreatePen behaviour */
        SetLastError(0xdeadbeef);
        hpen = CreatePen(pen[i].style, pen[i].width, pen[i].color);
        ok(hpen != 0, "CreatePen error %d\n", GetLastError());

        obj_type = GetObjectType(hpen);
        ok(obj_type == OBJ_PEN, "wrong object type %u\n", obj_type);

        /* check what's the real size of the object */
        size = GetObject(hpen, 0, NULL);
        ok(size == sizeof(lp), "GetObject returned %d, error %d\n", size, GetLastError());

        /* ask for truncated data */
        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(lp.lopnStyle), &lp);
        ok(!size, "GetObject should fail: size %d, error %d\n", size, GetLastError());

        /* see how larger buffer sizes are handled */
        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(lp) * 2, &lp);
        ok(size == sizeof(lp), "GetObject returned %d, error %d\n", size, GetLastError());

        /* see how larger buffer sizes are handled */
        memset(&elp, 0xb0, sizeof(elp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(elp) * 2, &elp);
        ok(size == sizeof(lp), "GetObject returned %d, error %d\n", size, GetLastError());

        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(lp), &lp);
        ok(size == sizeof(lp), "GetObject returned %d, error %d\n", size, GetLastError());

        ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
        ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %d\n", pen[i].ret_width, lp.lopnWidth.x);
        ok(lp.lopnWidth.y == 0, "expected 0, got %d\n", lp.lopnWidth.y);
        ok(lp.lopnColor == pen[i].ret_color, "expected %08x, got %08x\n", pen[i].ret_color, lp.lopnColor);

        memset(&elp, 0xb0, sizeof(elp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(elp), &elp);

        /* for some reason XP differentiates PS_NULL here */
        if (pen[i].style == PS_NULL)
        {
            ok(hpen == GetStockObject(NULL_PEN), "hpen should be a stock NULL_PEN\n");
            ok(size == sizeof(EXTLOGPEN), "GetObject returned %d, error %d\n", size, GetLastError());
            ok(elp.elpPenStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, elp.elpPenStyle);
            ok(elp.elpWidth == 0, "expected 0, got %u\n", elp.elpWidth);
            ok(elp.elpColor == pen[i].ret_color, "expected %08x, got %08x\n", pen[i].ret_color, elp.elpColor);
            ok(elp.elpBrushStyle == BS_SOLID, "expected BS_SOLID, got %u\n", elp.elpBrushStyle);
            ok(elp.elpHatch == 0, "expected 0, got %p\n", (void *)elp.elpHatch);
            ok(elp.elpNumEntries == 0, "expected 0, got %x\n", elp.elpNumEntries);
        }
        else
        {
            ok(size == sizeof(LOGPEN), "GetObject returned %d, error %d\n", size, GetLastError());
            memcpy(&lp, &elp, sizeof(lp));
            ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
            ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %d\n", pen[i].ret_width, lp.lopnWidth.x);
            ok(lp.lopnWidth.y == 0, "expected 0, got %d\n", lp.lopnWidth.y);
            ok(lp.lopnColor == pen[i].ret_color, "expected %08x, got %08x\n", pen[i].ret_color, lp.lopnColor);
        }

        DeleteObject(hpen);

        /********** cosmetic pens created by ExtCreatePen ***********/
        lb.lbStyle = BS_SOLID;
        lb.lbColor = pen[i].color;
        lb.lbHatch = HS_CROSS; /* just in case */
        SetLastError(0xdeadbeef);
        hpen = ExtCreatePen(pen[i].style, pen[i].width, &lb, 2, user_style);
        if (pen[i].style != PS_USERSTYLE)
        {
            ok(hpen == 0, "ExtCreatePen should fail\n");
            ok(GetLastError() == ERROR_INVALID_PARAMETER,
               "wrong last error value %d\n", GetLastError());
            SetLastError(0xdeadbeef);
            hpen = ExtCreatePen(pen[i].style, pen[i].width, &lb, 0, NULL);
            if (pen[i].style != PS_NULL)
            {
                ok(hpen == 0, "ExtCreatePen with width != 1 should fail\n");
                ok(GetLastError() == ERROR_INVALID_PARAMETER,
                   "wrong last error value %d\n", GetLastError());

                SetLastError(0xdeadbeef);
                hpen = ExtCreatePen(pen[i].style, 1, &lb, 0, NULL);
            }
        }
        else
        {
            ok(hpen == 0, "ExtCreatePen with width != 1 should fail\n");
            ok(GetLastError() == ERROR_INVALID_PARAMETER,
               "wrong last error value %d\n", GetLastError());
            SetLastError(0xdeadbeef);
            hpen = ExtCreatePen(pen[i].style, 1, &lb, 2, user_style);
        }
        if (pen[i].style == PS_INSIDEFRAME)
        {
            /* This style is applicable only for geometric pens */
            ok(hpen == 0, "ExtCreatePen should fail\n");
            goto test_geometric_pens;
        }
        ok(hpen != 0, "ExtCreatePen error %d\n", GetLastError());

        obj_type = GetObjectType(hpen);
        /* for some reason XP differentiates PS_NULL here */
        if (pen[i].style == PS_NULL)
        {
            ok(obj_type == OBJ_PEN, "wrong object type %u\n", obj_type);
            ok(hpen == GetStockObject(NULL_PEN), "hpen should be a stock NULL_PEN\n");
        }
        else
            ok(obj_type == OBJ_EXTPEN, "wrong object type %u\n", obj_type);

        /* check what's the real size of the object */
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, 0, NULL);
        switch (pen[i].style)
        {
        case PS_NULL:
            ok(size == sizeof(LOGPEN),
               "GetObject returned %d, error %d\n", size, GetLastError());
            break;

        case PS_USERSTYLE:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry) + sizeof(user_style),
               "GetObject returned %d, error %d\n", size, GetLastError());
            break;

        default:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry),
               "GetObject returned %d, error %d\n", size, GetLastError());
            break;
        }

        /* ask for truncated data */
        memset(&elp, 0xb0, sizeof(elp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(elp.elpPenStyle), &elp);
        ok(!size, "GetObject should fail: size %d, error %d\n", size, GetLastError());

        /* see how larger buffer sizes are handled */
        memset(&ext_pen, 0xb0, sizeof(ext_pen));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(ext_pen), &ext_pen.elp);
        switch (pen[i].style)
        {
        case PS_NULL:
            ok(size == sizeof(LOGPEN),
               "GetObject returned %d, error %d\n", size, GetLastError());
            memcpy(&lp, &ext_pen.elp, sizeof(lp));
            ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
            ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %d\n", pen[i].ret_width, lp.lopnWidth.x);
            ok(lp.lopnWidth.y == 0, "expected 0, got %d\n", lp.lopnWidth.y);
            ok(lp.lopnColor == pen[i].ret_color, "expected %08x, got %08x\n", pen[i].ret_color, lp.lopnColor);

            /* for PS_NULL it also works this way */
            memset(&elp, 0xb0, sizeof(elp));
            SetLastError(0xdeadbeef);
            size = GetObject(hpen, sizeof(elp), &elp);
            ok(size == sizeof(EXTLOGPEN),
                "GetObject returned %d, error %d\n", size, GetLastError());
            ok(ext_pen.elp.elpHatch == 0xb0b0b0b0, "expected 0xb0b0b0b0, got %p\n", (void *)ext_pen.elp.elpHatch);
            ok(ext_pen.elp.elpNumEntries == 0xb0b0b0b0, "expected 0xb0b0b0b0, got %x\n", ext_pen.elp.elpNumEntries);
            break;

        case PS_USERSTYLE:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry) + sizeof(user_style),
               "GetObject returned %d, error %d\n", size, GetLastError());
            ok(ext_pen.elp.elpHatch == HS_CROSS, "expected HS_CROSS, got %p\n", (void *)ext_pen.elp.elpHatch);
            ok(ext_pen.elp.elpNumEntries == 2, "expected 0, got %x\n", ext_pen.elp.elpNumEntries);
            ok(ext_pen.elp.elpStyleEntry[0] == 0xabc, "expected 0xabc, got %x\n", ext_pen.elp.elpStyleEntry[0]);
            ok(ext_pen.elp.elpStyleEntry[1] == 0xdef, "expected 0xabc, got %x\n", ext_pen.elp.elpStyleEntry[1]);
            break;

        default:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry),
               "GetObject returned %d, error %d\n", size, GetLastError());
            ok(ext_pen.elp.elpHatch == HS_CROSS, "expected HS_CROSS, got %p\n", (void *)ext_pen.elp.elpHatch);
            ok(ext_pen.elp.elpNumEntries == 0, "expected 0, got %x\n", ext_pen.elp.elpNumEntries);
            break;
        }

if (pen[i].style == PS_USERSTYLE)
{
    todo_wine
        ok(ext_pen.elp.elpPenStyle == pen[i].style, "expected %x, got %x\n", pen[i].style, ext_pen.elp.elpPenStyle);
}
else
        ok(ext_pen.elp.elpPenStyle == pen[i].style, "expected %x, got %x\n", pen[i].style, ext_pen.elp.elpPenStyle);
        ok(ext_pen.elp.elpWidth == 1, "expected 1, got %x\n", ext_pen.elp.elpWidth);
        ok(ext_pen.elp.elpColor == pen[i].ret_color, "expected %08x, got %08x\n", pen[i].ret_color, ext_pen.elp.elpColor);
        ok(ext_pen.elp.elpBrushStyle == BS_SOLID, "expected BS_SOLID, got %x\n", ext_pen.elp.elpBrushStyle);

        DeleteObject(hpen);

test_geometric_pens:
        /********************** geometric pens **********************/
        lb.lbStyle = BS_SOLID;
        lb.lbColor = pen[i].color;
        lb.lbHatch = HS_CROSS; /* just in case */
        SetLastError(0xdeadbeef);
        hpen = ExtCreatePen(PS_GEOMETRIC | pen[i].style, pen[i].width, &lb, 2, user_style);
        if (pen[i].style != PS_USERSTYLE)
        {
            ok(hpen == 0, "ExtCreatePen should fail\n");
            SetLastError(0xdeadbeef);
            hpen = ExtCreatePen(PS_GEOMETRIC | pen[i].style, pen[i].width, &lb, 0, NULL);
        }
        if (pen[i].style == PS_ALTERNATE)
        {
            /* This style is applicable only for cosmetic pens */
            ok(hpen == 0, "ExtCreatePen should fail\n");
            continue;
        }
        ok(hpen != 0, "ExtCreatePen error %d\n", GetLastError());

        obj_type = GetObjectType(hpen);
        /* for some reason XP differentiates PS_NULL here */
        if (pen[i].style == PS_NULL)
            ok(obj_type == OBJ_PEN, "wrong object type %u\n", obj_type);
        else
            ok(obj_type == OBJ_EXTPEN, "wrong object type %u\n", obj_type);

        /* check what's the real size of the object */
        size = GetObject(hpen, 0, NULL);
        switch (pen[i].style)
        {
        case PS_NULL:
            ok(size == sizeof(LOGPEN),
               "GetObject returned %d, error %d\n", size, GetLastError());
            break;

        case PS_USERSTYLE:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry) + sizeof(user_style),
               "GetObject returned %d, error %d\n", size, GetLastError());
            break;

        default:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry),
               "GetObject returned %d, error %d\n", size, GetLastError());
            break;
        }

        /* ask for truncated data */
        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(lp.lopnStyle), &lp);
        ok(!size, "GetObject should fail: size %d, error %d\n", size, GetLastError());

        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObject(hpen, sizeof(lp), &lp);
        /* for some reason XP differentiates PS_NULL here */
        if (pen[i].style == PS_NULL)
        {
            ok(size == sizeof(LOGPEN), "GetObject returned %d, error %d\n", size, GetLastError());
            ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
            ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %d\n", pen[i].ret_width, lp.lopnWidth.x);
            ok(lp.lopnWidth.y == 0, "expected 0, got %d\n", lp.lopnWidth.y);
            ok(lp.lopnColor == pen[i].ret_color, "expected %08x, got %08x\n", pen[i].ret_color, lp.lopnColor);
        }
        else
            /* XP doesn't set last error here */
            ok(!size /*&& GetLastError() == ERROR_INVALID_PARAMETER*/,
               "GetObject should fail: size %d, error %d\n", size, GetLastError());

        memset(&ext_pen, 0xb0, sizeof(ext_pen));
        SetLastError(0xdeadbeef);
        /* buffer is too small for user styles */
        size = GetObject(hpen, sizeof(elp), &ext_pen.elp);
        switch (pen[i].style)
        {
        case PS_NULL:
            ok(size == sizeof(EXTLOGPEN),
                "GetObject returned %d, error %d\n", size, GetLastError());
            ok(ext_pen.elp.elpHatch == 0, "expected 0, got %p\n", (void *)ext_pen.elp.elpHatch);
            ok(ext_pen.elp.elpNumEntries == 0, "expected 0, got %x\n", ext_pen.elp.elpNumEntries);

            /* for PS_NULL it also works this way */
            SetLastError(0xdeadbeef);
            size = GetObject(hpen, sizeof(ext_pen), &lp);
            ok(size == sizeof(LOGPEN),
                "GetObject returned %d, error %d\n", size, GetLastError());
            ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
            ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %d\n", pen[i].ret_width, lp.lopnWidth.x);
            ok(lp.lopnWidth.y == 0, "expected 0, got %d\n", lp.lopnWidth.y);
            ok(lp.lopnColor == pen[i].ret_color, "expected %08x, got %08x\n", pen[i].ret_color, lp.lopnColor);
            break;

        case PS_USERSTYLE:
            ok(!size /*&& GetLastError() == ERROR_INVALID_PARAMETER*/,
               "GetObject should fail: size %d, error %d\n", size, GetLastError());
            size = GetObject(hpen, sizeof(ext_pen), &ext_pen.elp);
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry) + sizeof(user_style),
               "GetObject returned %d, error %d\n", size, GetLastError());
            ok(ext_pen.elp.elpHatch == HS_CROSS, "expected HS_CROSS, got %p\n", (void *)ext_pen.elp.elpHatch);
            ok(ext_pen.elp.elpNumEntries == 2, "expected 0, got %x\n", ext_pen.elp.elpNumEntries);
            ok(ext_pen.elp.elpStyleEntry[0] == 0xabc, "expected 0xabc, got %x\n", ext_pen.elp.elpStyleEntry[0]);
            ok(ext_pen.elp.elpStyleEntry[1] == 0xdef, "expected 0xabc, got %x\n", ext_pen.elp.elpStyleEntry[1]);
            break;

        default:
            ok(size == sizeof(EXTLOGPEN) - sizeof(elp.elpStyleEntry),
               "GetObject returned %d, error %d\n", size, GetLastError());
            ok(ext_pen.elp.elpHatch == HS_CROSS, "expected HS_CROSS, got %p\n", (void *)ext_pen.elp.elpHatch);
            ok(ext_pen.elp.elpNumEntries == 0, "expected 0, got %x\n", ext_pen.elp.elpNumEntries);
            break;
        }

        /* for some reason XP differentiates PS_NULL here */
        if (pen[i].style == PS_NULL)
            ok(ext_pen.elp.elpPenStyle == pen[i].ret_style, "expected %x, got %x\n", pen[i].ret_style, ext_pen.elp.elpPenStyle);
        else
        {
            ok(ext_pen.elp.elpPenStyle == (PS_GEOMETRIC | pen[i].style), "expected %x, got %x\n", PS_GEOMETRIC | pen[i].style, ext_pen.elp.elpPenStyle);
        }

        if (pen[i].style == PS_NULL)
            ok(ext_pen.elp.elpWidth == 0, "expected 0, got %x\n", ext_pen.elp.elpWidth);
        else
            ok(ext_pen.elp.elpWidth == pen[i].ret_width, "expected %u, got %x\n", pen[i].ret_width, ext_pen.elp.elpWidth);
        ok(ext_pen.elp.elpColor == pen[i].ret_color, "expected %08x, got %08x\n", pen[i].ret_color, ext_pen.elp.elpColor);
        ok(ext_pen.elp.elpBrushStyle == BS_SOLID, "expected BS_SOLID, got %x\n", ext_pen.elp.elpBrushStyle);

        DeleteObject(hpen);
    }
}

static unsigned int atoi2(const char *s)
{
    unsigned int ret = 0;
    while(*s) ret = (ret << 1) | (*s++ == '1');
    return ret;
}

#define TEST_LINE(x1, x2, z) \
    { int buf = 0; \
      SetBitmapBits(bmp, sizeof(buf), &buf); \
      MoveToEx(hdc, x1, 0, NULL); \
      LineTo(hdc, x2, 0); \
      GetBitmapBits(bmp, sizeof(buf), &buf); \
      expect(atoi2(z), buf); }

static void test_ps_alternate(void)
{
    HDC hdc;
    HBITMAP bmp;
    HPEN pen;
    LOGBRUSH lb;

    lb.lbStyle = BS_SOLID;
    lb.lbColor = RGB(0xff,0xff,0xff);

    SetLastError(0xdeadbeef);
    pen = ExtCreatePen(PS_COSMETIC|PS_ALTERNATE, 1, &lb, 0, NULL);
    if(pen == NULL && GetLastError() == 0xdeadbeef) {
        skip("looks like 9x, skipping PS_ALTERNATE tests\n");
        return;
    }
    ok(pen != NULL, "gle=%d\n", GetLastError());
    hdc = CreateCompatibleDC(NULL);
    ok(hdc != NULL, "gle=%d\n", GetLastError());
    bmp = CreateBitmap(8, 1, 1, 1, NULL);
    ok(bmp != NULL, "gle=%d\n", GetLastError());
    ok(SelectObject(hdc, bmp) != NULL, "gle=%d\n", GetLastError());
    ok(SelectObject(hdc, pen) != NULL, "gle=%d\n", GetLastError());
    ok(SetBkMode(hdc, TRANSPARENT), "gle=%d\n", GetLastError());

    TEST_LINE(0, 1, "10000000")
    TEST_LINE(0, 2, "10000000")
    TEST_LINE(0, 3, "10100000")
    TEST_LINE(0, 4, "10100000")
    TEST_LINE(1, 4, "01010000")
    TEST_LINE(1, 5, "01010000")
    TEST_LINE(4, 8, "00001010")

    DeleteObject(pen);
    DeleteObject(bmp);
    DeleteDC(hdc);
}

static void test_ps_userstyle(void)
{
    static DWORD style[17] = {0, 2, 0, 4, 5, 0, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 17};
    static DWORD bad_style[5] = {0, 0, 0, 0, 0};
    static DWORD bad_style2[5] = {4, 7, 8, 3, -1};

    LOGBRUSH lb;
    HPEN pen;
    INT size, i;

    struct
    {
        EXTLOGPEN elp;
        DWORD style_data[15];
    } ext_pen;

    lb.lbColor = 0x00ff0000;
    lb.lbStyle = BS_SOLID;
    lb.lbHatch = 0;

    pen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 50, &lb, 3, NULL);
    ok(pen == 0, "ExtCreatePen should fail\n");
    expect(ERROR_INVALID_PARAMETER, GetLastError());
    DeleteObject(pen);
    SetLastError(0xdeadbeef);

    pen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 50, &lb, 0, style);
    ok(pen == 0, "ExtCreatePen should fail\n");
    expect2(0xdeadbeef, ERROR_INVALID_PARAMETER, GetLastError());
    DeleteObject(pen);
    SetLastError(0xdeadbeef);

    pen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 50, &lb, 17, style);
    ok(pen == 0, "ExtCreatePen should fail\n");
    expect(ERROR_INVALID_PARAMETER, GetLastError());
    DeleteObject(pen);
    SetLastError(0xdeadbeef);

    pen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 50, &lb, -1, style);
    ok(pen == 0, "ExtCreatePen should fail\n");
    expect(0xdeadbeef, GetLastError());
    DeleteObject(pen);
    SetLastError(0xdeadbeef);

    pen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 50, &lb, 5, bad_style);
    ok(pen == 0, "ExtCreatePen should fail\n");
    expect(ERROR_INVALID_PARAMETER, GetLastError());
    DeleteObject(pen);
    SetLastError(0xdeadbeef);

    pen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 50, &lb, 5, bad_style2);
    ok(pen == 0, "ExtCreatePen should fail\n");
    expect(ERROR_INVALID_PARAMETER, GetLastError());
    DeleteObject(pen);
    SetLastError(0xdeadbeef);

    pen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 50, &lb, 16, style);
    ok(pen != 0, "ExtCreatePen should not fail\n");

    size = GetObject(pen, sizeof(ext_pen), &ext_pen);
    expect(88, size);

    for(i = 0; i < 16; i++)
        expect(style[i], ext_pen.elp.elpStyleEntry[i]);

    DeleteObject(pen);
}

START_TEST(pen)
{
    test_logpen();
    test_ps_alternate();
    test_ps_userstyle();
}
