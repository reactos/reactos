/*
 * Unit test suite for paths
 *
 * Copyright 2007 Laurent Vromman
 * Copyright 2007 Misha Koshelev
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
#include <stdio.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

#include "wine/test.h"

#include "winuser.h"
#include "winerror.h"

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)

static void test_path_state(void)
{
    BYTE buffer[sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD)];
    BITMAPINFO *bi = (BITMAPINFO *)buffer;
    HDC hdc;
    HRGN rgn;
    HBITMAP orig, dib;
    void *bits;
    BOOL ret;

    hdc = CreateCompatibleDC( 0 );
    memset( buffer, 0, sizeof(buffer) );
    bi->bmiHeader.biSize = sizeof(bi->bmiHeader);
    bi->bmiHeader.biHeight = 256;
    bi->bmiHeader.biWidth = 256;
    bi->bmiHeader.biBitCount = 32;
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biCompression = BI_RGB;
    dib = CreateDIBSection( 0, bi, DIB_RGB_COLORS, &bits, NULL, 0 );
    orig = SelectObject( hdc, dib );

    BeginPath( hdc );
    LineTo( hdc, 100, 100 );
    ret = WidenPath( hdc );
    ok( !ret, "WidenPath succeeded\n" );

    /* selecting another bitmap doesn't affect the path */
    SelectObject( hdc, orig );
    ret = WidenPath( hdc );
    ok( !ret, "WidenPath succeeded\n" );

    SelectObject( hdc, dib );
    ret = WidenPath( hdc );
    ok( !ret, "WidenPath succeeded\n" );

    ret = EndPath( hdc );
    ok( ret, "EndPath failed error %lu\n", GetLastError() );
    ret = WidenPath( hdc );
    ok( ret, "WidenPath failed error %lu\n", GetLastError() );

    SelectObject( hdc, orig );
    ret = WidenPath( hdc );
    ok( ret, "WidenPath failed error %lu\n", GetLastError() );

    BeginPath( hdc );
    LineTo( hdc, 100, 100 );
    ret = WidenPath( hdc );
    ok( !ret, "WidenPath succeeded\n" );
    SaveDC( hdc );
    SelectObject( hdc, dib );
    ret = EndPath( hdc );
    ok( ret, "EndPath failed error %lu\n", GetLastError() );
    ret = WidenPath( hdc );
    ok( ret, "WidenPath failed error %lu\n", GetLastError() );

    /* path should be open again after RestoreDC */
    RestoreDC( hdc, -1  );
    ret = WidenPath( hdc );
    ok( !ret, "WidenPath succeeded\n" );
    ret = EndPath( hdc );
    ok( ret, "EndPath failed error %lu\n", GetLastError() );

    SaveDC( hdc );
    BeginPath( hdc );
    RestoreDC( hdc, -1  );
    ret = WidenPath( hdc );
    ok( ret, "WidenPath failed error %lu\n", GetLastError() );

    /* test all functions with no path at all */
    AbortPath( hdc );
    SetLastError( 0xdeadbeef );
    ret = WidenPath( hdc );
    ok( !ret, "WidenPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = FlattenPath( hdc );
    ok( !ret, "FlattenPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = StrokePath( hdc );
    ok( !ret, "StrokePath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = FillPath( hdc );
    ok( !ret, "FillPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = StrokeAndFillPath( hdc );
    ok( !ret, "StrokeAndFillPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = SelectClipPath( hdc, RGN_OR );
    ok( !ret, "SelectClipPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    rgn = PathToRegion( hdc );
    ok( !rgn, "PathToRegion succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = EndPath( hdc );
    ok( !ret, "SelectClipPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = CloseFigure( hdc );
    ok( !ret, "CloseFigure succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %lu\n", GetLastError() );

    /* test all functions with an open path */
    AbortPath( hdc );
    BeginPath( hdc );
    SetLastError( 0xdeadbeef );
    ret = WidenPath( hdc );
    ok( !ret, "WidenPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %lu\n", GetLastError() );

    AbortPath( hdc );
    BeginPath( hdc );
    SetLastError( 0xdeadbeef );
    ret = FlattenPath( hdc );
    ok( !ret, "FlattenPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %lu\n", GetLastError() );

    AbortPath( hdc );
    BeginPath( hdc );
    SetLastError( 0xdeadbeef );
    ret = StrokePath( hdc );
    ok( !ret, "StrokePath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %lu\n", GetLastError() );

    AbortPath( hdc );
    BeginPath( hdc );
    SetLastError( 0xdeadbeef );
    ret = FillPath( hdc );
    ok( !ret, "FillPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %lu\n", GetLastError() );

    AbortPath( hdc );
    BeginPath( hdc );
    SetLastError( 0xdeadbeef );
    ret = StrokeAndFillPath( hdc );
    ok( !ret, "StrokeAndFillPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %lu\n", GetLastError() );

    AbortPath( hdc );
    BeginPath( hdc );
    Rectangle( hdc, 1, 1, 10, 10 );  /* region needs some contents */
    SetLastError( 0xdeadbeef );
    ret = SelectClipPath( hdc, RGN_OR );
    ok( !ret, "SelectClipPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %lu\n", GetLastError() );

    AbortPath( hdc );
    BeginPath( hdc );
    Rectangle( hdc, 1, 1, 10, 10 );  /* region needs some contents */
    SetLastError( 0xdeadbeef );
    rgn = PathToRegion( hdc );
    ok( !rgn, "PathToRegion succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %lu\n", GetLastError() );

    AbortPath( hdc );
    BeginPath( hdc );
    ret = CloseFigure( hdc );
    ok( ret, "CloseFigure failed\n" );

    /* test all functions with a closed path */
    AbortPath( hdc );
    BeginPath( hdc );
    EndPath( hdc );
    ret = WidenPath( hdc );
    ok( ret, "WidenPath failed\n" );
    ok( GetPath( hdc, NULL, NULL, 0 ) != -1, "path deleted\n" );

    AbortPath( hdc );
    BeginPath( hdc );
    EndPath( hdc );
    ret = FlattenPath( hdc );
    ok( ret, "FlattenPath failed\n" );
    ok( GetPath( hdc, NULL, NULL, 0 ) != -1, "path deleted\n" );

    AbortPath( hdc );
    BeginPath( hdc );
    EndPath( hdc );
    ret = StrokePath( hdc );
    ok( ret, "StrokePath failed\n" );
    ok( GetPath( hdc, NULL, NULL, 0 ) == -1, "path not deleted\n" );

    BeginPath( hdc );
    EndPath( hdc );
    ret = FillPath( hdc );
    ok( ret, "FillPath failed\n" );
    ok( GetPath( hdc, NULL, NULL, 0 ) == -1, "path not deleted\n" );

    BeginPath( hdc );
    EndPath( hdc );
    ret = StrokeAndFillPath( hdc );
    ok( ret, "StrokeAndFillPath failed\n" );
    ok( GetPath( hdc, NULL, NULL, 0 ) == -1, "path not deleted\n" );

    BeginPath( hdc );
    Rectangle( hdc, 1, 1, 10, 10 );  /* region needs some contents */
    EndPath( hdc );
    ret = SelectClipPath( hdc, RGN_OR );
    ok( ret, "SelectClipPath failed\n" );
    ok( GetPath( hdc, NULL, NULL, 0 ) == -1, "path not deleted\n" );

    BeginPath( hdc );
    EndPath( hdc );
    SetLastError( 0xdeadbeef );
    ret = SelectClipPath( hdc, RGN_OR );
    ok( !ret, "SelectClipPath succeeded on empty path\n" );
    ok( GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError() );
    ok( GetPath( hdc, NULL, NULL, 0 ) == -1, "path not deleted\n" );

    BeginPath( hdc );
    Rectangle( hdc, 1, 1, 10, 10 );  /* region needs some contents */
    EndPath( hdc );
    rgn = PathToRegion( hdc );
    ok( rgn != 0, "PathToRegion failed\n" );
    DeleteObject( rgn );
    ok( GetPath( hdc, NULL, NULL, 0 ) == -1, "path not deleted\n" );

    BeginPath( hdc );
    EndPath( hdc );
    SetLastError( 0xdeadbeef );
    rgn = PathToRegion( hdc );
    ok( !rgn, "PathToRegion succeeded on empty path\n" );
    ok( GetLastError() == 0xdeadbeef, "wrong error %lu\n", GetLastError() );
    DeleteObject( rgn );
    ok( GetPath( hdc, NULL, NULL, 0 ) == -1, "path not deleted\n" );

    BeginPath( hdc );
    EndPath( hdc );
    SetLastError( 0xdeadbeef );
    ret = CloseFigure( hdc );
    ok( !ret, "CloseFigure succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %lu\n", GetLastError() );

    AbortPath( hdc );
    BeginPath( hdc );
    EndPath( hdc );
    SetLastError( 0xdeadbeef );
    ret = EndPath( hdc );
    ok( !ret, "EndPath succeeded\n" );
    ok( GetLastError() == ERROR_CAN_NOT_COMPLETE || broken(GetLastError() == 0xdeadbeef),
        "wrong error %lu\n", GetLastError() );

    DeleteDC( hdc );
    DeleteObject( dib );
}

static void test_widenpath(void)
{
    HDC hdc = GetDC(0);
    HPEN greenPen, narrowPen;
    POINT pnt[6];
    INT nSize;
    BOOL ret;

    /* Create a pen to be used in WidenPath */
    greenPen = CreatePen(PS_SOLID, 10, RGB(0,0,0));
    SelectObject(hdc, greenPen);

    /* Prepare a path */
    pnt[0].x = 100;
    pnt[0].y = 0;
    pnt[1].x = 200;
    pnt[1].y = 0;
    pnt[2].x = 300;
    pnt[2].y = 100;
    pnt[3].x = 300;
    pnt[3].y = 200;
    pnt[4].x = 200;
    pnt[4].y = 300;
    pnt[5].x = 100;
    pnt[5].y = 300;

    /* Set a polyline path */
    BeginPath(hdc);
    Polyline(hdc, pnt, 6);
    EndPath(hdc);

    /* Widen the polyline path */
    ok(WidenPath(hdc), "WidenPath fails while widening a poyline path.\n");

    /* Test if WidenPath seems to have done his job */
    nSize = GetPath(hdc, NULL, NULL, 0);
    ok(nSize != -1, "GetPath fails after calling WidenPath.\n");
    ok(nSize > 6, "Path number of points is too low. Should be more than 6 but is %d\n", nSize);

    AbortPath(hdc);

    /* Test WidenPath with an open path (last error only set on Win2k and later) */
    SetLastError(0xdeadbeef);
    BeginPath(hdc);
    ret = WidenPath(hdc);
    ok(ret == FALSE && (GetLastError() == ERROR_CAN_NOT_COMPLETE || GetLastError() == 0xdeadbeef),
       "WidenPath fails while widening an open path. Return value is %d, should be %d. Error is %lu\n", ret, FALSE, GetLastError());

    AbortPath(hdc);

    /* Test when the pen width is equal to 1. The path should change too */
    narrowPen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
    SelectObject(hdc, narrowPen);
    BeginPath(hdc);
    Polyline(hdc, pnt, 6);
    EndPath(hdc);
    ret = WidenPath(hdc);
    ok(ret == TRUE, "WidenPath failed: %ld\n", GetLastError());
    nSize = GetPath(hdc, NULL, NULL, 0);
    ok(nSize > 6, "WidenPath should compute a widened path with a 1px wide pen. Path length is %d, should be more than 6\n", nSize);

    ReleaseDC(0, hdc);
    return;
}

/*
 * Tests for GDI drawing functions in paths
 */

typedef struct
{
    int x, y;
    BYTE type;
} path_test_t;

/* Helper function to verify that the current path in the given DC matches the expected path.
 *
 * We use a "smart" matching algorithm that allows us to detect partial improvements
 * in conformance. Specifically, two running indices are kept, one through the actual
 * path and one through the expected path. The actual path index increases unless there is
 * no match and the todo field of the appropriate path_test_t element is 2. Similarly,
 * if the wine_entries_preceding field of the appropriate path_test_t element is non-zero,
 * the expected path index does not increase for that many elements as long as there
 * is no match. This allows us to todo_wine extra path elements that are present only
 * on wine but not on native and vice versa.
 *
 * Note that if expected_size is zero and the WINETEST_DEBUG environment variable is
 * greater than 2, the trace() output is a C path_test_t array structure, useful for making
 * new tests that use this function.
 */
static void ok_path(HDC hdc, const char *path_name, const path_test_t *expected, int expected_size)
{
    static const char *type_string[8] = { "Unknown (0)", "PT_CLOSEFIGURE", "PT_LINETO",
                                          "PT_LINETO | PT_CLOSEFIGURE", "PT_BEZIERTO",
                                          "PT_BEZIERTO | PT_CLOSEFIGURE", "PT_MOVETO", "PT_MOVETO | PT_CLOSEFIGURE"};
    POINT *pnt;
    BYTE *types;
    int size, idx;

    /* Get the path */
    assert(hdc != 0);
    size = GetPath(hdc, NULL, NULL, 0);
    ok(size > 0, "GetPath returned size %d, last error %ld\n", size, GetLastError());
    if (size <= 0) return;

    pnt = malloc(size * sizeof(POINT));
    assert(pnt != 0);
    types = malloc(size);
    assert(types != 0);
    size = GetPath(hdc, pnt, types, size);
    assert(size > 0);

    ok( size == expected_size, "%s: Path size %d does not match expected size %d\n",
        path_name, size, expected_size);

    for (idx = 0; idx < min( size, expected_size ); idx++)
    {
        /* We allow a few pixels fudge in matching X and Y coordinates to account for imprecision in
         * floating point to integer conversion */
        static const int fudge = 2;

        ok( types[idx] == expected[idx].type, "%s: Expected #%d: %s (%d,%d) but got %s (%ld,%ld)\n",
            path_name, idx, type_string[expected[idx].type], expected[idx].x, expected[idx].y,
            type_string[types[idx]], pnt[idx].x, pnt[idx].y);

        if (types[idx] == expected[idx].type)
            ok( (pnt[idx].x >= expected[idx].x - fudge && pnt[idx].x <= expected[idx].x + fudge) &&
                (pnt[idx].y >= expected[idx].y - fudge && pnt[idx].y <= expected[idx].y + fudge),
                "%s: Expected #%d: %s  position (%d,%d) but got (%ld,%ld)\n", path_name, idx,
                type_string[expected[idx].type], expected[idx].x, expected[idx].y, pnt[idx].x, pnt[idx].y);
    }

    if (winetest_debug > 2)
    {
        printf("static const path_test_t %s[] =\n{\n", path_name);
        for (idx = 0; idx < size; idx++)
            printf("    {%ld, %ld, %s}, /* %d */\n", pnt[idx].x, pnt[idx].y, type_string[types[idx]], idx);
        printf("};\n" );
    }

    free(types);
    free(pnt);
}

static const path_test_t arcto_path[] =
{
    {0, 0, PT_MOVETO}, /* 0 */
    {229, 215, PT_LINETO}, /* 1 */
    {248, 205, PT_BEZIERTO}, /* 2 */
    {273, 200, PT_BEZIERTO}, /* 3 */
    {300, 200, PT_BEZIERTO}, /* 4 */
    {355, 200, PT_BEZIERTO}, /* 5 */
    {399, 222, PT_BEZIERTO}, /* 6 */
    {399, 250, PT_BEZIERTO}, /* 7 */
    {399, 263, PT_BEZIERTO}, /* 8 */
    {389, 275, PT_BEZIERTO}, /* 9 */
    {370, 285, PT_BEZIERTO}, /* 10 */
    {363, 277, PT_LINETO}, /* 11 */
    {380, 270, PT_BEZIERTO}, /* 12 */
    {389, 260, PT_BEZIERTO}, /* 13 */
    {389, 250, PT_BEZIERTO}, /* 14 */
    {389, 228, PT_BEZIERTO}, /* 15 */
    {349, 210, PT_BEZIERTO}, /* 16 */
    {300, 210, PT_BEZIERTO}, /* 17 */
    {276, 210, PT_BEZIERTO}, /* 18 */
    {253, 214, PT_BEZIERTO}, /* 19 */
    {236, 222, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 20 */
};

static void test_arcto(void)
{
    HDC hdc = GetDC(0);

    BeginPath(hdc);
    SetArcDirection(hdc, AD_CLOCKWISE);
    if (!ArcTo(hdc, 200, 200, 400, 300, 200, 200, 400, 300) &&
        GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        /* ArcTo is only available on Win2k and later */
        win_skip("ArcTo is not available\n");
        goto done;
    }
    SetArcDirection(hdc, AD_COUNTERCLOCKWISE);
    ArcTo(hdc, 210, 210, 390, 290, 390, 290, 210, 210);
    CloseFigure(hdc);
    EndPath(hdc);

    ok_path(hdc, "arcto_path", arcto_path, ARRAY_SIZE(arcto_path));
done:
    ReleaseDC(0, hdc);
}

static const path_test_t anglearc_path[] =
{
    {0, 0, PT_MOVETO}, /* 0 */
    {371, 229, PT_LINETO}, /* 1 */
    {352, 211, PT_BEZIERTO}, /* 2 */
    {327, 200, PT_BEZIERTO}, /* 3 */
    {300, 200, PT_BEZIERTO}, /* 4 */
    {245, 200, PT_BEZIERTO}, /* 5 */
    {200, 245, PT_BEZIERTO}, /* 6 */
    {200, 300, PT_BEZIERTO}, /* 7 */
    {200, 300, PT_BEZIERTO}, /* 8 */
    {200, 300, PT_BEZIERTO}, /* 9 */
    {200, 300, PT_BEZIERTO}, /* 10 */
    {231, 260, PT_LINETO}, /* 11 */
    {245, 235, PT_BEZIERTO}, /* 12 */
    {271, 220, PT_BEZIERTO}, /* 13 */
    {300, 220, PT_BEZIERTO}, /* 14 */
    {344, 220, PT_BEZIERTO}, /* 15 */
    {380, 256, PT_BEZIERTO}, /* 16 */
    {380, 300, PT_BEZIERTO}, /* 17 */
    {380, 314, PT_BEZIERTO}, /* 18 */
    {376, 328, PT_BEZIERTO}, /* 19 */
    {369, 340, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 20 */
};

static void test_anglearc(void)
{
    HDC hdc = GetDC(0);
    BeginPath(hdc);
    if (!AngleArc(hdc, 300, 300, 100, 45.0, 135.0) &&
        GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        /* AngleArc is only available on Win2k and later */
        win_skip("AngleArc is not available\n");
        goto done;
    }
    AngleArc(hdc, 300, 300, 80, 150.0, -180.0);
    CloseFigure(hdc);
    EndPath(hdc);

    ok_path(hdc, "anglearc_path", anglearc_path, ARRAY_SIZE(anglearc_path));
done:
    ReleaseDC(0, hdc);
}

static const path_test_t polydraw_path[] =
{
    {-20, -20, PT_MOVETO}, /* 0 */
    {10, 10, PT_LINETO}, /* 1 */
    {10, 15, PT_LINETO | PT_CLOSEFIGURE}, /* 2 */
    {-20, -20, PT_MOVETO}, /* 3 */
    {-10, -10, PT_LINETO}, /* 4 */
    {100, 100, PT_MOVETO}, /* 5 */
    {95, 95, PT_LINETO}, /* 6 */
    {10, 10, PT_LINETO}, /* 7 */
    {10, 15, PT_LINETO | PT_CLOSEFIGURE}, /* 8 */
    {100, 100, PT_MOVETO}, /* 9 */
    {15, 15, PT_LINETO}, /* 10 */
    {25, 25, PT_MOVETO}, /* 11 */
    {25, 30, PT_LINETO}, /* 12 */
    {100, 100, PT_MOVETO}, /* 13 */
    {30, 30, PT_BEZIERTO}, /* 14 */
    {30, 35, PT_BEZIERTO}, /* 15 */
    {35, 35, PT_BEZIERTO}, /* 16 */
    {35, 40, PT_LINETO}, /* 17 */
    {40, 40, PT_MOVETO}, /* 18 */
    {40, 45, PT_LINETO}, /* 19 */
    {35, 40, PT_MOVETO}, /* 20 */
    {45, 50, PT_LINETO}, /* 21 */
    {35, 40, PT_MOVETO}, /* 22 */
    {50, 55, PT_LINETO}, /* 23 */
    {45, 50, PT_LINETO}, /* 24 */
    {35, 40, PT_MOVETO}, /* 25 */
    {60, 60, PT_LINETO}, /* 26 */
    {60, 65, PT_MOVETO}, /* 27 */
    {65, 65, PT_LINETO}, /* 28 */
    {75, 75, PT_MOVETO}, /* 29 */
    {80, 80, PT_LINETO | PT_CLOSEFIGURE}, /* 30 */
};

static POINT polydraw_pts[] = {
    {10, 10}, {10, 15},
    {15, 15}, {15, 20}, {20, 20}, {20, 25},
    {25, 25}, {25, 30},
    {30, 30}, {30, 35}, {35, 35}, {35, 40},
    {40, 40}, {40, 45}, {45, 45},
    {45, 50}, {50, 50},
    {50, 55}, {45, 50}, {55, 60},
    {60, 60}, {60, 65}, {65, 65},
    {70, 70}, {75, 70}, {75, 75}, {80, 80}};

static BYTE polydraw_tps[] =
    {PT_LINETO, PT_CLOSEFIGURE | PT_LINETO, /* 2 */
     PT_LINETO, PT_BEZIERTO, PT_LINETO, PT_LINETO, /* 6 */
     PT_MOVETO, PT_LINETO, /* 8 */
     PT_BEZIERTO, PT_BEZIERTO, PT_BEZIERTO, PT_LINETO, /* 12 */
     PT_MOVETO, PT_LINETO, PT_CLOSEFIGURE, /* 15 */
     PT_LINETO, PT_MOVETO | PT_CLOSEFIGURE, /* 17 */
     PT_LINETO, PT_LINETO, PT_MOVETO | PT_CLOSEFIGURE, /* 20 */
     PT_LINETO, PT_MOVETO | PT_LINETO, PT_LINETO,  /* 23 */
     PT_MOVETO, PT_MOVETO, PT_MOVETO, PT_LINETO | PT_CLOSEFIGURE}; /* 27 */

static void test_polydraw(void)
{
    BOOL retb;
    POINT pos;
    HDC hdc = GetDC(0);
    HWND hwnd;

    MoveToEx( hdc, -20, -20, NULL );

    BeginPath(hdc);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == -20 && pos.y == -20, "wrong pos %ld,%ld\n", pos.x, pos.y );

    /* closefigure with no previous moveto */
    if (!(retb = PolyDraw(hdc, polydraw_pts, polydraw_tps, 2)) &&
        GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        /* PolyDraw is only available on Win2k and later */
        win_skip("PolyDraw is not available\n");
        ReleaseDC(0, hdc);
        return;
    }
    expect(TRUE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 10 && pos.y == 15, "wrong pos %ld,%ld\n", pos.x, pos.y );
    LineTo(hdc, -10, -10);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == -10 && pos.y == -10, "wrong pos %ld,%ld\n", pos.x, pos.y );

    MoveToEx(hdc, 100, 100, NULL);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 100 && pos.y == 100, "wrong pos %ld,%ld\n", pos.x, pos.y );
    LineTo(hdc, 95, 95);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 95 && pos.y == 95, "wrong pos %ld,%ld\n", pos.x, pos.y );
    /* closefigure with previous moveto */
    retb = PolyDraw(hdc, polydraw_pts, polydraw_tps, 2);
    expect(TRUE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 10 && pos.y == 15, "wrong pos %ld,%ld\n", pos.x, pos.y );
    /* bad bezier points */
    retb = PolyDraw(hdc, &(polydraw_pts[2]), &(polydraw_tps[2]), 4);
    expect(FALSE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 10 && pos.y == 15, "wrong pos %ld,%ld\n", pos.x, pos.y );
    retb = PolyDraw(hdc, &(polydraw_pts[6]), &(polydraw_tps[6]), 4);
    expect(FALSE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 10 && pos.y == 15, "wrong pos %ld,%ld\n", pos.x, pos.y );
    /* good bezier points */
    retb = PolyDraw(hdc, &(polydraw_pts[8]), &(polydraw_tps[8]), 4);
    expect(TRUE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 35 && pos.y == 40, "wrong pos %ld,%ld\n", pos.x, pos.y );
    /* does lineto or bezierto take precedence? */
    retb = PolyDraw(hdc, &(polydraw_pts[12]), &(polydraw_tps[12]), 4);
    expect(FALSE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 35 && pos.y == 40, "wrong pos %ld,%ld\n", pos.x, pos.y );
    /* bad point type, has already moved cursor position */
    retb = PolyDraw(hdc, &(polydraw_pts[15]), &(polydraw_tps[15]), 4);
    expect(FALSE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 35 && pos.y == 40, "wrong pos %ld,%ld\n", pos.x, pos.y );
    /* bad point type, cursor position is moved, but back to its original spot */
    retb = PolyDraw(hdc, &(polydraw_pts[17]), &(polydraw_tps[17]), 4);
    expect(FALSE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 35 && pos.y == 40, "wrong pos %ld,%ld\n", pos.x, pos.y );
    /* does lineto or moveto take precedence? */
    retb = PolyDraw(hdc, &(polydraw_pts[20]), &(polydraw_tps[20]), 3);
    expect(TRUE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 65 && pos.y == 65, "wrong pos %ld,%ld\n", pos.x, pos.y );
    /* consecutive movetos */
    retb = PolyDraw(hdc, &(polydraw_pts[23]), &(polydraw_tps[23]), 4);
    expect(TRUE, retb);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 80 && pos.y == 80, "wrong pos %ld,%ld\n", pos.x, pos.y );

    EndPath(hdc);
    ok_path(hdc, "polydraw_path", polydraw_path, ARRAY_SIZE(polydraw_path));
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 80 && pos.y == 80, "wrong pos %ld,%ld\n", pos.x, pos.y );
    ReleaseDC(0, hdc);

    /* Test a special case that GDI path driver is created before window driver */
    hwnd = CreateWindowA("static", NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    hdc = GetDC(hwnd);

    BeginPath(hdc);
    SetWindowPos(hwnd, 0, 0, 0, 100, 100, SWP_NOZORDER | SWP_NOMOVE | SWP_SHOWWINDOW);
    retb = PolyDraw(hdc, polydraw_pts, polydraw_tps, 2);
    ok(retb, "PolyDraw failed, error %#lx\n", GetLastError());
    EndPath(hdc);

    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);
}

static void test_closefigure(void) {
    int nSize, nSizeWitness;
    POINT pos;
    HDC hdc = GetDC(0);

    MoveToEx( hdc, 100, 100, NULL );
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 100 && pos.y == 100, "wrong pos %ld,%ld\n", pos.x, pos.y );

    BeginPath(hdc);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 100 && pos.y == 100, "wrong pos %ld,%ld\n", pos.x, pos.y );
    MoveToEx(hdc, 95, 95, NULL);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 95 && pos.y == 95, "wrong pos %ld,%ld\n", pos.x, pos.y );
    LineTo(hdc, 95,  0);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 95 && pos.y == 0, "wrong pos %ld,%ld\n", pos.x, pos.y );
    LineTo(hdc,  0, 95);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 0 && pos.y == 95, "wrong pos %ld,%ld\n", pos.x, pos.y );

    CloseFigure(hdc);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 0 && pos.y == 95, "wrong pos %ld,%ld\n", pos.x, pos.y );
    EndPath(hdc);
    GetCurrentPositionEx( hdc, &pos );
    ok( pos.x == 0 && pos.y == 95, "wrong pos %ld,%ld\n", pos.x, pos.y );
    nSize = GetPath(hdc, NULL, NULL, 0);

    AbortPath(hdc);

    BeginPath(hdc);
    MoveToEx(hdc, 95, 95, NULL);
    LineTo(hdc, 95,  0);
    LineTo(hdc,  0, 95);

    EndPath(hdc);
    nSizeWitness = GetPath(hdc, NULL, NULL, 0);

    /* This test shows CloseFigure does not have to add a point at the end of the path */
    ok(nSize == nSizeWitness, "Wrong number of points, no point should be added by CloseFigure\n");

    ReleaseDC(0, hdc);
}

