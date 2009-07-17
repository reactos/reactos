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

static void test_widenpath(void)
{
    HDC hdc = GetDC(0);
    HPEN greenPen, narrowPen;
    HPEN oldPen;
    POINT pnt[6];
    INT nSize, ret;

    /* Create a pen to be used in WidenPath */
    greenPen = CreatePen(PS_SOLID, 10, RGB(0,0,0));
    oldPen = SelectObject(hdc, greenPen);

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
       "WidenPath fails while widening an open path. Return value is %d, should be %d. Error is %u\n", ret, FALSE, GetLastError());

    AbortPath(hdc);

    /* Test when the pen width is equal to 1. The path should change too */
    narrowPen = CreatePen(PS_SOLID, 1, RGB(0,0,0));
    oldPen = SelectObject(hdc, narrowPen);
    BeginPath(hdc);
    Polyline(hdc, pnt, 6);
    EndPath(hdc);
    ret = WidenPath(hdc);
    nSize = GetPath(hdc, NULL, NULL, 0);
    ok(nSize > 6, "WidenPath should compute a widdened path with a 1px wide pen. Path length is %d, should be more than 6\n", nSize);

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

    /* How many extra entries before this one only on wine
     * but not on native? */
    int wine_only_entries_preceding;

    /* 0 - This entry matches on wine.
     * 1 - This entry corresponds to a single entry on wine that does not match the native entry.
     * 2 - This entry is currently skipped on wine but present on native. */
    int todo;
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
static void ok_path(HDC hdc, const char *path_name, const path_test_t *expected, int expected_size, BOOL todo_size)
{
    static const char *type_string[8] = { "Unknown (0)", "PT_CLOSEFIGURE", "PT_LINETO",
                                          "PT_LINETO | PT_CLOSEFIGURE", "PT_BEZIERTO",
                                          "PT_BEZIERTO | PT_CLOSEFIGURE", "PT_MOVETO", "PT_MOVETO | PT_CLOSEFIGURE"};
    POINT *pnt = NULL;
    BYTE *types = NULL;
    int size, numskip,
        idx = 0, eidx = 0;

    /* Get the path */
    assert(hdc != 0);
    size = GetPath(hdc, NULL, NULL, 0);
    ok(size > 0, "GetPath returned size %d, last error %d\n", size, GetLastError());
    if (size <= 0)
    {
        skip("Cannot perform path comparisons due to failure to retrieve path.\n");
        return;
    }
    pnt = HeapAlloc(GetProcessHeap(), 0, size*sizeof(POINT));
    assert(pnt != 0);
    types = HeapAlloc(GetProcessHeap(), 0, size*sizeof(BYTE));
    assert(types != 0);
    size = GetPath(hdc, pnt, types, size);
    assert(size > 0);

    if (todo_size) todo_wine
        ok(size == expected_size, "Path size %d does not match expected size %d\n", size, expected_size);
    else
        ok(size == expected_size, "Path size %d does not match expected size %d\n", size, expected_size);

    if (winetest_debug > 2)
        trace("static const path_test_t %s[] = {\n", path_name);

    numskip = expected_size ? expected[eidx].wine_only_entries_preceding : 0;
    while (idx < size && eidx < expected_size)
    {
        /* We allow a few pixels fudge in matching X and Y coordinates to account for imprecision in
         * floating point to integer conversion */
        BOOL match = (types[idx] == expected[eidx].type) &&
            (pnt[idx].x >= expected[eidx].x-2 && pnt[idx].x <= expected[eidx].x+2) &&
            (pnt[idx].y >= expected[eidx].y-2 && pnt[idx].y <= expected[eidx].y+2);

        if (expected[eidx].todo || numskip) todo_wine
            ok(match, "Expected #%d: %s (%d,%d) but got %s (%d,%d)\n", eidx,
               type_string[expected[eidx].type], expected[eidx].x, expected[eidx].y,
               type_string[types[idx]], pnt[idx].x, pnt[idx].y);
        else
            ok(match, "Expected #%d: %s (%d,%d) but got %s (%d,%d)\n", eidx,
               type_string[expected[eidx].type], expected[eidx].x, expected[eidx].y,
               type_string[types[idx]], pnt[idx].x, pnt[idx].y);

        if (match || expected[eidx].todo != 2)
        {
            if (winetest_debug > 2)
                trace("    {%d, %d, %s, 0, 0}%s /* %d */\n", pnt[idx].x, pnt[idx].y,
                      type_string[types[idx]], idx < size-1 ? "," : "};", idx);
            idx++;
        }
        if (match || !numskip--)
            numskip = expected[++eidx].wine_only_entries_preceding;
    }

    /* If we are debugging and the actual path is longer than the expected path, make
     * sure to display the entire path */
    if (winetest_debug > 2 && idx < size)
        for (; idx < size; idx++)
            trace("    {%d, %d, %s, 0, 0}%s /* %d */\n", pnt[idx].x, pnt[idx].y,
                  type_string[types[idx]], idx < size-1 ? "," : "};", idx);

    HeapFree(GetProcessHeap(), 0, types);
    HeapFree(GetProcessHeap(), 0, pnt);
}

