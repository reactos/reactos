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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/test.h"

#define expect(expected,got) expect_(__LINE__, expected, got)
static inline void expect_(unsigned line, DWORD expected, DWORD got)
{
    ok_(__FILE__, line)(expected == got, "Expected %.08lx, got %.08lx\n", expected, got);
}
#define expect2(expected,alt,got) expect2_(__LINE__, expected, alt, got)
static inline void expect2_(unsigned line, DWORD expected, DWORD alt, DWORD got)
{
    ok_(__FILE__, line)(expected == got || alt == got,
                        "Expected %.08lx or %.08lx, got %.08lx\n", expected, alt, got);
}

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
        { PS_ALTERNATE, 123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) },
        {  9, 123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) },
        { 10, 123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) },
        { 11, 123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) },
        { 13, 123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) },
        { 14, 123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) },
        { 15, 123, RGB(0x12,0x34,0x56), PS_SOLID, 123, RGB(0x12,0x34,0x56) },
    };
    INT i, size;
    HPEN hpen;
    LOGPEN lp;
    EXTLOGPEN elp;
    LOGBRUSH lb;
    DWORD_PTR unset_hatch;
    DWORD obj_type, user_style[2] = { 0xabc, 0xdef };
    char elp_buffer[128];
    EXTLOGPEN *ext_pen = (EXTLOGPEN *)elp_buffer;
    DWORD *ext_style = ext_pen->elpStyleEntry;

    for (i = 0; i < ARRAY_SIZE(pen); i++)
    {
        trace("%d: testing style %u\n", i, pen[i].style);

        /********************** cosmetic pens **********************/
        /* CreatePenIndirect behaviour */
        lp.lopnStyle = pen[i].style;
        lp.lopnWidth.x = pen[i].width;
        lp.lopnWidth.y = 11; /* just in case */
        lp.lopnColor = pen[i].color;
        SetLastError(0xdeadbeef);
        hpen = CreatePenIndirect(&lp);
        if(hpen == 0 && GetLastError() == ERROR_INVALID_PARAMETER)
        {
            win_skip("No support for pen style %u (%d)\n", pen[i].style, i);
            continue;
        }

        obj_type = GetObjectType(hpen);
        ok(obj_type == OBJ_PEN, "wrong object type %lu\n", obj_type);

        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObjectW(hpen, sizeof(lp), &lp);
        ok(size == sizeof(lp), "GetObject returned %d, error %ld\n", size, GetLastError());

        ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
        ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %ld\n", pen[i].ret_width, lp.lopnWidth.x);
        ok(lp.lopnWidth.y == 0, "expected 0, got %ld\n", lp.lopnWidth.y);
        ok(lp.lopnColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, lp.lopnColor);

        DeleteObject(hpen);

        /* CreatePen behaviour */
        SetLastError(0xdeadbeef);
        hpen = CreatePen(pen[i].style, pen[i].width, pen[i].color);
        ok(hpen != 0, "CreatePen error %ld\n", GetLastError());

        obj_type = GetObjectType(hpen);
        ok(obj_type == OBJ_PEN, "wrong object type %lu\n", obj_type);

        /* check what's the real size of the object */
        size = GetObjectW(hpen, 0, NULL);
        ok(size == sizeof(lp), "GetObject returned %d, error %ld\n", size, GetLastError());

        /* ask for truncated data */
        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObjectW(hpen, sizeof(lp.lopnStyle), &lp);
        ok(!size, "GetObject should fail: size %d, error %ld\n", size, GetLastError());

        /* see how larger buffer sizes are handled */
        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObjectW(hpen, sizeof(lp) * 4, &lp);
        ok(size == sizeof(lp), "GetObject returned %d, error %ld\n", size, GetLastError());

        /* see how larger buffer sizes are handled */
        memset(&elp, 0xb0, sizeof(elp));
        SetLastError(0xdeadbeef);
        size = GetObjectW(hpen, sizeof(elp) * 2, &elp);
        ok(size == sizeof(lp), "GetObject returned %d, error %ld\n", size, GetLastError());

        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObjectW(hpen, sizeof(lp), &lp);
        ok(size == sizeof(lp), "GetObject returned %d, error %ld\n", size, GetLastError());

        ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
        ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %ld\n", pen[i].ret_width, lp.lopnWidth.x);
        ok(lp.lopnWidth.y == 0, "expected 0, got %ld\n", lp.lopnWidth.y);
        ok(lp.lopnColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, lp.lopnColor);

        memset(&elp, 0xb0, sizeof(elp));
        SetLastError(0xdeadbeef);
        size = GetObjectW(hpen, sizeof(elp), &elp);

        /* for some reason XP differentiates PS_NULL here */
        if (pen[i].style == PS_NULL)
        {
            ok(hpen == GetStockObject(NULL_PEN), "hpen should be a stock NULL_PEN\n");
            ok(size == offsetof(EXTLOGPEN, elpStyleEntry[1]), "GetObject returned %d, error %ld\n",
                size, GetLastError());
            ok(elp.elpPenStyle == pen[i].ret_style, "expected %u, got %lu\n", pen[i].ret_style, elp.elpPenStyle);
            ok(elp.elpWidth == 0, "expected 0, got %lu\n", elp.elpWidth);
            ok(elp.elpColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, elp.elpColor);
            ok(elp.elpBrushStyle == BS_SOLID, "expected BS_SOLID, got %u\n", elp.elpBrushStyle);
            ok(elp.elpHatch == 0, "expected 0, got %p\n", (void *)elp.elpHatch);
            ok(elp.elpNumEntries == 0, "expected 0, got %lx\n", elp.elpNumEntries);
        }
        else
        {
            ok(size == sizeof(LOGPEN), "GetObject returned %d, error %ld\n", size, GetLastError());
            memcpy(&lp, &elp, sizeof(lp));
            ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
            ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %ld\n", pen[i].ret_width, lp.lopnWidth.x);
            ok(lp.lopnWidth.y == 0, "expected 0, got %ld\n", lp.lopnWidth.y);
            ok(lp.lopnColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, lp.lopnColor);
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
               "wrong last error value %ld\n", GetLastError());
            SetLastError(0xdeadbeef);
            hpen = ExtCreatePen(pen[i].style, pen[i].width, &lb, 0, NULL);
            if (pen[i].style != PS_NULL)
            {
                ok(hpen == 0, "ExtCreatePen with width != 1 should fail\n");
                ok(GetLastError() == ERROR_INVALID_PARAMETER,
                   "wrong last error value %ld\n", GetLastError());

                SetLastError(0xdeadbeef);
                hpen = ExtCreatePen(pen[i].style, 1, &lb, 0, NULL);
            }
        }
        else
        {
            ok(hpen == 0, "ExtCreatePen with width != 1 should fail\n");
            ok(GetLastError() == ERROR_INVALID_PARAMETER,
               "wrong last error value %ld\n", GetLastError());
            SetLastError(0xdeadbeef);
            hpen = ExtCreatePen(pen[i].style, 1, &lb, 2, user_style);
        }
        if (pen[i].style == PS_INSIDEFRAME)
        {
            /* This style is applicable only for geometric pens */
            ok(hpen == 0, "ExtCreatePen should fail\n");
            goto test_geometric_pens;
        }
        if (pen[i].style > PS_ALTERNATE)
        {
            ok(hpen == 0, "ExtCreatePen should fail\n");
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong last error value %ld\n", GetLastError());
            goto test_geometric_pens;
        }
        ok(hpen != 0, "ExtCreatePen error %ld\n", GetLastError());

        obj_type = GetObjectType(hpen);
        /* for some reason XP differentiates PS_NULL here */
        if (pen[i].style == PS_NULL)
        {
            ok(obj_type == OBJ_PEN, "wrong object type %lu\n", obj_type);
            ok(hpen == GetStockObject(NULL_PEN), "hpen should be a stock NULL_PEN\n");
        }
        else
            ok(obj_type == OBJ_EXTPEN, "wrong object type %lu\n", obj_type);

        /* check what's the real size of the object */
        SetLastError(0xdeadbeef);
        size = GetObjectW(hpen, 0, NULL);
        switch (pen[i].style)
        {
        case PS_NULL:
            ok(size == sizeof(LOGPEN),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            break;

        case PS_USERSTYLE:
            ok(size == offsetof( EXTLOGPEN, elpStyleEntry[2] ),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            break;

        default:
            ok(size == offsetof( EXTLOGPEN, elpStyleEntry ),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            break;
        }

        /* ask for truncated data */
        memset(&elp, 0xb0, sizeof(elp));
        SetLastError(0xdeadbeef);
        size = GetObjectW(hpen, sizeof(elp.elpPenStyle), &elp);
        ok(!size, "GetObject should fail: size %d, error %ld\n", size, GetLastError());

        /* see how larger buffer sizes are handled */
        memset(elp_buffer, 0xb0, sizeof(elp_buffer));
        SetLastError(0xdeadbeef);
        size = GetObjectW(hpen, sizeof(elp_buffer), elp_buffer);
        switch (pen[i].style)
        {
        case PS_NULL:
            ok(size == sizeof(LOGPEN),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            memcpy(&lp, ext_pen, sizeof(lp));
            ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
            ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %ld\n", pen[i].ret_width, lp.lopnWidth.x);
            ok(lp.lopnWidth.y == 0, "expected 0, got %ld\n", lp.lopnWidth.y);
            ok(lp.lopnColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, lp.lopnColor);

            /* for PS_NULL it also works this way */
            memset(&elp, 0xb0, sizeof(elp));
            memset(&unset_hatch, 0xb0, sizeof(unset_hatch));
            SetLastError(0xdeadbeef);
            size = GetObjectW(hpen, sizeof(elp), &elp);
            ok(size == offsetof(EXTLOGPEN, elpStyleEntry[1]),
                "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(ext_pen->elpHatch == unset_hatch, "expected 0xb0b0b0b0, got %p\n", (void *)ext_pen->elpHatch);
            ok(ext_pen->elpNumEntries == 0xb0b0b0b0, "expected 0xb0b0b0b0, got %lx\n", ext_pen->elpNumEntries);
            break;

        case PS_USERSTYLE:
            ok(size == offsetof( EXTLOGPEN, elpStyleEntry[2] ),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(ext_pen->elpHatch == HS_CROSS, "expected HS_CROSS, got %p\n", (void *)ext_pen->elpHatch);
            ok(ext_pen->elpNumEntries == 2, "expected 0, got %lx\n", ext_pen->elpNumEntries);
            ok(ext_style[0] == 0xabc, "expected 0xabc, got %lx\n", ext_style[0]);
            ok(ext_style[1] == 0xdef, "expected 0xdef, got %lx\n", ext_style[1]);
            break;

        default:
            ok(size == offsetof( EXTLOGPEN, elpStyleEntry ),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(ext_pen->elpHatch == HS_CROSS, "expected HS_CROSS, got %p\n", (void *)ext_pen->elpHatch);
            ok(ext_pen->elpNumEntries == 0, "expected 0, got %lx\n", ext_pen->elpNumEntries);
            break;
        }

        ok(ext_pen->elpPenStyle == pen[i].style, "expected %x, got %lx\n", pen[i].style, ext_pen->elpPenStyle);
        ok(ext_pen->elpWidth == 1, "expected 1, got %lx\n", ext_pen->elpWidth);
        ok(ext_pen->elpColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, ext_pen->elpColor);
        ok(ext_pen->elpBrushStyle == BS_SOLID, "expected BS_SOLID, got %x\n", ext_pen->elpBrushStyle);

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
        if (pen[i].style > PS_ALTERNATE)
        {
            ok(hpen == 0, "ExtCreatePen should fail\n");
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong last error value %ld\n", GetLastError());
            continue;
        }
        ok(hpen != 0, "ExtCreatePen error %ld\n", GetLastError());

        obj_type = GetObjectType(hpen);
        /* for some reason XP differentiates PS_NULL here */
        if (pen[i].style == PS_NULL)
            ok(obj_type == OBJ_PEN, "wrong object type %lu\n", obj_type);
        else
            ok(obj_type == OBJ_EXTPEN, "wrong object type %lu\n", obj_type);

        /* check what's the real size of the object */
        size = GetObjectW(hpen, 0, NULL);
        switch (pen[i].style)
        {
        case PS_NULL:
            ok(size == sizeof(LOGPEN),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            break;

        case PS_USERSTYLE:
            ok(size == offsetof( EXTLOGPEN, elpStyleEntry[2] ),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            break;

        default:
            ok(size == offsetof( EXTLOGPEN, elpStyleEntry ),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            break;
        }

        /* ask for truncated data */
        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObjectW(hpen, sizeof(lp.lopnStyle), &lp);
        ok(!size, "GetObject should fail: size %d, error %ld\n", size, GetLastError());

        memset(&lp, 0xb0, sizeof(lp));
        SetLastError(0xdeadbeef);
        size = GetObjectW(hpen, sizeof(lp), &lp);
        /* for some reason XP differentiates PS_NULL here */
        if (pen[i].style == PS_NULL)
        {
            ok(size == sizeof(LOGPEN), "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
            ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %ld\n", pen[i].ret_width, lp.lopnWidth.x);
            ok(lp.lopnWidth.y == 0, "expected 0, got %ld\n", lp.lopnWidth.y);
            ok(lp.lopnColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, lp.lopnColor);
        }
        else
            /* XP doesn't set last error here */
            ok(!size /*&& GetLastError() == ERROR_INVALID_PARAMETER*/,
               "GetObject should fail: size %d, error %ld\n", size, GetLastError());

        memset(elp_buffer, 0xb0, sizeof(elp_buffer));
        SetLastError(0xdeadbeef);
        /* buffer is too small for user styles */
        size = GetObjectW(hpen, offsetof(EXTLOGPEN, elpStyleEntry[1]), elp_buffer);
        switch (pen[i].style)
        {
        case PS_NULL:
            ok(size == offsetof(EXTLOGPEN, elpStyleEntry[1]),
                "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(ext_pen->elpHatch == 0, "expected 0, got %p\n", (void *)ext_pen->elpHatch);
            ok(ext_pen->elpNumEntries == 0, "expected 0, got %lx\n", ext_pen->elpNumEntries);

            /* for PS_NULL it also works this way */
            SetLastError(0xdeadbeef);
            size = GetObjectW(hpen, sizeof(elp_buffer), &lp);
            ok(size == sizeof(LOGPEN),
                "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(lp.lopnStyle == pen[i].ret_style, "expected %u, got %u\n", pen[i].ret_style, lp.lopnStyle);
            ok(lp.lopnWidth.x == pen[i].ret_width, "expected %u, got %ld\n", pen[i].ret_width, lp.lopnWidth.x);
            ok(lp.lopnWidth.y == 0, "expected 0, got %ld\n", lp.lopnWidth.y);
            ok(lp.lopnColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, lp.lopnColor);
            break;

        case PS_USERSTYLE:
            ok(!size /*&& GetLastError() == ERROR_INVALID_PARAMETER*/,
               "GetObject should fail: size %d, error %ld\n", size, GetLastError());
            size = GetObjectW(hpen, sizeof(elp_buffer), elp_buffer);
            ok(size == offsetof( EXTLOGPEN, elpStyleEntry[2] ),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(ext_pen->elpHatch == HS_CROSS, "expected HS_CROSS, got %p\n", (void *)ext_pen->elpHatch);
            ok(ext_pen->elpNumEntries == 2, "expected 0, got %lx\n", ext_pen->elpNumEntries);
            ok(ext_style[0] == 0xabc, "expected 0xabc, got %lx\n", ext_style[0]);
            ok(ext_style[1] == 0xdef, "expected 0xdef, got %lx\n", ext_style[1]);
            break;

        default:
            ok(size == offsetof( EXTLOGPEN, elpStyleEntry ),
               "GetObject returned %d, error %ld\n", size, GetLastError());
            ok(ext_pen->elpHatch == HS_CROSS, "expected HS_CROSS, got %p\n", (void *)ext_pen->elpHatch);
            ok(ext_pen->elpNumEntries == 0, "expected 0, got %lx\n", ext_pen->elpNumEntries);
            break;
        }

        /* for some reason XP differentiates PS_NULL here */
        if (pen[i].style == PS_NULL)
            ok(ext_pen->elpPenStyle == pen[i].ret_style, "expected %x, got %lx\n", pen[i].ret_style, ext_pen->elpPenStyle);
        else
        {
            ok(ext_pen->elpPenStyle == (PS_GEOMETRIC | pen[i].style), "expected %x, got %lx\n", PS_GEOMETRIC | pen[i].style, ext_pen->elpPenStyle);
        }

        if (pen[i].style == PS_NULL)
            ok(ext_pen->elpWidth == 0, "expected 0, got %lx\n", ext_pen->elpWidth);
        else
            ok(ext_pen->elpWidth == pen[i].ret_width, "expected %u, got %lx\n", pen[i].ret_width, ext_pen->elpWidth);
        ok(ext_pen->elpColor == pen[i].ret_color, "expected %08lx, got %08lx\n", pen[i].ret_color, ext_pen->elpColor);
        ok(ext_pen->elpBrushStyle == BS_SOLID, "expected BS_SOLID, got %x\n", ext_pen->elpBrushStyle);

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
    INT iRet;
    HGDIOBJ hRet;

    lb.lbStyle = BS_SOLID;
    lb.lbColor = RGB(0xff,0xff,0xff);

    SetLastError(0xdeadbeef);
    pen = ExtCreatePen(PS_COSMETIC|PS_ALTERNATE, 1, &lb, 0, NULL);
    if(pen == NULL && GetLastError() == 0xdeadbeef) {
        skip("looks like 9x, skipping PS_ALTERNATE tests\n");
        return;
    }
    ok(pen != NULL, "gle=%ld\n", GetLastError());
    hdc = CreateCompatibleDC(NULL);
    ok(hdc != NULL, "gle=%ld\n", GetLastError());
    bmp = CreateBitmap(8, 1, 1, 1, NULL);
    ok(bmp != NULL, "gle=%ld\n", GetLastError());
    hRet = SelectObject(hdc, bmp);
    ok(hRet != NULL, "gle=%ld\n", GetLastError());
    hRet = SelectObject(hdc, pen);
    ok(hRet != NULL, "gle=%ld\n", GetLastError());
    iRet = SetBkMode(hdc, TRANSPARENT);
    ok(iRet, "gle=%ld\n", GetLastError());

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
    char buffer[offsetof(EXTLOGPEN, elpStyleEntry) + 16 * sizeof(DWORD)];
    EXTLOGPEN *ext_pen = (EXTLOGPEN *)buffer;

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

    size = GetObjectW(pen, sizeof(buffer), ext_pen);
    ok(size == offsetof(EXTLOGPEN, elpStyleEntry[16]), "wrong size %d\n", size);

    for(i = 0; i < 16; i++)
        expect(style[i], ext_pen->elpStyleEntry[i]);

    DeleteObject(pen);
}

static void test_brush_pens(void)
{
    char buffer[offsetof(EXTLOGPEN, elpStyleEntry) + 16 * sizeof(DWORD)];
    EXTLOGPEN *elp = (EXTLOGPEN *)buffer;
    LOGBRUSH lb;
    HPEN pen = 0;
    DWORD size;
    HBITMAP bmp = CreateBitmap( 8, 8, 1, 1, NULL );
    BITMAPINFO *info;
    HGLOBAL hmem;

    hmem = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(*info) + 16 * 16 * 4 );
    info = GlobalLock( hmem );
    info->bmiHeader.biSize        = sizeof(info->bmiHeader);
    info->bmiHeader.biWidth       = 16;
    info->bmiHeader.biHeight      = 16;
    info->bmiHeader.biPlanes      = 1;
    info->bmiHeader.biBitCount    = 32;
    info->bmiHeader.biCompression = BI_RGB;

    for (lb.lbStyle = BS_SOLID; lb.lbStyle <= BS_MONOPATTERN + 1; lb.lbStyle++)
    {
        SetLastError( 0xdeadbeef );
        memset( buffer, 0xcc, sizeof(buffer) );
        trace( "testing brush style %u\n", lb.lbStyle );

        switch (lb.lbStyle)
        {
        case BS_SOLID:
        case BS_HATCHED:
            lb.lbColor = RGB(12,34,56);
            lb.lbHatch = HS_CROSS;
            pen = ExtCreatePen( PS_DOT | PS_GEOMETRIC, 3, &lb, 0, NULL );
            ok( pen != 0, "ExtCreatePen failed err %lu\n", GetLastError() );
            size = GetObjectW( pen, sizeof(buffer), elp );
            ok( size == offsetof( EXTLOGPEN, elpStyleEntry ), "wrong size %lu\n", size );
            ok( elp->elpPenStyle == (PS_DOT | PS_GEOMETRIC), "wrong pen style %lx\n", elp->elpPenStyle );
            ok( elp->elpBrushStyle == lb.lbStyle, "wrong brush style %x\n", elp->elpBrushStyle );
            ok( elp->elpColor == RGB(12,34,56), "wrong color %lx\n", elp->elpColor );
            ok( elp->elpHatch == HS_CROSS, "wrong hatch %Ix\n", elp->elpHatch );
            ok( elp->elpNumEntries == 0, "wrong entries %lx\n", elp->elpNumEntries );
            break;

        case BS_NULL:
            pen = ExtCreatePen( PS_SOLID | PS_GEOMETRIC, 3, &lb, 0, NULL );
            ok( pen != 0, "ExtCreatePen failed err %lu\n", GetLastError() );
            size = GetObjectW( pen, sizeof(buffer), elp );
            ok( size == sizeof(LOGPEN), "wrong size %lu\n", size );
            ok( ((LOGPEN *)elp)->lopnStyle == PS_NULL,
                "wrong pen style %x\n", ((LOGPEN *)elp)->lopnStyle );
            ok( ((LOGPEN *)elp)->lopnColor == 0,
                "wrong color %lx\n", ((LOGPEN *)elp)->lopnColor );
            break;

        case BS_PATTERN:
            lb.lbColor = RGB(12,34,56);
            lb.lbHatch = (ULONG_PTR)bmp;
            pen = ExtCreatePen( PS_DOT | PS_GEOMETRIC, 3, &lb, 0, NULL );
            ok( pen != 0, "ExtCreatePen failed err %lu\n", GetLastError() );
            size = GetObjectW( pen, sizeof(buffer), elp );
            ok( size == offsetof( EXTLOGPEN, elpStyleEntry ), "wrong size %lu\n", size );
            ok( elp->elpPenStyle == (PS_DOT | PS_GEOMETRIC), "wrong pen style %lx\n", elp->elpPenStyle );
            ok( elp->elpBrushStyle == BS_PATTERN, "wrong brush style %x\n", elp->elpBrushStyle );
            ok( elp->elpColor == 0, "wrong color %lx\n", elp->elpColor );
            ok( elp->elpHatch == (ULONG_PTR)bmp, "wrong hatch %Ix/%p\n", elp->elpHatch, bmp );
            ok( elp->elpNumEntries == 0, "wrong entries %lx\n", elp->elpNumEntries );
            break;

        case BS_DIBPATTERN:
        case BS_DIBPATTERNPT:
            lb.lbColor = DIB_PAL_COLORS;
            lb.lbHatch = lb.lbStyle == BS_DIBPATTERN ? (ULONG_PTR)hmem : (ULONG_PTR)info;
            pen = ExtCreatePen( PS_DOT | PS_GEOMETRIC, 3, &lb, 0, NULL );
            ok( pen != 0, "ExtCreatePen failed err %lu\n", GetLastError() );
            size = GetObjectW( pen, sizeof(buffer), elp );
            ok( size == offsetof( EXTLOGPEN, elpStyleEntry ), "wrong size %lu\n", size );
            ok( elp->elpPenStyle == (PS_DOT | PS_GEOMETRIC), "wrong pen style %lx\n", elp->elpPenStyle );
            ok( elp->elpBrushStyle == BS_DIBPATTERNPT, "wrong brush style %x\n", elp->elpBrushStyle );
            ok( elp->elpColor == 0, "wrong color %lx\n", elp->elpColor );
            ok( elp->elpHatch == lb.lbHatch, "wrong hatch %Ix/%Ix\n", elp->elpHatch, lb.lbHatch );
            ok( elp->elpNumEntries == 0, "wrong entries %lx\n", elp->elpNumEntries );
            break;

        default:
            pen = ExtCreatePen( PS_DOT | PS_GEOMETRIC, 3, &lb, 0, NULL );
            ok( !pen, "ExtCreatePen succeeded\n" );
            ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
            break;
        }

        if (pen) DeleteObject( pen );
        else continue;

        /* cosmetic pens require BS_SOLID */
        SetLastError( 0xdeadbeef );
        pen = ExtCreatePen( PS_DOT, 1, &lb, 0, NULL );
        if (lb.lbStyle == BS_SOLID)
        {
            ok( pen != 0, "ExtCreatePen failed err %lu\n", GetLastError() );
            size = GetObjectW( pen, sizeof(buffer), elp );
            ok( size == offsetof( EXTLOGPEN, elpStyleEntry ), "wrong size %lu\n", size );
            ok( elp->elpPenStyle == PS_DOT, "wrong pen style %lx\n", elp->elpPenStyle );
            ok( elp->elpBrushStyle == BS_SOLID, "wrong brush style %x\n", elp->elpBrushStyle );
            ok( elp->elpColor == RGB(12,34,56), "wrong color %lx\n", elp->elpColor );
            ok( elp->elpHatch == HS_CROSS, "wrong hatch %Ix\n", elp->elpHatch );
            DeleteObject( pen );
        }
        else
        {
            ok( !pen, "ExtCreatePen succeeded\n" );
            ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
        }
    }

    GlobalUnlock( hmem );
    GlobalFree( hmem );
    DeleteObject( bmp );
}

START_TEST(pen)
{
    test_logpen();
    test_brush_pens();
    test_ps_alternate();
    test_ps_userstyle();
}