static void WINAPI linedda_callback(INT x, INT y, LPARAM lparam)
{
    POINT **pt = (POINT**)lparam;
    ok((*pt)->x == x && (*pt)->y == y, "point mismatch expect(%ld,%ld) got(%d,%d)\n",
       (*pt)->x, (*pt)->y, x, y);

    (*pt)++;
    return;
}

static void test_linedda(void)
{
    const POINT *pt;
    static const POINT array_10_20_20_40[] = {{10,20},{10,21},{11,22},{11,23},
                                              {12,24},{12,25},{13,26},{13,27},
                                              {14,28},{14,29},{15,30},{15,31},
                                              {16,32},{16,33},{17,34},{17,35},
                                              {18,36},{18,37},{19,38},{19,39},
                                              {-1,-1}};
    static const POINT array_10_20_20_43[] = {{10,20},{10,21},{11,22},{11,23},
                                              {12,24},{12,25},{13,26},{13,27},
                                              {13,28},{14,29},{14,30},{15,31},
                                              {15,32},{16,33},{16,34},{17,35},
                                              {17,36},{17,37},{18,38},{18,39},
                                              {19,40},{19,41},{20,42},{-1,-1}};

    static const POINT array_10_20_10_20[] = {{-1,-1}};
    static const POINT array_10_20_11_27[] = {{10,20},{10,21},{10,22},{10,23},
                                              {11,24},{11,25},{11,26},{-1,-1}};

    static const POINT array_20_43_10_20[] = {{20,43},{20,42},{19,41},{19,40},
                                              {18,39},{18,38},{17,37},{17,36},
                                              {17,35},{16,34},{16,33},{15,32},
                                              {15,31},{14,30},{14,29},{13,28},
                                              {13,27},{13,26},{12,25},{12,24},
                                              {11,23},{11,22},{10,21},{-1,-1}};

    static const POINT array_20_20_10_43[] = {{20,20},{20,21},{19,22},{19,23},
                                              {18,24},{18,25},{17,26},{17,27},
                                              {17,28},{16,29},{16,30},{15,31},
                                              {15,32},{14,33},{14,34},{13,35},
                                              {13,36},{13,37},{12,38},{12,39},
                                              {11,40},{11,41},{10,42},{-1,-1}};

    static const POINT array_20_20_43_10[] = {{20,20},{21,20},{22,19},{23,19},
                                              {24,18},{25,18},{26,17},{27,17},
                                              {28,17},{29,16},{30,16},{31,15},
                                              {32,15},{33,14},{34,14},{35,13},
                                              {36,13},{37,13},{38,12},{39,12},
                                              {40,11},{41,11},{42,10},{-1,-1}};


    pt = array_10_20_20_40;
    LineDDA(10, 20, 20, 40, linedda_callback, (LPARAM)&pt);
    ok(pt->x == -1 && pt->y == -1, "didn't find terminator\n");

    pt = array_10_20_20_43;
    LineDDA(10, 20, 20, 43, linedda_callback, (LPARAM)&pt);
    ok(pt->x == -1 && pt->y == -1, "didn't find terminator\n");

    pt = array_10_20_10_20;
    LineDDA(10, 20, 10, 20, linedda_callback, (LPARAM)&pt);
    ok(pt->x == -1 && pt->y == -1, "didn't find terminator\n");

    pt = array_10_20_11_27;
    LineDDA(10, 20, 11, 27, linedda_callback, (LPARAM)&pt);
    ok(pt->x == -1 && pt->y == -1, "didn't find terminator\n");

    pt = array_20_43_10_20;
    LineDDA(20, 43, 10, 20, linedda_callback, (LPARAM)&pt);
    ok(pt->x == -1 && pt->y == -1, "didn't find terminator\n");

    pt = array_20_20_10_43;
    LineDDA(20, 20, 10, 43, linedda_callback, (LPARAM)&pt);
    ok(pt->x == -1 && pt->y == -1, "didn't find terminator\n");

    pt = array_20_20_43_10;
    LineDDA(20, 20, 43, 10, linedda_callback, (LPARAM)&pt);
    ok(pt->x == -1 && pt->y == -1, "didn't find terminator\n");
}