static const path_test_t arcto_path[] = {
    {0, 0, PT_MOVETO, 0, 0}, /* 0 */
    {229, 215, PT_LINETO, 0, 0}, /* 1 */
    {248, 205, PT_BEZIERTO, 0, 0}, /* 2 */
    {273, 200, PT_BEZIERTO, 0, 0}, /* 3 */
    {300, 200, PT_BEZIERTO, 0, 0}, /* 4 */
    {355, 200, PT_BEZIERTO, 0, 0}, /* 5 */
    {399, 222, PT_BEZIERTO, 0, 0}, /* 6 */
    {399, 250, PT_BEZIERTO, 0, 0}, /* 7 */
    {399, 263, PT_BEZIERTO, 0, 0}, /* 8 */
    {389, 275, PT_BEZIERTO, 0, 0}, /* 9 */
    {370, 285, PT_BEZIERTO, 0, 0}, /* 10 */
    {363, 277, PT_LINETO, 0, 0}, /* 11 */
    {380, 270, PT_BEZIERTO, 0, 0}, /* 12 */
    {389, 260, PT_BEZIERTO, 0, 0}, /* 13 */
    {389, 250, PT_BEZIERTO, 0, 0}, /* 14 */
    {389, 228, PT_BEZIERTO, 0, 0}, /* 15 */
    {349, 210, PT_BEZIERTO, 0, 0}, /* 16 */
    {300, 210, PT_BEZIERTO, 0, 0}, /* 17 */
    {276, 210, PT_BEZIERTO, 0, 0}, /* 18 */
    {253, 214, PT_BEZIERTO, 0, 0}, /* 19 */
    {236, 222, PT_BEZIERTO | PT_CLOSEFIGURE, 0, 0}}; /* 20 */

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

    ok_path(hdc, "arcto_path", arcto_path, sizeof(arcto_path)/sizeof(path_test_t), 0);
done:
    ReleaseDC(0, hdc);
}

static const path_test_t anglearc_path[] = {
    {0, 0, PT_MOVETO, 0, 0}, /* 0 */
    {371, 229, PT_LINETO, 0, 0}, /* 1 */
    {352, 211, PT_BEZIERTO, 0, 0}, /* 2 */
    {327, 200, PT_BEZIERTO, 0, 0}, /* 3 */
    {300, 200, PT_BEZIERTO, 0, 0}, /* 4 */
    {245, 200, PT_BEZIERTO, 0, 0}, /* 5 */
    {200, 245, PT_BEZIERTO, 0, 0}, /* 6 */
    {200, 300, PT_BEZIERTO, 0, 0}, /* 7 */
    {200, 300, PT_BEZIERTO, 0, 2}, /* 8 */
    {200, 300, PT_BEZIERTO, 0, 2}, /* 9 */
    {200, 300, PT_BEZIERTO, 0, 2}, /* 10 */
    {231, 260, PT_LINETO, 0, 0}, /* 11 */
    {245, 235, PT_BEZIERTO, 0, 0}, /* 12 */
    {271, 220, PT_BEZIERTO, 0, 0}, /* 13 */
    {300, 220, PT_BEZIERTO, 0, 0}, /* 14 */
    {344, 220, PT_BEZIERTO, 0, 0}, /* 15 */
    {380, 256, PT_BEZIERTO, 0, 0}, /* 16 */
    {380, 300, PT_BEZIERTO, 0, 0}, /* 17 */
    {380, 314, PT_BEZIERTO, 0, 0}, /* 18 */
    {376, 328, PT_BEZIERTO, 0, 0}, /* 19 */
    {369, 340, PT_BEZIERTO | PT_CLOSEFIGURE, 0, 0}}; /* 20 */

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

    ok_path(hdc, "anglearc_path", anglearc_path, sizeof(anglearc_path)/sizeof(path_test_t), 1);
done:
    ReleaseDC(0, hdc);
}

static const path_test_t polydraw_path[] = {
    {0, 0, PT_MOVETO, 0, 0}, /*0*/
    {10, 10, PT_LINETO, 0, 0}, /*1*/
    {10, 15, PT_LINETO | PT_CLOSEFIGURE, 0, 0}, /*2*/
    {100, 100, PT_MOVETO, 0, 0}, /*3*/
    {95, 95, PT_LINETO, 0, 0}, /*4*/
    {10, 10, PT_LINETO, 0, 0}, /*5*/
    {10, 15, PT_LINETO | PT_CLOSEFIGURE, 0, 0}, /*6*/
    {100, 100, PT_MOVETO, 0, 0}, /*7*/
    {15, 15, PT_LINETO, 0, 0}, /*8*/
    {25, 25, PT_MOVETO, 0, 0}, /*9*/
    {25, 30, PT_LINETO, 0, 0}, /*10*/
    {100, 100, PT_MOVETO, 0, 0}, /*11*/
    {30, 30, PT_BEZIERTO, 0, 0}, /*12*/
    {30, 35, PT_BEZIERTO, 0, 0}, /*13*/
    {35, 35, PT_BEZIERTO, 0, 0}, /*14*/
    {35, 40, PT_LINETO, 0, 0}, /*15*/
    {40, 40, PT_MOVETO, 0, 0}, /*16*/
    {40, 45, PT_LINETO, 0, 0}, /*17*/
    {35, 40, PT_MOVETO, 0, 0}, /*18*/
    {45, 50, PT_LINETO, 0, 0}, /*19*/
    {35, 40, PT_MOVETO, 0, 0}, /*20*/
    {50, 55, PT_LINETO, 0, 0}, /*21*/
    {45, 50, PT_LINETO, 0, 0}, /*22*/
    {35, 40, PT_MOVETO, 0, 0}, /*23*/
    {60, 60, PT_LINETO, 0, 0}, /*24*/
    {60, 65, PT_MOVETO, 0, 0}, /*25*/
    {65, 65, PT_LINETO, 0, 0} /*26*/
    };

static POINT polydraw_pts[] = {
    {10, 10}, {10, 15},
    {15, 15}, {15, 20}, {20, 20}, {20, 25},
    {25, 25}, {25, 30},
    {30, 30}, {30, 35}, {35, 35}, {35, 40},
    {40, 40}, {40, 45}, {45, 45},
    {45, 50}, {50, 50},
    {50, 55}, {45, 50}, {55, 60},
    {60, 60}, {60, 65}, {65, 65}};

static BYTE polydraw_tps[] =
    {PT_LINETO, PT_CLOSEFIGURE | PT_LINETO, /* 2 */
     PT_LINETO, PT_BEZIERTO, PT_LINETO, PT_LINETO, /* 6 */
     PT_MOVETO, PT_LINETO, /* 8 */
     PT_BEZIERTO, PT_BEZIERTO, PT_BEZIERTO, PT_LINETO, /* 12 */
     PT_MOVETO, PT_LINETO, PT_CLOSEFIGURE, /* 15 */
     PT_LINETO, PT_MOVETO | PT_CLOSEFIGURE, /* 17 */
     PT_LINETO, PT_LINETO, PT_MOVETO | PT_CLOSEFIGURE, /* 20 */
     PT_LINETO, PT_MOVETO | PT_LINETO, PT_LINETO}; /* 23 */

static void test_polydraw(void)
{
    BOOL retb;
    HDC hdc = GetDC(0);
    BeginPath(hdc);

    /* closefigure with no previous moveto */
    if (!(retb = PolyDraw(hdc, polydraw_pts, polydraw_tps, 2)) &&
        GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        /* PolyDraw is only available on Win2k and later */
        win_skip("PolyDraw is not available\n");
        goto done;
    }
    expect(TRUE, retb);

    MoveToEx(hdc, 100, 100, NULL);
    LineTo(hdc, 95, 95);
    /* closefigure with previous moveto */
    retb = PolyDraw(hdc, polydraw_pts, polydraw_tps, 2);
    expect(TRUE, retb);
    /* bad bezier points */
    retb = PolyDraw(hdc, &(polydraw_pts[2]), &(polydraw_tps[2]), 4);
    expect(FALSE, retb);
    retb = PolyDraw(hdc, &(polydraw_pts[6]), &(polydraw_tps[6]), 4);
    expect(FALSE, retb);
    /* good bezier points */
    retb = PolyDraw(hdc, &(polydraw_pts[8]), &(polydraw_tps[8]), 4);
    expect(TRUE, retb);
    /* does lineto or bezierto take precedence? */
    retb = PolyDraw(hdc, &(polydraw_pts[12]), &(polydraw_tps[12]), 4);
    expect(FALSE, retb);
    /* bad point type, has already moved cursor position */
    retb = PolyDraw(hdc, &(polydraw_pts[15]), &(polydraw_tps[15]), 4);
    expect(FALSE, retb);
    /* bad point type, cursor position is moved, but back to its original spot */
    retb = PolyDraw(hdc, &(polydraw_pts[17]), &(polydraw_tps[17]), 4);
    expect(FALSE, retb);
    /* does lineto or moveto take precedence? */
    retb = PolyDraw(hdc, &(polydraw_pts[20]), &(polydraw_tps[20]), 3);
    expect(TRUE, retb);

    EndPath(hdc);
    ok_path(hdc, "polydraw_path", polydraw_path, sizeof(polydraw_path)/sizeof(path_test_t), 0);
done:
    ReleaseDC(0, hdc);
}

static void test_closefigure(void) {
    BOOL retb;
    int nSize, nSizeWitness;
    HDC hdc = GetDC(0);

    BeginPath(hdc);
    MoveToEx(hdc, 95, 95, NULL);
    LineTo(hdc, 95,  0);
    LineTo(hdc,  0, 95);

    retb = CloseFigure(hdc);
    EndPath(hdc);
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
    ok((*pt)->x == x && (*pt)->y == y, "point mismatch expect(%d,%d) got(%d,%d)\n",
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

START_TEST(path)
{
    test_widenpath();
    test_arcto();
    test_anglearc();
    test_polydraw();
    test_closefigure();
    test_linedda();
}