static const path_test_t rectangle_path[] =
{
    {39, 20, PT_MOVETO}, /* 0 */
    {20, 20, PT_LINETO}, /* 1 */
    {20, 39, PT_LINETO}, /* 2 */
    {39, 39, PT_LINETO | PT_CLOSEFIGURE}, /* 3 */
    {54, 35, PT_MOVETO}, /* 4 */
    {30, 35, PT_LINETO}, /* 5 */
    {30, 49, PT_LINETO}, /* 6 */
    {54, 49, PT_LINETO | PT_CLOSEFIGURE}, /* 7 */
    {59, 45, PT_MOVETO}, /* 8 */
    {35, 45, PT_LINETO}, /* 9 */
    {35, 59, PT_LINETO}, /* 10 */
    {59, 59, PT_LINETO | PT_CLOSEFIGURE}, /* 11 */
    {80, 80, PT_MOVETO}, /* 12 */
    {80, 80, PT_LINETO}, /* 13 */
    {80, 80, PT_LINETO}, /* 14 */
    {80, 80, PT_LINETO | PT_CLOSEFIGURE}, /* 15 */
    {39, 39, PT_MOVETO}, /* 16 */
    {20, 39, PT_LINETO}, /* 17 */
    {20, 20, PT_LINETO}, /* 18 */
    {39, 20, PT_LINETO | PT_CLOSEFIGURE}, /* 19 */
    {54, 49, PT_MOVETO}, /* 20 */
    {30, 49, PT_LINETO}, /* 21 */
    {30, 35, PT_LINETO}, /* 22 */
    {54, 35, PT_LINETO | PT_CLOSEFIGURE}, /* 23 */
    {59, 59, PT_MOVETO}, /* 24 */
    {35, 59, PT_LINETO}, /* 25 */
    {35, 45, PT_LINETO}, /* 26 */
    {59, 45, PT_LINETO | PT_CLOSEFIGURE}, /* 27 */
    {80, 80, PT_MOVETO}, /* 28 */
    {80, 80, PT_LINETO}, /* 29 */
    {80, 80, PT_LINETO}, /* 30 */
    {80, 80, PT_LINETO | PT_CLOSEFIGURE}, /* 31 */
    {-41, 40, PT_MOVETO}, /* 32 */
    {-80, 40, PT_LINETO}, /* 33 */
    {-80, 79, PT_LINETO}, /* 34 */
    {-41, 79, PT_LINETO | PT_CLOSEFIGURE}, /* 35 */
    {-61, 70, PT_MOVETO}, /* 36 */
    {-110, 70, PT_LINETO}, /* 37 */
    {-110, 99, PT_LINETO}, /* 38 */
    {-61, 99, PT_LINETO | PT_CLOSEFIGURE}, /* 39 */
    {119, -120, PT_MOVETO}, /* 40 */
    {60, -120, PT_LINETO}, /* 41 */
    {60, -61, PT_LINETO}, /* 42 */
    {119, -61, PT_LINETO | PT_CLOSEFIGURE}, /* 43 */
    {164, -150, PT_MOVETO}, /* 44 */
    {90, -150, PT_LINETO}, /* 45 */
    {90, -106, PT_LINETO}, /* 46 */
    {164, -106, PT_LINETO | PT_CLOSEFIGURE}, /* 47 */
    {-4, -6, PT_MOVETO}, /* 48 */
    {-6, -6, PT_LINETO}, /* 49 */
    {-6, -4, PT_LINETO}, /* 50 */
    {-4, -4, PT_LINETO | PT_CLOSEFIGURE}, /* 51 */
    {40, 20, PT_MOVETO}, /* 52 */
    {20, 20, PT_LINETO}, /* 53 */
    {20, 40, PT_LINETO}, /* 54 */
    {40, 40, PT_LINETO | PT_CLOSEFIGURE}, /* 55 */
    {55, 35, PT_MOVETO}, /* 56 */
    {30, 35, PT_LINETO}, /* 57 */
    {30, 50, PT_LINETO}, /* 58 */
    {55, 50, PT_LINETO | PT_CLOSEFIGURE}, /* 59 */
    {60, 45, PT_MOVETO}, /* 60 */
    {35, 45, PT_LINETO}, /* 61 */
    {35, 60, PT_LINETO}, /* 62 */
    {60, 60, PT_LINETO | PT_CLOSEFIGURE}, /* 63 */
    {70, 70, PT_MOVETO}, /* 64 */
    {50, 70, PT_LINETO}, /* 65 */
    {50, 70, PT_LINETO}, /* 66 */
    {70, 70, PT_LINETO | PT_CLOSEFIGURE}, /* 67 */
    {75, 75, PT_MOVETO}, /* 68 */
    {75, 75, PT_LINETO}, /* 69 */
    {75, 85, PT_LINETO}, /* 70 */
    {75, 85, PT_LINETO | PT_CLOSEFIGURE}, /* 71 */
    {81, 80, PT_MOVETO}, /* 72 */
    {80, 80, PT_LINETO}, /* 73 */
    {80, 81, PT_LINETO}, /* 74 */
    {81, 81, PT_LINETO | PT_CLOSEFIGURE}, /* 75 */
    {40, 40, PT_MOVETO}, /* 76 */
    {20, 40, PT_LINETO}, /* 77 */
    {20, 20, PT_LINETO}, /* 78 */
    {40, 20, PT_LINETO | PT_CLOSEFIGURE}, /* 79 */
    {55, 50, PT_MOVETO}, /* 80 */
    {30, 50, PT_LINETO}, /* 81 */
    {30, 35, PT_LINETO}, /* 82 */
    {55, 35, PT_LINETO | PT_CLOSEFIGURE}, /* 83 */
    {60, 60, PT_MOVETO}, /* 84 */
    {35, 60, PT_LINETO}, /* 85 */
    {35, 45, PT_LINETO}, /* 86 */
    {60, 45, PT_LINETO | PT_CLOSEFIGURE}, /* 87 */
    {70, 70, PT_MOVETO}, /* 88 */
    {50, 70, PT_LINETO}, /* 89 */
    {50, 70, PT_LINETO}, /* 90 */
    {70, 70, PT_LINETO | PT_CLOSEFIGURE}, /* 91 */
    {75, 85, PT_MOVETO}, /* 92 */
    {75, 85, PT_LINETO}, /* 93 */
    {75, 75, PT_LINETO}, /* 94 */
    {75, 75, PT_LINETO | PT_CLOSEFIGURE}, /* 95 */
    {81, 81, PT_MOVETO}, /* 96 */
    {80, 81, PT_LINETO}, /* 97 */
    {80, 80, PT_LINETO}, /* 98 */
    {81, 80, PT_LINETO | PT_CLOSEFIGURE}, /* 99 */
};

static void test_rectangle(void)
{
    HDC hdc = GetDC( 0 );

    BeginPath( hdc );
    Rectangle( hdc, 20, 20, 40, 40 );
    Rectangle( hdc, 30, 50, 55, 35 );
    Rectangle( hdc, 60, 60, 35, 45 );
    Rectangle( hdc, 70, 70, 50, 70 );
    Rectangle( hdc, 75, 75, 75, 85 );
    Rectangle( hdc, 80, 80, 81, 81 );
    SetArcDirection( hdc, AD_CLOCKWISE );
    Rectangle( hdc, 20, 20, 40, 40 );
    Rectangle( hdc, 30, 50, 55, 35 );
    Rectangle( hdc, 60, 60, 35, 45 );
    Rectangle( hdc, 70, 70, 50, 70 );
    Rectangle( hdc, 75, 75, 75, 85 );
    Rectangle( hdc, 80, 80, 81, 81 );
    SetArcDirection( hdc, AD_COUNTERCLOCKWISE );
    SetMapMode( hdc, MM_ANISOTROPIC );
    SetViewportExtEx( hdc, -2, 2, NULL );
    Rectangle( hdc, 20, 20, 40, 40 );
    Rectangle( hdc, 30, 50, 55, 35 );
    SetViewportExtEx( hdc, 3, -3, NULL );
    Rectangle( hdc, 20, 20, 40, 40 );
    Rectangle( hdc, 30, 50, 55, 35 );
    SetWindowExtEx( hdc, -20, 20, NULL );
    Rectangle( hdc, 20, 20, 40, 40 );
    Rectangle( hdc, 24, 22, 21, 20 );
    SetMapMode( hdc, MM_TEXT );
    SetGraphicsMode( hdc, GM_ADVANCED );
    Rectangle( hdc, 20, 20, 40, 40 );
    Rectangle( hdc, 30, 50, 55, 35 );
    Rectangle( hdc, 60, 60, 35, 45 );
    Rectangle( hdc, 70, 70, 50, 70 );
    Rectangle( hdc, 75, 75, 75, 85 );
    Rectangle( hdc, 80, 80, 81, 81 );
    SetArcDirection( hdc, AD_CLOCKWISE );
    Rectangle( hdc, 20, 20, 40, 40 );
    Rectangle( hdc, 30, 50, 55, 35 );
    Rectangle( hdc, 60, 60, 35, 45 );
    Rectangle( hdc, 70, 70, 50, 70 );
    Rectangle( hdc, 75, 75, 75, 85 );
    Rectangle( hdc, 80, 80, 81, 81 );
    SetArcDirection( hdc, AD_COUNTERCLOCKWISE );
    EndPath( hdc );
    SetMapMode( hdc, MM_TEXT );
    ok_path( hdc, "rectangle_path", rectangle_path, ARRAY_SIZE(rectangle_path) );
    ReleaseDC( 0, hdc );
}

static const path_test_t roundrect_path[] =
{
    {39, 25, PT_MOVETO}, /* 0 */
    {39, 22, PT_BEZIERTO}, /* 1 */
    {37, 20, PT_BEZIERTO}, /* 2 */
    {34, 20, PT_BEZIERTO}, /* 3 */
    {25, 20, PT_LINETO}, /* 4 */
    {22, 20, PT_BEZIERTO}, /* 5 */
    {20, 22, PT_BEZIERTO}, /* 6 */
    {20, 25, PT_BEZIERTO}, /* 7 */
    {20, 34, PT_LINETO}, /* 8 */
    {20, 37, PT_BEZIERTO}, /* 9 */
    {22, 39, PT_BEZIERTO}, /* 10 */
    {25, 39, PT_BEZIERTO}, /* 11 */
    {34, 39, PT_LINETO}, /* 12 */
    {37, 39, PT_BEZIERTO}, /* 13 */
    {39, 37, PT_BEZIERTO}, /* 14 */
    {39, 34, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 15 */
    {54, 42, PT_MOVETO}, /* 16 */
    {54, 38, PT_BEZIERTO}, /* 17 */
    {49, 35, PT_BEZIERTO}, /* 18 */
    {42, 35, PT_BEZIERTO}, /* 19 */
    {42, 35, PT_LINETO}, /* 20 */
    {35, 35, PT_BEZIERTO}, /* 21 */
    {30, 38, PT_BEZIERTO}, /* 22 */
    {30, 42, PT_BEZIERTO}, /* 23 */
    {30, 42, PT_LINETO}, /* 24 */
    {30, 46, PT_BEZIERTO}, /* 25 */
    {35, 49, PT_BEZIERTO}, /* 26 */
    {42, 49, PT_BEZIERTO}, /* 27 */
    {42, 49, PT_LINETO}, /* 28 */
    {49, 49, PT_BEZIERTO}, /* 29 */
    {54, 46, PT_BEZIERTO}, /* 30 */
    {54, 42, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 31 */
    {59, 46, PT_MOVETO}, /* 32 */
    {59, 45, PT_BEZIERTO}, /* 33 */
    {58, 45, PT_BEZIERTO}, /* 34 */
    {57, 45, PT_BEZIERTO}, /* 35 */
    {37, 45, PT_LINETO}, /* 36 */
    {36, 45, PT_BEZIERTO}, /* 37 */
    {35, 45, PT_BEZIERTO}, /* 38 */
    {35, 46, PT_BEZIERTO}, /* 39 */
    {35, 58, PT_LINETO}, /* 40 */
    {35, 59, PT_BEZIERTO}, /* 41 */
    {36, 59, PT_BEZIERTO}, /* 42 */
    {37, 59, PT_BEZIERTO}, /* 43 */
    {57, 59, PT_LINETO}, /* 44 */
    {58, 59, PT_BEZIERTO}, /* 45 */
    {59, 59, PT_BEZIERTO}, /* 46 */
    {59, 58, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 47 */
    {80, 80, PT_MOVETO}, /* 48 */
    {80, 80, PT_BEZIERTO}, /* 49 */
    {80, 80, PT_BEZIERTO}, /* 50 */
    {80, 80, PT_BEZIERTO}, /* 51 */
    {80, 80, PT_LINETO}, /* 52 */
    {80, 80, PT_BEZIERTO}, /* 53 */
    {80, 80, PT_BEZIERTO}, /* 54 */
    {80, 80, PT_BEZIERTO}, /* 55 */
    {80, 80, PT_LINETO}, /* 56 */
    {80, 80, PT_BEZIERTO}, /* 57 */
    {80, 80, PT_BEZIERTO}, /* 58 */
    {80, 80, PT_BEZIERTO}, /* 59 */
    {80, 80, PT_LINETO}, /* 60 */
    {80, 80, PT_BEZIERTO}, /* 61 */
    {80, 80, PT_BEZIERTO}, /* 62 */
    {80, 80, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 63 */
    {94, 85, PT_MOVETO}, /* 64 */
    {90, 85, PT_LINETO}, /* 65 */
    {90, 89, PT_LINETO}, /* 66 */
    {94, 89, PT_LINETO | PT_CLOSEFIGURE}, /* 67 */
    {39, 34, PT_MOVETO}, /* 68 */
    {39, 37, PT_BEZIERTO}, /* 69 */
    {37, 39, PT_BEZIERTO}, /* 70 */
    {34, 39, PT_BEZIERTO}, /* 71 */
    {25, 39, PT_LINETO}, /* 72 */
    {22, 39, PT_BEZIERTO}, /* 73 */
    {20, 37, PT_BEZIERTO}, /* 74 */
    {20, 34, PT_BEZIERTO}, /* 75 */
    {20, 25, PT_LINETO}, /* 76 */
    {20, 22, PT_BEZIERTO}, /* 77 */
    {22, 20, PT_BEZIERTO}, /* 78 */
    {25, 20, PT_BEZIERTO}, /* 79 */
    {34, 20, PT_LINETO}, /* 80 */
    {37, 20, PT_BEZIERTO}, /* 81 */
    {39, 22, PT_BEZIERTO}, /* 82 */
    {39, 25, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 83 */
    {54, 42, PT_MOVETO}, /* 84 */
    {54, 46, PT_BEZIERTO}, /* 85 */
    {49, 49, PT_BEZIERTO}, /* 86 */
    {42, 49, PT_BEZIERTO}, /* 87 */
    {42, 49, PT_LINETO}, /* 88 */
    {35, 49, PT_BEZIERTO}, /* 89 */
    {30, 46, PT_BEZIERTO}, /* 90 */
    {30, 42, PT_BEZIERTO}, /* 91 */
    {30, 42, PT_LINETO}, /* 92 */
    {30, 38, PT_BEZIERTO}, /* 93 */
    {35, 35, PT_BEZIERTO}, /* 94 */
    {42, 35, PT_BEZIERTO}, /* 95 */
    {42, 35, PT_LINETO}, /* 96 */
    {49, 35, PT_BEZIERTO}, /* 97 */
    {54, 38, PT_BEZIERTO}, /* 98 */
    {54, 42, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 99 */
    {-41, 52, PT_MOVETO}, /* 100 */
    {-41, 45, PT_BEZIERTO}, /* 101 */
    {-47, 40, PT_BEZIERTO}, /* 102 */
    {-56, 40, PT_BEZIERTO}, /* 103 */
    {-65, 40, PT_LINETO}, /* 104 */
    {-73, 40, PT_BEZIERTO}, /* 105 */
    {-80, 45, PT_BEZIERTO}, /* 106 */
    {-80, 52, PT_BEZIERTO}, /* 107 */
    {-80, 67, PT_LINETO}, /* 108 */
    {-80, 74, PT_BEZIERTO}, /* 109 */
    {-73, 79, PT_BEZIERTO}, /* 110 */
    {-65, 79, PT_BEZIERTO}, /* 111 */
    {-56, 79, PT_LINETO}, /* 112 */
    {-47, 79, PT_BEZIERTO}, /* 113 */
    {-41, 74, PT_BEZIERTO}, /* 114 */
    {-41, 67, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 115 */
    {-61, 79, PT_MOVETO}, /* 116 */
    {-61, 74, PT_BEZIERTO}, /* 117 */
    {-64, 70, PT_BEZIERTO}, /* 118 */
    {-68, 70, PT_BEZIERTO}, /* 119 */
    {-103, 70, PT_LINETO}, /* 120 */
    {-107, 70, PT_BEZIERTO}, /* 121 */
    {-110, 74, PT_BEZIERTO}, /* 122 */
    {-110, 79, PT_BEZIERTO}, /* 123 */
    {-110, 90, PT_LINETO}, /* 124 */
    {-110, 95, PT_BEZIERTO}, /* 125 */
    {-107, 99, PT_BEZIERTO}, /* 126 */
    {-103, 99, PT_BEZIERTO}, /* 127 */
    {-68, 99, PT_LINETO}, /* 128 */
    {-64, 99, PT_BEZIERTO}, /* 129 */
    {-61, 95, PT_BEZIERTO}, /* 130 */
    {-61, 90, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 131 */
    {119, -102, PT_MOVETO}, /* 132 */
    {119, -112, PT_BEZIERTO}, /* 133 */
    {109, -120, PT_BEZIERTO}, /* 134 */
    {97, -120, PT_BEZIERTO}, /* 135 */
    {82, -120, PT_LINETO}, /* 136 */
    {70, -120, PT_BEZIERTO}, /* 137 */
    {60, -112, PT_BEZIERTO}, /* 138 */
    {60, -102, PT_BEZIERTO}, /* 139 */
    {60, -79, PT_LINETO}, /* 140 */
    {60, -69, PT_BEZIERTO}, /* 141 */
    {70, -61, PT_BEZIERTO}, /* 142 */
    {82, -61, PT_BEZIERTO}, /* 143 */
    {97, -61, PT_LINETO}, /* 144 */
    {109, -61, PT_BEZIERTO}, /* 145 */
    {119, -69, PT_BEZIERTO}, /* 146 */
    {119, -79, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 147 */
    {164, -144, PT_MOVETO}, /* 148 */
    {164, -147, PT_BEZIERTO}, /* 149 */
    {162, -150, PT_BEZIERTO}, /* 150 */
    {160, -150, PT_BEZIERTO}, /* 151 */
    {94, -150, PT_LINETO}, /* 152 */
    {92, -150, PT_BEZIERTO}, /* 153 */
    {90, -147, PT_BEZIERTO}, /* 154 */
    {90, -144, PT_BEZIERTO}, /* 155 */
    {90, -112, PT_LINETO}, /* 156 */
    {90, -109, PT_BEZIERTO}, /* 157 */
    {92, -106, PT_BEZIERTO}, /* 158 */
    {94, -106, PT_BEZIERTO}, /* 159 */
    {160, -106, PT_LINETO}, /* 160 */
    {162, -106, PT_BEZIERTO}, /* 161 */
    {164, -109, PT_BEZIERTO}, /* 162 */
    {164, -112, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 163 */
    {-4, -6, PT_MOVETO}, /* 164 */
    {-4, -6, PT_BEZIERTO}, /* 165 */
    {-4, -6, PT_BEZIERTO}, /* 166 */
    {-4, -6, PT_BEZIERTO}, /* 167 */
    {-6, -6, PT_LINETO}, /* 168 */
    {-6, -6, PT_BEZIERTO}, /* 169 */
    {-6, -6, PT_BEZIERTO}, /* 170 */
    {-6, -6, PT_BEZIERTO}, /* 171 */
    {-6, -4, PT_LINETO}, /* 172 */
    {-6, -4, PT_BEZIERTO}, /* 173 */
    {-6, -4, PT_BEZIERTO}, /* 174 */
    {-6, -4, PT_BEZIERTO}, /* 175 */
    {-4, -4, PT_LINETO}, /* 176 */
    {-4, -4, PT_BEZIERTO}, /* 177 */
    {-4, -4, PT_BEZIERTO}, /* 178 */
    {-4, -4, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 179 */
    {40, 25, PT_MOVETO}, /* 180 */
    {40, 22, PT_BEZIERTO}, /* 181 */
    {38, 20, PT_BEZIERTO}, /* 182 */
    {35, 20, PT_BEZIERTO}, /* 183 */
    {25, 20, PT_LINETO}, /* 184 */
    {22, 20, PT_BEZIERTO}, /* 185 */
    {20, 22, PT_BEZIERTO}, /* 186 */
    {20, 25, PT_BEZIERTO}, /* 187 */
    {20, 35, PT_LINETO}, /* 188 */
    {20, 38, PT_BEZIERTO}, /* 189 */
    {22, 40, PT_BEZIERTO}, /* 190 */
    {25, 40, PT_BEZIERTO}, /* 191 */
    {35, 40, PT_LINETO}, /* 192 */
    {38, 40, PT_BEZIERTO}, /* 193 */
    {40, 38, PT_BEZIERTO}, /* 194 */
    {40, 35, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 195 */
    {55, 43, PT_MOVETO}, /* 196 */
    {55, 38, PT_BEZIERTO}, /* 197 */
    {49, 35, PT_BEZIERTO}, /* 198 */
    {43, 35, PT_BEZIERTO}, /* 199 */
    {43, 35, PT_LINETO}, /* 200 */
    {36, 35, PT_BEZIERTO}, /* 201 */
    {30, 38, PT_BEZIERTO}, /* 202 */
    {30, 43, PT_BEZIERTO}, /* 203 */
    {30, 43, PT_LINETO}, /* 204 */
    {30, 47, PT_BEZIERTO}, /* 205 */
    {36, 50, PT_BEZIERTO}, /* 206 */
    {43, 50, PT_BEZIERTO}, /* 207 */
    {43, 50, PT_LINETO}, /* 208 */
    {49, 50, PT_BEZIERTO}, /* 209 */
    {55, 47, PT_BEZIERTO}, /* 210 */
    {55, 43, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 211 */
    {60, 46, PT_MOVETO}, /* 212 */
    {60, 46, PT_BEZIERTO}, /* 213 */
    {59, 45, PT_BEZIERTO}, /* 214 */
    {58, 45, PT_BEZIERTO}, /* 215 */
    {38, 45, PT_LINETO}, /* 216 */
    {36, 45, PT_BEZIERTO}, /* 217 */
    {35, 46, PT_BEZIERTO}, /* 218 */
    {35, 46, PT_BEZIERTO}, /* 219 */
    {35, 59, PT_LINETO}, /* 220 */
    {35, 60, PT_BEZIERTO}, /* 221 */
    {36, 60, PT_BEZIERTO}, /* 222 */
    {38, 60, PT_BEZIERTO}, /* 223 */
    {58, 60, PT_LINETO}, /* 224 */
    {59, 60, PT_BEZIERTO}, /* 225 */
    {60, 60, PT_BEZIERTO}, /* 226 */
    {60, 59, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 227 */
    {70, 70, PT_MOVETO}, /* 228 */
    {70, 70, PT_BEZIERTO}, /* 229 */
    {70, 70, PT_BEZIERTO}, /* 230 */
    {70, 70, PT_BEZIERTO}, /* 231 */
    {50, 70, PT_LINETO}, /* 232 */
    {50, 70, PT_BEZIERTO}, /* 233 */
    {50, 70, PT_BEZIERTO}, /* 234 */
    {50, 70, PT_BEZIERTO}, /* 235 */
    {50, 70, PT_LINETO}, /* 236 */
    {50, 70, PT_BEZIERTO}, /* 237 */
    {50, 70, PT_BEZIERTO}, /* 238 */
    {50, 70, PT_BEZIERTO}, /* 239 */
    {70, 70, PT_LINETO}, /* 240 */
    {70, 70, PT_BEZIERTO}, /* 241 */
    {70, 70, PT_BEZIERTO}, /* 242 */
    {70, 70, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 243 */
    {75, 75, PT_MOVETO}, /* 244 */
    {75, 75, PT_BEZIERTO}, /* 245 */
    {75, 75, PT_BEZIERTO}, /* 246 */
    {75, 75, PT_BEZIERTO}, /* 247 */
    {75, 75, PT_LINETO}, /* 248 */
    {75, 75, PT_BEZIERTO}, /* 249 */
    {75, 75, PT_BEZIERTO}, /* 250 */
    {75, 75, PT_BEZIERTO}, /* 251 */
    {75, 85, PT_LINETO}, /* 252 */
    {75, 85, PT_BEZIERTO}, /* 253 */
    {75, 85, PT_BEZIERTO}, /* 254 */
    {75, 85, PT_BEZIERTO}, /* 255 */
    {75, 85, PT_LINETO}, /* 256 */
    {75, 85, PT_BEZIERTO}, /* 257 */
    {75, 85, PT_BEZIERTO}, /* 258 */
    {75, 85, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 259 */
    {81, 81, PT_MOVETO}, /* 260 */
    {81, 80, PT_BEZIERTO}, /* 261 */
    {81, 80, PT_BEZIERTO}, /* 262 */
    {81, 80, PT_BEZIERTO}, /* 263 */
    {81, 80, PT_LINETO}, /* 264 */
    {80, 80, PT_BEZIERTO}, /* 265 */
    {80, 80, PT_BEZIERTO}, /* 266 */
    {80, 81, PT_BEZIERTO}, /* 267 */
    {80, 81, PT_LINETO}, /* 268 */
    {80, 81, PT_BEZIERTO}, /* 269 */
    {80, 81, PT_BEZIERTO}, /* 270 */
    {81, 81, PT_BEZIERTO}, /* 271 */
    {81, 81, PT_LINETO}, /* 272 */
    {81, 81, PT_BEZIERTO}, /* 273 */
    {81, 81, PT_BEZIERTO}, /* 274 */
    {81, 81, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 275 */
    {95, 85, PT_MOVETO}, /* 276 */
    {90, 85, PT_LINETO}, /* 277 */
    {90, 90, PT_LINETO}, /* 278 */
    {95, 90, PT_LINETO | PT_CLOSEFIGURE}, /* 279 */
    {40, 35, PT_MOVETO}, /* 280 */
    {40, 38, PT_BEZIERTO}, /* 281 */
    {38, 40, PT_BEZIERTO}, /* 282 */
    {35, 40, PT_BEZIERTO}, /* 283 */
    {25, 40, PT_LINETO}, /* 284 */
    {22, 40, PT_BEZIERTO}, /* 285 */
    {20, 38, PT_BEZIERTO}, /* 286 */
    {20, 35, PT_BEZIERTO}, /* 287 */
    {20, 25, PT_LINETO}, /* 288 */
    {20, 22, PT_BEZIERTO}, /* 289 */
    {22, 20, PT_BEZIERTO}, /* 290 */
    {25, 20, PT_BEZIERTO}, /* 291 */
    {35, 20, PT_LINETO}, /* 292 */
    {38, 20, PT_BEZIERTO}, /* 293 */
    {40, 22, PT_BEZIERTO}, /* 294 */
    {40, 25, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 295 */
    {55, 43, PT_MOVETO}, /* 296 */
    {55, 47, PT_BEZIERTO}, /* 297 */
    {49, 50, PT_BEZIERTO}, /* 298 */
    {43, 50, PT_BEZIERTO}, /* 299 */
    {43, 50, PT_LINETO}, /* 300 */
    {36, 50, PT_BEZIERTO}, /* 301 */
    {30, 47, PT_BEZIERTO}, /* 302 */
    {30, 43, PT_BEZIERTO}, /* 303 */
    {30, 43, PT_LINETO}, /* 304 */
    {30, 38, PT_BEZIERTO}, /* 305 */
    {36, 35, PT_BEZIERTO}, /* 306 */
    {43, 35, PT_BEZIERTO}, /* 307 */
    {43, 35, PT_LINETO}, /* 308 */
    {49, 35, PT_BEZIERTO}, /* 309 */
    {55, 38, PT_BEZIERTO}, /* 310 */
    {55, 43, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 311 */
};

static void test_roundrect(void)
{
    HDC hdc = GetDC( 0 );

    BeginPath( hdc );
    RoundRect( hdc, 20, 20, 40, 40, 10, 10 );
    RoundRect( hdc, 30, 50, 55, 35, -30, -30 );
    RoundRect( hdc, 60, 60, 35, 45, 5, 2 );
    RoundRect( hdc, 70, 70, 50, 70, 3, 5 );
    RoundRect( hdc, 75, 75, 75, 85, 6, 4 );
    RoundRect( hdc, 80, 80, 81, 81, 8, 9 );
    RoundRect( hdc, 90, 90, 95, 85, 0, 7 );
    SetArcDirection( hdc, AD_CLOCKWISE );
    RoundRect( hdc, 20, 20, 40, 40, 10, 10 );
    RoundRect( hdc, 30, 50, 55, 35, -30, -30 );
    SetArcDirection( hdc, AD_COUNTERCLOCKWISE );
    SetMapMode( hdc, MM_ANISOTROPIC );
    SetViewportExtEx( hdc, -2, 2, NULL );
    RoundRect( hdc, 20, 20, 40, 40, 15, 12 );
    RoundRect( hdc, 30, 50, 55, 35, 7, 9 );
    SetViewportExtEx( hdc, 3, -3, NULL );
    RoundRect( hdc, 20, 20, 40, 40, 15, 12 );
    RoundRect( hdc, 30, 50, 55, 35, 3, 4 );
    SetWindowExtEx( hdc, -20, 20, NULL );
    RoundRect( hdc, 20, 20, 40, 40, 2, 1 );
    RoundRect( hdc, 24, 22, 21, 20, 4, 4 );
    SetMapMode( hdc, MM_TEXT );
    SetGraphicsMode( hdc, GM_ADVANCED );
    RoundRect( hdc, 20, 20, 40, 40, 10, 10 );
    RoundRect( hdc, 30, 50, 55, 35, -30, -30 );
    RoundRect( hdc, 60, 60, 35, 45, 5, 2 );
    RoundRect( hdc, 70, 70, 50, 70, 3, 5 );
    RoundRect( hdc, 75, 75, 75, 85, 6, 4 );
    RoundRect( hdc, 80, 80, 81, 81, 8, 9 );
    RoundRect( hdc, 90, 90, 95, 85, 0, 7 );
    SetArcDirection( hdc, AD_CLOCKWISE );
    RoundRect( hdc, 20, 20, 40, 40, 10, 10 );
    RoundRect( hdc, 30, 50, 55, 35, -30, -30 );
    SetArcDirection( hdc, AD_COUNTERCLOCKWISE );
    EndPath( hdc );
    SetMapMode( hdc, MM_TEXT );
    ok_path( hdc, "roundrect_path", roundrect_path, ARRAY_SIZE(roundrect_path) );
    ReleaseDC( 0, hdc );
}

static const path_test_t ellipse_path[] =
{
    {39, 30, PT_MOVETO}, /* 0 */
    {39, 24, PT_BEZIERTO}, /* 1 */
    {35, 20, PT_BEZIERTO}, /* 2 */
    {30, 20, PT_BEZIERTO}, /* 3 */
    {24, 20, PT_BEZIERTO}, /* 4 */
    {20, 24, PT_BEZIERTO}, /* 5 */
    {20, 30, PT_BEZIERTO}, /* 6 */
    {20, 35, PT_BEZIERTO}, /* 7 */
    {24, 39, PT_BEZIERTO}, /* 8 */
    {30, 39, PT_BEZIERTO}, /* 9 */
    {35, 39, PT_BEZIERTO}, /* 10 */
    {39, 35, PT_BEZIERTO}, /* 11 */
    {39, 30, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 12 */
    {54, 42, PT_MOVETO}, /* 13 */
    {54, 38, PT_BEZIERTO}, /* 14 */
    {49, 35, PT_BEZIERTO}, /* 15 */
    {42, 35, PT_BEZIERTO}, /* 16 */
    {35, 35, PT_BEZIERTO}, /* 17 */
    {30, 38, PT_BEZIERTO}, /* 18 */
    {30, 42, PT_BEZIERTO}, /* 19 */
    {30, 46, PT_BEZIERTO}, /* 20 */
    {35, 49, PT_BEZIERTO}, /* 21 */
    {42, 49, PT_BEZIERTO}, /* 22 */
    {49, 49, PT_BEZIERTO}, /* 23 */
    {54, 46, PT_BEZIERTO}, /* 24 */
    {54, 42, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 25 */
    {59, 52, PT_MOVETO}, /* 26 */
    {59, 48, PT_BEZIERTO}, /* 27 */
    {54, 45, PT_BEZIERTO}, /* 28 */
    {47, 45, PT_BEZIERTO}, /* 29 */
    {40, 45, PT_BEZIERTO}, /* 30 */
    {35, 48, PT_BEZIERTO}, /* 31 */
    {35, 52, PT_BEZIERTO}, /* 32 */
    {35, 56, PT_BEZIERTO}, /* 33 */
    {40, 59, PT_BEZIERTO}, /* 34 */
    {47, 59, PT_BEZIERTO}, /* 35 */
    {54, 59, PT_BEZIERTO}, /* 36 */
    {59, 56, PT_BEZIERTO}, /* 37 */
    {59, 52, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 38 */
    {80, 80, PT_MOVETO}, /* 39 */
    {80, 80, PT_BEZIERTO}, /* 40 */
    {80, 80, PT_BEZIERTO}, /* 41 */
    {80, 80, PT_BEZIERTO}, /* 42 */
    {80, 80, PT_BEZIERTO}, /* 43 */
    {80, 80, PT_BEZIERTO}, /* 44 */
    {80, 80, PT_BEZIERTO}, /* 45 */
    {80, 80, PT_BEZIERTO}, /* 46 */
    {80, 80, PT_BEZIERTO}, /* 47 */
    {80, 80, PT_BEZIERTO}, /* 48 */
    {80, 80, PT_BEZIERTO}, /* 49 */
    {80, 80, PT_BEZIERTO}, /* 50 */
    {80, 80, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 51 */
    {39, 30, PT_MOVETO}, /* 52 */
    {39, 35, PT_BEZIERTO}, /* 53 */
    {35, 39, PT_BEZIERTO}, /* 54 */
    {30, 39, PT_BEZIERTO}, /* 55 */
    {24, 39, PT_BEZIERTO}, /* 56 */
    {20, 35, PT_BEZIERTO}, /* 57 */
    {20, 30, PT_BEZIERTO}, /* 58 */
    {20, 24, PT_BEZIERTO}, /* 59 */
    {24, 20, PT_BEZIERTO}, /* 60 */
    {30, 20, PT_BEZIERTO}, /* 61 */
    {35, 20, PT_BEZIERTO}, /* 62 */
    {39, 24, PT_BEZIERTO}, /* 63 */
    {39, 30, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 64 */
    {54, 42, PT_MOVETO}, /* 65 */
    {54, 46, PT_BEZIERTO}, /* 66 */
    {49, 49, PT_BEZIERTO}, /* 67 */
    {42, 49, PT_BEZIERTO}, /* 68 */
    {35, 49, PT_BEZIERTO}, /* 69 */
    {30, 46, PT_BEZIERTO}, /* 70 */
    {30, 42, PT_BEZIERTO}, /* 71 */
    {30, 38, PT_BEZIERTO}, /* 72 */
    {35, 35, PT_BEZIERTO}, /* 73 */
    {42, 35, PT_BEZIERTO}, /* 74 */
    {49, 35, PT_BEZIERTO}, /* 75 */
    {54, 38, PT_BEZIERTO}, /* 76 */
    {54, 42, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 77 */
    {59, 52, PT_MOVETO}, /* 78 */
    {59, 56, PT_BEZIERTO}, /* 79 */
    {54, 59, PT_BEZIERTO}, /* 80 */
    {47, 59, PT_BEZIERTO}, /* 81 */
    {40, 59, PT_BEZIERTO}, /* 82 */
    {35, 56, PT_BEZIERTO}, /* 83 */
    {35, 52, PT_BEZIERTO}, /* 84 */
    {35, 48, PT_BEZIERTO}, /* 85 */
    {40, 45, PT_BEZIERTO}, /* 86 */
    {47, 45, PT_BEZIERTO}, /* 87 */
    {54, 45, PT_BEZIERTO}, /* 88 */
    {59, 48, PT_BEZIERTO}, /* 89 */
    {59, 52, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 90 */
    {80, 80, PT_MOVETO}, /* 91 */
    {80, 80, PT_BEZIERTO}, /* 92 */
    {80, 80, PT_BEZIERTO}, /* 93 */
    {80, 80, PT_BEZIERTO}, /* 94 */
    {80, 80, PT_BEZIERTO}, /* 95 */
    {80, 80, PT_BEZIERTO}, /* 96 */
    {80, 80, PT_BEZIERTO}, /* 97 */
    {80, 80, PT_BEZIERTO}, /* 98 */
    {80, 80, PT_BEZIERTO}, /* 99 */
    {80, 80, PT_BEZIERTO}, /* 100 */
    {80, 80, PT_BEZIERTO}, /* 101 */
    {80, 80, PT_BEZIERTO}, /* 102 */
    {80, 80, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 103 */
    {-41, 60, PT_MOVETO}, /* 104 */
    {-41, 49, PT_BEZIERTO}, /* 105 */
    {-50, 40, PT_BEZIERTO}, /* 106 */
    {-60, 40, PT_BEZIERTO}, /* 107 */
    {-71, 40, PT_BEZIERTO}, /* 108 */
    {-80, 49, PT_BEZIERTO}, /* 109 */
    {-80, 60, PT_BEZIERTO}, /* 110 */
    {-80, 70, PT_BEZIERTO}, /* 111 */
    {-71, 79, PT_BEZIERTO}, /* 112 */
    {-60, 79, PT_BEZIERTO}, /* 113 */
    {-50, 79, PT_BEZIERTO}, /* 114 */
    {-41, 70, PT_BEZIERTO}, /* 115 */
    {-41, 60, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 116 */
    {-61, 85, PT_MOVETO}, /* 117 */
    {-61, 77, PT_BEZIERTO}, /* 118 */
    {-72, 70, PT_BEZIERTO}, /* 119 */
    {-85, 70, PT_BEZIERTO}, /* 120 */
    {-99, 70, PT_BEZIERTO}, /* 121 */
    {-110, 77, PT_BEZIERTO}, /* 122 */
    {-110, 85, PT_BEZIERTO}, /* 123 */
    {-110, 93, PT_BEZIERTO}, /* 124 */
    {-99, 99, PT_BEZIERTO}, /* 125 */
    {-85, 99, PT_BEZIERTO}, /* 126 */
    {-72, 99, PT_BEZIERTO}, /* 127 */
    {-61, 93, PT_BEZIERTO}, /* 128 */
    {-61, 85, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 129 */
    {119, -90, PT_MOVETO}, /* 130 */
    {119, -107, PT_BEZIERTO}, /* 131 */
    {106, -120, PT_BEZIERTO}, /* 132 */
    {90, -120, PT_BEZIERTO}, /* 133 */
    {73, -120, PT_BEZIERTO}, /* 134 */
    {60, -107, PT_BEZIERTO}, /* 135 */
    {60, -90, PT_BEZIERTO}, /* 136 */
    {60, -74, PT_BEZIERTO}, /* 137 */
    {73, -61, PT_BEZIERTO}, /* 138 */
    {90, -61, PT_BEZIERTO}, /* 139 */
    {106, -61, PT_BEZIERTO}, /* 140 */
    {119, -74, PT_BEZIERTO}, /* 141 */
    {119, -90, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 142 */
    {164, -128, PT_MOVETO}, /* 143 */
    {164, -140, PT_BEZIERTO}, /* 144 */
    {147, -150, PT_BEZIERTO}, /* 145 */
    {127, -150, PT_BEZIERTO}, /* 146 */
    {107, -150, PT_BEZIERTO}, /* 147 */
    {90, -140, PT_BEZIERTO}, /* 148 */
    {90, -128, PT_BEZIERTO}, /* 149 */
    {90, -116, PT_BEZIERTO}, /* 150 */
    {107, -106, PT_BEZIERTO}, /* 151 */
    {127, -106, PT_BEZIERTO}, /* 152 */
    {147, -106, PT_BEZIERTO}, /* 153 */
    {164, -116, PT_BEZIERTO}, /* 154 */
    {164, -128, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 155 */
    {-4, -5, PT_MOVETO}, /* 156 */
    {-4, -5, PT_BEZIERTO}, /* 157 */
    {-4, -6, PT_BEZIERTO}, /* 158 */
    {-5, -6, PT_BEZIERTO}, /* 159 */
    {-6, -6, PT_BEZIERTO}, /* 160 */
    {-6, -5, PT_BEZIERTO}, /* 161 */
    {-6, -5, PT_BEZIERTO}, /* 162 */
    {-6, -4, PT_BEZIERTO}, /* 163 */
    {-6, -4, PT_BEZIERTO}, /* 164 */
    {-5, -4, PT_BEZIERTO}, /* 165 */
    {-4, -4, PT_BEZIERTO}, /* 166 */
    {-4, -4, PT_BEZIERTO}, /* 167 */
    {-4, -5, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 168 */
    {40, 30, PT_MOVETO}, /* 169 */
    {40, 25, PT_BEZIERTO}, /* 170 */
    {36, 20, PT_BEZIERTO}, /* 171 */
    {30, 20, PT_BEZIERTO}, /* 172 */
    {24, 20, PT_BEZIERTO}, /* 173 */
    {20, 25, PT_BEZIERTO}, /* 174 */
    {20, 30, PT_BEZIERTO}, /* 175 */
    {20, 36, PT_BEZIERTO}, /* 176 */
    {24, 40, PT_BEZIERTO}, /* 177 */
    {30, 40, PT_BEZIERTO}, /* 178 */
    {36, 40, PT_BEZIERTO}, /* 179 */
    {40, 36, PT_BEZIERTO}, /* 180 */
    {40, 30, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 181 */
    {55, 43, PT_MOVETO}, /* 182 */
    {55, 38, PT_BEZIERTO}, /* 183 */
    {49, 35, PT_BEZIERTO}, /* 184 */
    {43, 35, PT_BEZIERTO}, /* 185 */
    {36, 35, PT_BEZIERTO}, /* 186 */
    {30, 38, PT_BEZIERTO}, /* 187 */
    {30, 43, PT_BEZIERTO}, /* 188 */
    {30, 47, PT_BEZIERTO}, /* 189 */
    {36, 50, PT_BEZIERTO}, /* 190 */
    {43, 50, PT_BEZIERTO}, /* 191 */
    {49, 50, PT_BEZIERTO}, /* 192 */
    {55, 47, PT_BEZIERTO}, /* 193 */
    {55, 43, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 194 */
    {60, 53, PT_MOVETO}, /* 195 */
    {60, 48, PT_BEZIERTO}, /* 196 */
    {54, 45, PT_BEZIERTO}, /* 197 */
    {48, 45, PT_BEZIERTO}, /* 198 */
    {41, 45, PT_BEZIERTO}, /* 199 */
    {35, 48, PT_BEZIERTO}, /* 200 */
    {35, 53, PT_BEZIERTO}, /* 201 */
    {35, 57, PT_BEZIERTO}, /* 202 */
    {41, 60, PT_BEZIERTO}, /* 203 */
    {48, 60, PT_BEZIERTO}, /* 204 */
    {54, 60, PT_BEZIERTO}, /* 205 */
    {60, 57, PT_BEZIERTO}, /* 206 */
    {60, 53, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 207 */
    {70, 70, PT_MOVETO}, /* 208 */
    {70, 70, PT_BEZIERTO}, /* 209 */
    {66, 70, PT_BEZIERTO}, /* 210 */
    {60, 70, PT_BEZIERTO}, /* 211 */
    {54, 70, PT_BEZIERTO}, /* 212 */
    {50, 70, PT_BEZIERTO}, /* 213 */
    {50, 70, PT_BEZIERTO}, /* 214 */
    {50, 70, PT_BEZIERTO}, /* 215 */
    {54, 70, PT_BEZIERTO}, /* 216 */
    {60, 70, PT_BEZIERTO}, /* 217 */
    {66, 70, PT_BEZIERTO}, /* 218 */
    {70, 70, PT_BEZIERTO}, /* 219 */
    {70, 70, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 220 */
    {75, 80, PT_MOVETO}, /* 221 */
    {75, 77, PT_BEZIERTO}, /* 222 */
    {75, 75, PT_BEZIERTO}, /* 223 */
    {75, 75, PT_BEZIERTO}, /* 224 */
    {75, 75, PT_BEZIERTO}, /* 225 */
    {75, 77, PT_BEZIERTO}, /* 226 */
    {75, 80, PT_BEZIERTO}, /* 227 */
    {75, 83, PT_BEZIERTO}, /* 228 */
    {75, 85, PT_BEZIERTO}, /* 229 */
    {75, 85, PT_BEZIERTO}, /* 230 */
    {75, 85, PT_BEZIERTO}, /* 231 */
    {75, 83, PT_BEZIERTO}, /* 232 */
    {75, 80, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 233 */
    {81, 81, PT_MOVETO}, /* 234 */
    {81, 80, PT_BEZIERTO}, /* 235 */
    {81, 80, PT_BEZIERTO}, /* 236 */
    {81, 80, PT_BEZIERTO}, /* 237 */
    {80, 80, PT_BEZIERTO}, /* 238 */
    {80, 80, PT_BEZIERTO}, /* 239 */
    {80, 81, PT_BEZIERTO}, /* 240 */
    {80, 81, PT_BEZIERTO}, /* 241 */
    {80, 81, PT_BEZIERTO}, /* 242 */
    {81, 81, PT_BEZIERTO}, /* 243 */
    {81, 81, PT_BEZIERTO}, /* 244 */
    {81, 81, PT_BEZIERTO}, /* 245 */
    {81, 81, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 246 */
    {40, 30, PT_MOVETO}, /* 247 */
    {40, 36, PT_BEZIERTO}, /* 248 */
    {36, 40, PT_BEZIERTO}, /* 249 */
    {30, 40, PT_BEZIERTO}, /* 250 */
    {24, 40, PT_BEZIERTO}, /* 251 */
    {20, 36, PT_BEZIERTO}, /* 252 */
    {20, 30, PT_BEZIERTO}, /* 253 */
    {20, 24, PT_BEZIERTO}, /* 254 */
    {24, 20, PT_BEZIERTO}, /* 255 */
    {30, 20, PT_BEZIERTO}, /* 256 */
    {36, 20, PT_BEZIERTO}, /* 257 */
    {40, 24, PT_BEZIERTO}, /* 258 */
    {40, 30, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 259 */
    {55, 43, PT_MOVETO}, /* 260 */
    {55, 47, PT_BEZIERTO}, /* 261 */
    {49, 50, PT_BEZIERTO}, /* 262 */
    {43, 50, PT_BEZIERTO}, /* 263 */
    {36, 50, PT_BEZIERTO}, /* 264 */
    {30, 47, PT_BEZIERTO}, /* 265 */
    {30, 43, PT_BEZIERTO}, /* 266 */
    {30, 38, PT_BEZIERTO}, /* 267 */
    {36, 35, PT_BEZIERTO}, /* 268 */
    {43, 35, PT_BEZIERTO}, /* 269 */
    {49, 35, PT_BEZIERTO}, /* 270 */
    {55, 38, PT_BEZIERTO}, /* 271 */
    {55, 43, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 272 */
    {60, 53, PT_MOVETO}, /* 273 */
    {60, 57, PT_BEZIERTO}, /* 274 */
    {54, 60, PT_BEZIERTO}, /* 275 */
    {48, 60, PT_BEZIERTO}, /* 276 */
    {41, 60, PT_BEZIERTO}, /* 277 */
    {35, 57, PT_BEZIERTO}, /* 278 */
    {35, 53, PT_BEZIERTO}, /* 279 */
    {35, 48, PT_BEZIERTO}, /* 280 */
    {41, 45, PT_BEZIERTO}, /* 281 */
    {48, 45, PT_BEZIERTO}, /* 282 */
    {54, 45, PT_BEZIERTO}, /* 283 */
    {60, 48, PT_BEZIERTO}, /* 284 */
    {60, 53, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 285 */
    {70, 70, PT_MOVETO}, /* 286 */
    {70, 70, PT_BEZIERTO}, /* 287 */
    {66, 70, PT_BEZIERTO}, /* 288 */
    {60, 70, PT_BEZIERTO}, /* 289 */
    {54, 70, PT_BEZIERTO}, /* 290 */
    {50, 70, PT_BEZIERTO}, /* 291 */
    {50, 70, PT_BEZIERTO}, /* 292 */
    {50, 70, PT_BEZIERTO}, /* 293 */
    {54, 70, PT_BEZIERTO}, /* 294 */
    {60, 70, PT_BEZIERTO}, /* 295 */
    {66, 70, PT_BEZIERTO}, /* 296 */
    {70, 70, PT_BEZIERTO}, /* 297 */
    {70, 70, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 298 */
    {75, 80, PT_MOVETO}, /* 299 */
    {75, 83, PT_BEZIERTO}, /* 300 */
    {75, 85, PT_BEZIERTO}, /* 301 */
    {75, 85, PT_BEZIERTO}, /* 302 */
    {75, 85, PT_BEZIERTO}, /* 303 */
    {75, 83, PT_BEZIERTO}, /* 304 */
    {75, 80, PT_BEZIERTO}, /* 305 */
    {75, 77, PT_BEZIERTO}, /* 306 */
    {75, 75, PT_BEZIERTO}, /* 307 */
    {75, 75, PT_BEZIERTO}, /* 308 */
    {75, 75, PT_BEZIERTO}, /* 309 */
    {75, 77, PT_BEZIERTO}, /* 310 */
    {75, 80, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 311 */
    {81, 81, PT_MOVETO}, /* 312 */
    {81, 81, PT_BEZIERTO}, /* 313 */
    {81, 81, PT_BEZIERTO}, /* 314 */
    {81, 81, PT_BEZIERTO}, /* 315 */
    {80, 81, PT_BEZIERTO}, /* 316 */
    {80, 81, PT_BEZIERTO}, /* 317 */
    {80, 81, PT_BEZIERTO}, /* 318 */
    {80, 80, PT_BEZIERTO}, /* 319 */
    {80, 80, PT_BEZIERTO}, /* 320 */
    {81, 80, PT_BEZIERTO}, /* 321 */
    {81, 80, PT_BEZIERTO}, /* 322 */
    {81, 80, PT_BEZIERTO}, /* 323 */
    {81, 81, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 324 */
};

static void test_ellipse(void)
{
    HDC hdc = GetDC( 0 );

    BeginPath( hdc );
    Ellipse( hdc, 20, 20, 40, 40 );
    Ellipse( hdc, 30, 50, 55, 35 );
    Ellipse( hdc, 60, 60, 35, 45 );
    Ellipse( hdc, 70, 70, 50, 70 );
    Ellipse( hdc, 75, 75, 75, 85 );
    Ellipse( hdc, 80, 80, 81, 81 );
    SetArcDirection( hdc, AD_CLOCKWISE );
    Ellipse( hdc, 20, 20, 40, 40 );
    Ellipse( hdc, 30, 50, 55, 35 );
    Ellipse( hdc, 60, 60, 35, 45 );
    Ellipse( hdc, 70, 70, 50, 70 );
    Ellipse( hdc, 75, 75, 75, 85 );
    Ellipse( hdc, 80, 80, 81, 81 );
    SetArcDirection( hdc, AD_COUNTERCLOCKWISE );
    SetMapMode( hdc, MM_ANISOTROPIC );
    SetViewportExtEx( hdc, -2, 2, NULL );
    Ellipse( hdc, 20, 20, 40, 40 );
    Ellipse( hdc, 30, 50, 55, 35 );
    SetViewportExtEx( hdc, 3, -3, NULL );
    Ellipse( hdc, 20, 20, 40, 40 );
    Ellipse( hdc, 30, 50, 55, 35 );
    SetWindowExtEx( hdc, -20, 20, NULL );
    Ellipse( hdc, 20, 20, 40, 40 );
    Ellipse( hdc, 24, 22, 21, 20 );
    SetMapMode( hdc, MM_TEXT );
    SetGraphicsMode( hdc, GM_ADVANCED );
    Ellipse( hdc, 20, 20, 40, 40 );
    Ellipse( hdc, 30, 50, 55, 35 );
    Ellipse( hdc, 60, 60, 35, 45 );
    Ellipse( hdc, 70, 70, 50, 70 );
    Ellipse( hdc, 75, 75, 75, 85 );
    Ellipse( hdc, 80, 80, 81, 81 );
    SetArcDirection( hdc, AD_CLOCKWISE );
    Ellipse( hdc, 20, 20, 40, 40 );
    Ellipse( hdc, 30, 50, 55, 35 );
    Ellipse( hdc, 60, 60, 35, 45 );
    Ellipse( hdc, 70, 70, 50, 70 );
    Ellipse( hdc, 75, 75, 75, 85 );
    Ellipse( hdc, 80, 80, 81, 81 );
    SetArcDirection( hdc, AD_COUNTERCLOCKWISE );
    EndPath( hdc );
    SetMapMode( hdc, MM_TEXT );
    ok_path( hdc, "ellipse_path", ellipse_path, ARRAY_SIZE(ellipse_path) );
}

static const path_test_t all_funcs_path[] =
{
    {0, 0, PT_MOVETO}, /* 0 */
    {50, 150, PT_LINETO}, /* 1 */
    {50, 50, PT_MOVETO}, /* 2 */
    {150, 150, PT_LINETO}, /* 3 */
    {150, 50, PT_LINETO}, /* 4 */
    {50, 50, PT_LINETO}, /* 5 */
    {37, 13, PT_LINETO}, /* 6 */
    {24, 13, PT_BEZIERTO}, /* 7 */
    {14, 23, PT_BEZIERTO}, /* 8 */
    {14, 36, PT_BEZIERTO}, /* 9 */
    {14, 49, PT_BEZIERTO}, /* 10 */
    {24, 59, PT_BEZIERTO}, /* 11 */
    {37, 59, PT_BEZIERTO}, /* 12 */
    {37, 59, PT_BEZIERTO}, /* 13 */
    {37, 59, PT_BEZIERTO}, /* 14 */
    {37, 59, PT_BEZIERTO}, /* 15 */
    {10, 10, PT_MOVETO}, /* 16 */
    {20, 10, PT_LINETO}, /* 17 */
    {10, 20, PT_LINETO}, /* 18 */
    {20, 20, PT_LINETO}, /* 19 */
    {36, 27, PT_MOVETO}, /* 20 */
    {37, 26, PT_BEZIERTO}, /* 21 */
    {38, 25, PT_BEZIERTO}, /* 22 */
    {38, 25, PT_BEZIERTO}, /* 23 */
    {38, 23, PT_BEZIERTO}, /* 24 */
    {34, 21, PT_BEZIERTO}, /* 25 */
    {30, 21, PT_BEZIERTO}, /* 26 */
    {27, 21, PT_BEZIERTO}, /* 27 */
    {25, 21, PT_BEZIERTO}, /* 28 */
    {24, 22, PT_BEZIERTO}, /* 29 */
    {37, 59, PT_MOVETO}, /* 30 */
    {10, 10, PT_LINETO}, /* 31 */
    {20, 10, PT_LINETO}, /* 32 */
    {10, 20, PT_LINETO}, /* 33 */
    {20, 20, PT_LINETO}, /* 34 */
    {34, 26, PT_LINETO}, /* 35 */
    {35, 25, PT_BEZIERTO}, /* 36 */
    {36, 25, PT_BEZIERTO}, /* 37 */
    {36, 25, PT_BEZIERTO}, /* 38 */
    {36, 24, PT_BEZIERTO}, /* 39 */
    {33, 23, PT_BEZIERTO}, /* 40 */
    {30, 23, PT_BEZIERTO}, /* 41 */
    {28, 23, PT_BEZIERTO}, /* 42 */
    {26, 23, PT_BEZIERTO}, /* 43 */
    {25, 23, PT_BEZIERTO}, /* 44 */
    {10, 10, PT_MOVETO}, /* 45 */
    {20, 10, PT_LINETO}, /* 46 */
    {10, 20, PT_LINETO}, /* 47 */
    {20, 20, PT_LINETO}, /* 48 */
    {30, 30, PT_MOVETO}, /* 49 */
    {40, 20, PT_LINETO}, /* 50 */
    {20, 30, PT_LINETO}, /* 51 */
    {30, 40, PT_LINETO}, /* 52 */
    {10, 50, PT_LINETO}, /* 53 */
    {45, 45, PT_MOVETO}, /* 54 */
    {45, 45, PT_BEZIERTO}, /* 55 */
    {44, 46, PT_BEZIERTO}, /* 56 */
    {43, 47, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 57 */
    {10, 10, PT_MOVETO}, /* 58 */
    {20, 10, PT_LINETO}, /* 59 */
    {10, 20, PT_BEZIERTO}, /* 60 */
    {20, 20, PT_BEZIERTO}, /* 61 */
    {30, 30, PT_BEZIERTO}, /* 62 */
    {40, 20, PT_LINETO}, /* 63 */
    {20, 30, PT_LINETO | PT_CLOSEFIGURE}, /* 64 */
    {30, 40, PT_MOVETO}, /* 65 */
    {10, 50, PT_LINETO}, /* 66 */
    {55, 55, PT_MOVETO}, /* 67 */
    {54, 55, PT_BEZIERTO}, /* 68 */
    {54, 56, PT_BEZIERTO}, /* 69 */
    {54, 56, PT_BEZIERTO}, /* 70 */
    {58, 61, PT_LINETO | PT_CLOSEFIGURE}, /* 71 */
    {10, 10, PT_MOVETO}, /* 72 */
    {20, 10, PT_LINETO}, /* 73 */
    {10, 20, PT_LINETO}, /* 74 */
    {20, 20, PT_LINETO}, /* 75 */
    {30, 30, PT_LINETO}, /* 76 */
    {40, 20, PT_LINETO}, /* 77 */
    {20, 30, PT_LINETO}, /* 78 */
    {30, 40, PT_LINETO}, /* 79 */
    {10, 50, PT_LINETO | PT_CLOSEFIGURE}, /* 80 */
    {43, 49, PT_MOVETO}, /* 81 */
    {43, 40, PT_BEZIERTO}, /* 82 */
    {38, 33, PT_BEZIERTO}, /* 83 */
    {33, 33, PT_BEZIERTO}, /* 84 */
    {27, 33, PT_BEZIERTO}, /* 85 */
    {22, 40, PT_BEZIERTO}, /* 86 */
    {22, 49, PT_BEZIERTO}, /* 87 */
    {22, 58, PT_BEZIERTO}, /* 88 */
    {27, 65, PT_BEZIERTO}, /* 89 */
    {33, 65, PT_BEZIERTO}, /* 90 */
    {38, 65, PT_BEZIERTO}, /* 91 */
    {43, 58, PT_BEZIERTO}, /* 92 */
    {43, 49, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 93 */
    {79, 70, PT_MOVETO}, /* 94 */
    {60, 70, PT_LINETO}, /* 95 */
    {60, 89, PT_LINETO}, /* 96 */
    {79, 89, PT_LINETO | PT_CLOSEFIGURE}, /* 97 */
    {199, 122, PT_MOVETO}, /* 98 */
    {199, 110, PT_BEZIERTO}, /* 99 */
    {191, 100, PT_BEZIERTO}, /* 100 */
    {182, 100, PT_BEZIERTO}, /* 101 */
    {117, 100, PT_LINETO}, /* 102 */
    {108, 100, PT_BEZIERTO}, /* 103 */
    {100, 110, PT_BEZIERTO}, /* 104 */
    {100, 122, PT_BEZIERTO}, /* 105 */
    {100, 177, PT_LINETO}, /* 106 */
    {100, 189, PT_BEZIERTO}, /* 107 */
    {108, 199, PT_BEZIERTO}, /* 108 */
    {117, 199, PT_BEZIERTO}, /* 109 */
    {182, 199, PT_LINETO}, /* 110 */
    {191, 199, PT_BEZIERTO}, /* 111 */
    {199, 189, PT_BEZIERTO}, /* 112 */
    {199, 177, PT_BEZIERTO | PT_CLOSEFIGURE}, /* 113 */
    {10, 10, PT_MOVETO}, /* 114 */
    {20, 10, PT_BEZIERTO}, /* 115 */
    {10, 20, PT_BEZIERTO}, /* 116 */
    {20, 20, PT_BEZIERTO}, /* 117 */
    {30, 30, PT_BEZIERTO}, /* 118 */
    {40, 20, PT_BEZIERTO}, /* 119 */
    {20, 30, PT_BEZIERTO}, /* 120 */
    {10, 10, PT_MOVETO}, /* 121 */
    {20, 10, PT_LINETO}, /* 122 */
    {10, 20, PT_LINETO}, /* 123 */
    {20, 20, PT_LINETO | PT_CLOSEFIGURE}, /* 124 */
    {30, 30, PT_MOVETO}, /* 125 */
    {40, 20, PT_LINETO}, /* 126 */
    {20, 30, PT_LINETO}, /* 127 */
    {30, 40, PT_LINETO}, /* 128 */
    {10, 50, PT_LINETO | PT_CLOSEFIGURE}, /* 129 */
    {10, 50, PT_MOVETO}, /* 130 */
    {10, 10, PT_BEZIERTO}, /* 131 */
    {20, 10, PT_BEZIERTO}, /* 132 */
    {10, 20, PT_BEZIERTO}, /* 133 */
    {20, 20, PT_BEZIERTO}, /* 134 */
    {30, 30, PT_BEZIERTO}, /* 135 */
    {40, 20, PT_BEZIERTO}, /* 136 */
    {20, 30, PT_BEZIERTO}, /* 137 */
    {30, 40, PT_BEZIERTO}, /* 138 */
    {10, 50, PT_BEZIERTO}, /* 139 */
    {150, 150, PT_LINETO}, /* 140 */
};

/* run once through all functions that support paths */
static void test_all_functions(void)
{
    POINT pts[9] = {{10, 10}, {20, 10}, {10, 20}, {20, 20}, {30, 30}, {40, 20},
                    {20, 30}, {30, 40}, {10, 50}};
    DWORD counts[5] = {4, 5, 0, 1, 2};
    BYTE types[9] = { PT_MOVETO, PT_LINETO, PT_BEZIERTO, PT_BEZIERTO, PT_BEZIERTO, PT_LINETO,
                      PT_LINETO | PT_CLOSEFIGURE, PT_MOVETO, PT_LINETO };
    HDC hdc = GetDC( 0 );

    BeginPath( hdc );
    LineTo( hdc, 50, 150 );
    MoveToEx( hdc, 50, 50, NULL );
    LineTo( hdc, 150, 150 );
    LineTo( hdc, 150, 50 );
    LineTo( hdc, 50, 50 );
    AngleArc( hdc, 37, 36, 23, 90, 180 );
    Polyline( hdc, pts, 4 );
    Arc( hdc, 21, 21, 39, 29, 39, 29, 21, 21 );
    PolylineTo( hdc, pts, 4 );
    ArcTo( hdc, 23, 23, 37, 27, 37, 27, 23, 23 );
    PolyPolyline( hdc, pts, counts, 2 );
    Chord( hdc, 42, 43, 57, 66, 39, 29, 21, 21 );
    PolyDraw( hdc, pts, types, 9 );
    Pie( hdc, 52, 54, 65, 68, 39, 29, 21, 21 );
    Polygon( hdc, pts, 9 );
    Ellipse( hdc, 22, 33, 44, 66 );
    Rectangle( hdc, 60, 70, 80, 90 );
    RoundRect( hdc, 100, 100, 200, 200, 35, 45 );
    PolyBezier( hdc, pts, 7 );
    PolyPolygon( hdc, pts, (int *)counts, 2 );
    PolyBezierTo( hdc, pts, 9 );
    LineTo( hdc, 150, 150 );
    /* FIXME: ExtTextOut */
    EndPath( hdc );
    ok_path( hdc, "all_funcs_path", all_funcs_path, ARRAY_SIZE(all_funcs_path) );
    ReleaseDC( 0, hdc );
}

static void test_clipped_polygon_fill(void)
{
    const POINT pts[3] = {{-10, -10}, {10, -5}, {0, 10}};
    HBRUSH brush, oldbrush;
    HBITMAP bmp, oldbmp;
    HDC hdc, memdc;
    COLORREF col;

    hdc = GetDC( 0 );
    memdc = CreateCompatibleDC( hdc );
    bmp = CreateCompatibleBitmap( hdc, 20, 20 );
    brush = CreateSolidBrush( RGB( 0x11, 0x22, 0x33 ) );
    oldbrush = SelectObject( memdc, brush );
    oldbmp = SelectObject( memdc, bmp );
    Polygon( memdc, pts, ARRAY_SIZE(pts) );
    col = GetPixel( memdc, 1, 1 );
    ok( col == RGB( 0x11, 0x22, 0x33 ), "got %06lx\n", col );
    SelectObject( memdc, oldbrush );
    SelectObject( memdc, oldbmp );
    DeleteObject( brush );
    DeleteObject( bmp );
    DeleteDC( memdc );
    ReleaseDC( 0, hdc );
}

START_TEST(path)
{
    test_path_state();
    test_widenpath();
    test_arcto();
    test_anglearc();
    test_polydraw();
    test_closefigure();
    test_linedda();
    test_rectangle();
    test_roundrect();
    test_ellipse();
    test_clipped_polygon_fill();
    test_all_functions();
}
