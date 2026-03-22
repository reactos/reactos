/*
 * Unit test suite for paths
 *
 * Copyright (C) 2007 Google (Evan Stade)
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

#include "objbase.h"
#include "gdiplus.h"
#include "wine/test.h"
#include <math.h>

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)
#define expectf(expected, got) ok(fabs(expected - got) < 2.0, "Expected %.2f, got %.2f\n", expected, got)
#define POINT_TYPE_MAX_LEN (75)

static void stringify_point_type(PathPointType type, char * name)
{
    *name = '\0';

    switch(type & PathPointTypePathTypeMask){
        case PathPointTypeStart:
            strcat(name, "PathPointTypeStart");
            break;
        case PathPointTypeLine:
            strcat(name, "PathPointTypeLine");
            break;
        case PathPointTypeBezier:
            strcat(name, "PathPointTypeBezier");
            break;
        default:
            strcat(name, "Unknown type");
            return;
    }

    type &= ~PathPointTypePathTypeMask;
    if(type & ~((PathPointTypePathMarker | PathPointTypeCloseSubpath))){
        *name = '\0';
        strcat(name, "Unknown type");
        return;
    }

    if(type & PathPointTypePathMarker)
        strcat(name, " | PathPointTypePathMarker");
    if(type & PathPointTypeCloseSubpath)
        strcat(name, " | PathPointTypeCloseSubpath");
}

/* this helper structure and function modeled after gdi path.c test */
typedef struct
{
    REAL X, Y;
    BYTE type;

    /* How many extra entries before this one only on wine
     * but not on native? */
    int wine_only_entries_preceding;

    /* 0 - This entry matches on wine.
     * 1 - This entry corresponds to a single entry on wine that does not match the native entry.
     * 2 - This entry is currently skipped on wine but present on native. */
    int todo;
} path_test_t;

#define ok_path(a,b,c,d) _ok_path_fudge(a,b,c,d,1.0,__LINE__)
#define ok_path_fudge(a,b,c,d,e) _ok_path_fudge(a,b,c,d,e,__LINE__)
static void _ok_path_fudge(GpPath* path, const path_test_t *expected, INT expected_size,
        BOOL todo_size, REAL fudge, int line)
{
    BYTE * types;
    INT size, idx = 0, eidx = 0, numskip;
    GpPointF * points;
    char ename[POINT_TYPE_MAX_LEN], name[POINT_TYPE_MAX_LEN];

    if(GdipGetPointCount(path, &size) != Ok){
        skip("Cannot perform path comparisons due to failure to retrieve path.\n");
        return;
    }

    todo_wine_if (todo_size)
        ok_(__FILE__,line)(size == expected_size, "Path size %d does not match expected size %d\n",
            size, expected_size);

    points = malloc(size * sizeof(GpPointF));
    types = malloc(size);

    if(GdipGetPathPoints(path, points, size) != Ok || GdipGetPathTypes(path, types, size) != Ok){
        skip("Cannot perform path comparisons due to failure to retrieve path.\n");
        goto end;
    }

    numskip = expected_size ? expected[eidx].wine_only_entries_preceding : 0;
    while (idx < size && eidx < expected_size){
        /* We allow a few pixels fudge in matching X and Y coordinates to account for imprecision in
         * floating point to integer conversion */
        BOOL match = (types[idx] == expected[eidx].type) &&
            fabs(points[idx].X - expected[eidx].X) <= fudge &&
            fabs(points[idx].Y - expected[eidx].Y) <= fudge;

        stringify_point_type(expected[eidx].type, ename);
        stringify_point_type(types[idx], name);

        todo_wine_if (expected[eidx].todo || numskip)
            ok_(__FILE__,line)(match, "Expected #%d: %s (%.6f,%.6f) but got %s (%.6f,%.6f)\n", eidx,
               ename, expected[eidx].X, expected[eidx].Y,
               name, points[idx].X, points[idx].Y);

        if (match || expected[eidx].todo != 2)
            idx++;
        if (match || !numskip--)
            numskip = expected[++eidx].wine_only_entries_preceding;
    }

end:
    free(types);
    free(points);
}

static void test_constructor_destructor(void)
{
    GpStatus status;
    GpPath* path = NULL;

    status = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, status);
    ok(path != NULL, "Expected path to be initialized\n");

    status = GdipDeletePath(NULL);
    expect(InvalidParameter, status);

    status = GdipDeletePath(path);
    expect(Ok, status);
}

static void test_getpathdata(void)
{
    GpPath *path;
    GpPathData data;
    GpStatus status;
    INT count;

    status = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, status);
    status = GdipAddPathLine(path, 5.0, 5.0, 100.0, 50.0);
    expect(Ok, status);

    status = GdipGetPointCount(path, &count);
    expect(Ok, status);
    expect(2, count);

    data.Count  = count;
    data.Types  = GdipAlloc(sizeof(BYTE) * count);
    data.Points = GdipAlloc(sizeof(PointF) * count);

    status = GdipGetPathData(path, &data);
    expect(Ok, status);
    expect((data.Points[0].X == 5.0) && (data.Points[0].Y == 5.0) &&
           (data.Points[1].X == 100.0) && (data.Points[1].Y == 50.0), TRUE);
    expect((data.Types[0] == PathPointTypeStart) && (data.Types[1] == PathPointTypeLine), TRUE);

    GdipFree(data.Points);
    GdipFree(data.Types);
    GdipDeletePath(path);
}

static void test_createpath2(void)
{
    GpStatus status;
    GpPath* path = NULL;
    GpPathData data;
    INT i, count, expect_count;

    PointF test_line_points[] = {{1.0,1.0}, {2.0,1.0}, {2.0,2.0}};
    BYTE test_line_types[] = {PathPointTypeStart, PathPointTypeLine, PathPointTypeStart};

    PointF test_bez_points[] = {{1.0,1.0}, {2.0,1.0}, {3.0,1.0}, {4.0,1.0},
            {5.0,1.0}, {6.0,1.0}, {7.0,1.0}};
    BYTE test_bez_types[] = {PathPointTypeStart, PathPointTypeBezier,
            PathPointTypeBezier, PathPointTypeBezier, PathPointTypeBezier,
            PathPointTypeBezier, PathPointTypeBezier};

    status = GdipCreatePath2(test_line_points, test_line_types, 2, FillModeAlternate, &path);
    expect(Ok, status);
    status = GdipGetPointCount(path, &count);
    expect(Ok, status);
    expect(2, count);
    GdipDeletePath(path);

    status = GdipCreatePath2(test_line_points, test_line_types, 1, FillModeAlternate, &path);
    expect(Ok, status);
    status = GdipGetPointCount(path, &count);
    expect(Ok, status);
    expect(1, count);
    GdipDeletePath(path);

    path = (void *)0xdeadbeef;
    status = GdipCreatePath2(test_line_points, test_line_types, 0, FillModeAlternate, &path);
    expect(OutOfMemory, status);
    ok(!path, "Expected NULL, got %p\n", path);
    if(path && path != (void *)0xdeadbeef)
        GdipDeletePath(path);

    path = (void *)0xdeadbeef;
    status = GdipCreatePath2(test_line_points, test_line_types, -1, FillModeAlternate, &path);
    expect(OutOfMemory, status);
    ok(!path, "Expected NULL, got %p\n", path);
    if(path && path != (void *)0xdeadbeef)
        GdipDeletePath(path);

    path = (void *)0xdeadbeef;
    status = GdipCreatePath2(NULL, test_line_types, 2, FillModeAlternate, &path);
    expect(InvalidParameter, status);
    ok(path == (void *)0xdeadbeef, "Expected %p, got %p\n", (void *)0xdeadbeef, path);
    if(path && path != (void *)0xdeadbeef)
        GdipDeletePath(path);

    path = (void *)0xdeadbeef;
    status = GdipCreatePath2(test_line_points, NULL, 2, FillModeAlternate, &path);
    expect(InvalidParameter, status);
    ok(path == (void *)0xdeadbeef, "Expected %p, got %p\n", (void *)0xdeadbeef, path);
    if(path && path != (void *)0xdeadbeef)
        GdipDeletePath(path);

    status = GdipCreatePath2(test_line_points, test_line_types, 2, FillModeAlternate, NULL);
    expect(InvalidParameter, status);

    /* Multi-point paths should not end with Start */
    status = GdipCreatePath2(test_line_points, test_line_types, 3, FillModeAlternate, &path);
    expect(Ok, status);
    status = GdipGetPointCount(path, &count);
    expect(Ok, status);
    expect(0, count);
    GdipDeletePath(path);

    /* Zero-length line points do not get altered */
    test_line_points[1].X = test_line_points[0].X;
    test_line_points[1].Y = test_line_points[0].Y;
    status = GdipCreatePath2(test_line_points, test_line_types, 2, FillModeAlternate, &path);
    expect(Ok, status);
    status = GdipGetPointCount(path, &count);
    expect(Ok, status);
    expect(2, count);
    GdipDeletePath(path);

    /* The type of the first point is always converted to PathPointTypeStart */
    test_line_types[0] = PathPointTypeLine;
    status = GdipCreatePath2(test_line_points, test_line_types, 1, FillModeAlternate, &path);
    expect(Ok, status);
    status = GdipGetPointCount(path, &count);
    expect(Ok, status);
    expect(1, count);
    data.Count  = count;
    data.Types  = GdipAlloc(sizeof(BYTE) * count);
    data.Points = GdipAlloc(sizeof(PointF) * count);
    status = GdipGetPathData(path, &data);
    expect(Ok, status);
    expect((data.Points[0].X == 1.0) && (data.Points[0].Y == 1.0), TRUE);
    expect(data.Types[0], PathPointTypeStart);
    GdipFree(data.Points);
    GdipFree(data.Types);
    GdipDeletePath(path);

    /* Bezier points must come in groups of three */
    for(i = 2; i <= 7; i++) {
        expect_count = (i % 3 == 1) ? i : 0;
        status = GdipCreatePath2(test_bez_points, test_bez_types, i, FillModeAlternate, &path);
        expect(Ok, status);
        status = GdipGetPointCount(path, &count);
        expect(Ok, status);
        expect(expect_count, count);
        GdipDeletePath(path);
    }
}

static path_test_t line2_path[] = {
    {0.0, 50.0, PathPointTypeStart, 0, 0}, /*0*/
    {5.0, 45.0, PathPointTypeLine, 0, 0}, /*1*/
    {0.0, 40.0, PathPointTypeLine, 0, 0}, /*2*/
    {15.0, 35.0, PathPointTypeLine, 0, 0}, /*3*/
    {0.0, 30.0, PathPointTypeLine, 0, 0}, /*4*/
    {25.0, 25.0, PathPointTypeLine | PathPointTypeCloseSubpath, 0, 0}, /*5*/
    {0.0, 20.0, PathPointTypeStart, 0, 0}, /*6*/
    {35.0, 15.0, PathPointTypeLine, 0, 0}, /*7*/
    {0.0, 10.0, PathPointTypeLine, 0, 0} /*8*/
    };

static void test_line2(void)
{
    GpStatus status;
    GpPath* path;
    int i;
    GpPointF line2_points[9];

    for(i = 0; i < 9; i ++){
        line2_points[i].X = i * 5.0 * (REAL)(i % 2);
        line2_points[i].Y = 50.0 - i * 5.0;
    }

    GdipCreatePath(FillModeAlternate, &path);

    status = GdipAddPathLine2(NULL, line2_points, 2);
    expect(InvalidParameter, status);
    status = GdipAddPathLine2(path, NULL, 2);
    expect(InvalidParameter, status);
    status = GdipAddPathLine2(path, line2_points, 0);
    expect(InvalidParameter, status);
    status = GdipAddPathLine2(path, line2_points, -1);
    expect(InvalidParameter, status);

    status = GdipAddPathLine2(path, line2_points, 3);
    expect(Ok, status);
    status = GdipAddPathLine2(path, &(line2_points[3]), 3);
    expect(Ok, status);
    status = GdipClosePathFigure(path);
    expect(Ok, status);
    status = GdipAddPathLine2(path, &(line2_points[6]), 3);
    expect(Ok, status);

    ok_path(path, line2_path, ARRAY_SIZE(line2_path), FALSE);

    GdipResetPath(path);
    status = GdipAddPathLine2(path, line2_points, 3);
    expect(Ok, status);
    status = GdipAddPathLine2(path, &(line2_points[2]), 3);
    expect(Ok, status);

    ok_path(path, line2_path, 5, FALSE);

    GdipDeletePath(path);
}

static path_test_t bezier_path[] = {
    {10.0, 10.0, PathPointTypeStart, 0, 0}, /*0*/
    {20.0, 10.0, PathPointTypeBezier, 0, 0}, /*1*/
    {20.0, 20.0, PathPointTypeBezier, 0, 0}, /*2*/
    {30.0, 20.0, PathPointTypeBezier, 0, 0}, /*3*/
    {40.0, 20.0, PathPointTypeBezier, 0, 0}, /*4*/
    {40.0, 30.0, PathPointTypeBezier, 0, 0}, /*5*/
    {50.0, 30.0, PathPointTypeBezier, 0, 0}, /*6*/
    {50.0, 10.0, PathPointTypeLine, 0, 0}, /*7*/
    {60.0, 10.0, PathPointTypeBezier, 0, 0}, /*8*/
    {60.0, 20.0, PathPointTypeBezier, 0, 0}, /*9*/
    {70.0, 20.0, PathPointTypeBezier, 0, 0} /*10*/
    };

static void test_bezier(void)
{
    GpStatus status;
    GpPath* path;

    GdipCreatePath(FillModeAlternate, &path);

    status = GdipAddPathBezier(path, 10.0, 10.0, 20.0, 10.0, 20.0, 20.0, 30.0, 20.0);
    expect(Ok, status);
    status = GdipAddPathBezier(path, 30.0, 20.0, 40.0, 20.0, 40.0, 30.0, 50.0, 30.0);
    expect(Ok, status);
    status = GdipAddPathBezier(path, 50.0, 10.0, 60.0, 10.0, 60.0, 20.0, 70.0, 20.0);
    expect(Ok, status);

    ok_path(path, bezier_path, ARRAY_SIZE(bezier_path), FALSE);

    GdipDeletePath(path);
}

static void test_beziers(void)
{
    GpStatus status;
    GpPath* path;
    PointF bezier_points1[] = {{10.0,10.0}, {20.0,10.0}, {20.0,20.0}, {30.0,20.0}};
    PointF bezier_points2[] = {{30.0,20.0}, {40.0,20.0}, {40.0,30.0}, {50.0,30.0}};
    PointF bezier_points3[] = {{50.0,10.0}, {60.0,10.0}, {60.0,20.0}, {70.0,20.0}};

    GdipCreatePath(FillModeAlternate, &path);

    status = GdipAddPathBeziers(path, bezier_points1, 4);
    expect(Ok, status);
    status = GdipAddPathBeziers(path, bezier_points2, 4);
    expect(Ok, status);
    status = GdipAddPathBeziers(path, bezier_points3, 4);
    expect(Ok, status);

    ok_path(path, bezier_path, ARRAY_SIZE(bezier_path), FALSE);

    GdipDeletePath(path);
}

static path_test_t arc_path[] = {
    {600.0, 450.0, PathPointTypeStart, 0, 0}, /*0*/
    {600.0, 643.3, PathPointTypeBezier, 0, 0}, /*1*/
    {488.1, 800.0, PathPointTypeBezier, 0, 0}, /*2*/
    {350.0, 800.0, PathPointTypeBezier, 0, 0}, /*3*/
    {600.0, 450.0, PathPointTypeLine, 0, 0}, /*4*/
    {600.0, 643.3, PathPointTypeBezier, 0, 0}, /*5*/
    {488.1, 800.0, PathPointTypeBezier, 0, 0}, /*6*/
    {350.0, 800.0, PathPointTypeBezier, 0, 0}, /*7*/
    {329.8, 800.0, PathPointTypeBezier, 0, 0}, /*8*/
    {309.7, 796.6, PathPointTypeBezier, 0, 0}, /*9*/
    {290.1, 789.8, PathPointTypeBezier, 0, 0}, /*10*/
    {409.9, 110.2, PathPointTypeLine, 0, 0}, /*11*/
    {544.0, 156.5, PathPointTypeBezier, 0, 0}, /*12*/
    {625.8, 346.2, PathPointTypeBezier, 0, 0}, /*13*/
    {592.7, 533.9, PathPointTypeBezier, 0, 0}, /*14*/
    {592.5, 535.3, PathPointTypeBezier, 0, 0}, /*15*/
    {592.2, 536.7, PathPointTypeBezier, 0, 0}, /*16*/
    {592.0, 538.1, PathPointTypeBezier, 0, 0}, /*17*/
    {409.9, 789.8, PathPointTypeLine, 0, 0}, /*18*/
    {544.0, 743.5, PathPointTypeBezier, 0, 0}, /*19*/
    {625.8, 553.8, PathPointTypeBezier, 0, 0}, /*20*/
    {592.7, 366.1, PathPointTypeBezier, 0, 0}, /*21*/
    {592.5, 364.7, PathPointTypeBezier, 0, 0}, /*22*/
    {592.2, 363.3, PathPointTypeBezier, 0, 0}, /*23*/
    {592.0, 361.9, PathPointTypeBezier, 0, 0}, /*24*/
    {540.4, 676.9, PathPointTypeLine, 0, 0}, /*25*/
    {629.9, 529.7, PathPointTypeBezier, 0, 0}, /*26*/
    {617.2, 308.8, PathPointTypeBezier, 0, 0}, /*27*/
    {512.1, 183.5, PathPointTypeBezier, 0, 0}, /*28*/
    {406.9, 58.2, PathPointTypeBezier, 0, 0}, /*29*/
    {249.1, 75.9, PathPointTypeBezier, 0, 0}, /*30*/
    {159.6, 223.1, PathPointTypeBezier, 0, 0}, /*31*/
    {70.1, 370.3, PathPointTypeBezier, 0, 0}, /*32*/
    {82.8, 591.2, PathPointTypeBezier, 0, 0}, /*33*/
    {187.9, 716.5, PathPointTypeBezier, 0, 0}, /*34*/
    {293.1, 841.8, PathPointTypeBezier, 0, 0}, /*35*/
    {450.9, 824.1, PathPointTypeBezier, 0, 0}, /*36*/
    {540.4, 676.9, PathPointTypeBezier | PathPointTypeCloseSubpath, 0, 1} /*37*/
    };
static path_test_t arc_path2[] = {
    {1.0, 0.0, PathPointTypeStart, 0, 0}, /*0*/
    {1.0, 0.5, PathPointTypeLine, 0, 0}, /*1*/
    {1.0, 0.776142, PathPointTypeBezier, 0, 0}, /*2*/
    {0.776142, 1.0, PathPointTypeBezier, 0, 0}, /*3*/
    {0.5, 1.0, PathPointTypeBezier, 0, 0} /*4*/
    };

static void test_arc(void)
{
    GpStatus status;
    GpPath* path;

    GdipCreatePath(FillModeAlternate, &path);

    status = GdipAddPathArc(path, 100.0, 100.0, 1.0, 0.0, 0.0, 90.0);
    expect(InvalidParameter, status);

    status = GdipAddPathArc(path, 100.0, 100.0, 0.0, 1.0, 0.0, 90.0);
    expect(InvalidParameter, status);

    status = GdipAddPathArc(path, 100.0, 100.0, -40, 1.0, 0.0, 90.0);
    expect(InvalidParameter, status);

    status = GdipAddPathArc(path, 100.0, 100.0, 1.0, -50.0, 0.0, 90.0);
    expect(InvalidParameter, status);

    GdipResetPath(path);
    /* Exactly 90 degrees */
    status = GdipAddPathArc(path, 100.0, 100.0, 500.0, 700.0, 0.0, 90.0);
    expect(Ok, status);
    /* Over 90 degrees */
    status = GdipAddPathArc(path, 100.0, 100.0, 500.0, 700.0, 0.0, 100.0);
    expect(Ok, status);
    /* Negative start angle */
    status = GdipAddPathArc(path, 100.0, 100.0, 500.0, 700.0, -80.0, 100.0);
    expect(Ok, status);
    /* Negative sweep angle */
    status = GdipAddPathArc(path, 100.0, 100.0, 500.0, 700.0, 80.0, -100.0);
    expect(Ok, status);
    /* More than a full revolution */
    status = GdipAddPathArc(path, 100.0, 100.0, 500.0, 700.0, 50.0, -400.0);
    expect(Ok, status);
    /* 0 sweep angle */
    status = GdipAddPathArc(path, 100.0, 100.0, 500.0, 700.0, 50.0, 0.0);
    expect(Ok, status);

    ok_path(path, arc_path, ARRAY_SIZE(arc_path), FALSE);

    GdipResetPath(path);
    GdipAddPathLine(path, 1.0, 0.0, 1.0, 0.5);
    status = GdipAddPathArc(path, 0.0, 0.0, 1.0, 1.0, 0.0, 90.0);
    expect(Ok, status);

    ok_path_fudge(path, arc_path2, ARRAY_SIZE(arc_path2), FALSE, 0.000005);

    GdipDeletePath(path);
}

static void test_worldbounds(void)
{
    GpStatus status;
    GpPath *path;
    GpPen *pen;
    GpMatrix *matrix;
    GpRectF bounds;
    GpPointF line2_points[10];
    int i;

    for(i = 0; i < 10; i ++){
        line2_points[i].X = 200.0 + i * 50.0 * (i % 2);
        line2_points[i].Y = 200.0 + i * 50.0 * !(i % 2);
    }
    GdipCreatePen1((ARGB)0xdeadbeef, 20.0, UnitWorld, &pen);
    GdipSetPenEndCap(pen, LineCapSquareAnchor);
    GdipCreateMatrix2(1.5, 0.0, 1.0, 1.2, 10.4, 10.2, &matrix);

    GdipCreatePath(FillModeAlternate, &path);
    GdipAddPathArc(path, 100.0, 100.0, 500.0, 700.0, 0.0, 100.0);
    GdipAddPathLine2(path, &(line2_points[0]), 10);
    status = GdipGetPathWorldBounds(path, &bounds, NULL, NULL);
    expect(Ok, status);
    GdipDeletePath(path);

    expectf(200.0, bounds.X);
    expectf(200.0, bounds.Y);
    expectf(450.0, bounds.Width);
    expectf(600.0, bounds.Height);

    GdipCreatePath(FillModeAlternate, &path);
    GdipAddPathArc(path, 100.0, 100.0, 500.0, 700.0, 0.0, 100.0);
    GdipAddPathLine2(path, &(line2_points[0]), 10);
    status = GdipGetPathWorldBounds(path, &bounds, matrix, NULL);
    expect(Ok, status);
    GdipDeletePath(path);

    expectf(510.4, bounds.X);
    expectf(250.2, bounds.Y);
    expectf(1275.0, bounds.Width);
    expectf(720.0, bounds.Height);

    GdipCreatePath(FillModeAlternate, &path);
    GdipAddPathArc(path, 100.0, 100.0, 500.0, 700.0, 0.0, 100.0);
    GdipAddPathLine2(path, &(line2_points[0]), 10);
    status = GdipGetPathWorldBounds(path, &bounds, NULL, pen);
    expect(Ok, status);
    GdipDeletePath(path);

    expectf(100.0, bounds.X);
    expectf(100.0, bounds.Y);
    expectf(650.0, bounds.Width);
    expectf(800.0, bounds.Height);

    GdipCreatePath(FillModeAlternate, &path);
    GdipAddPathLine2(path, &(line2_points[0]), 2);
    status = GdipGetPathWorldBounds(path, &bounds, NULL, pen);
    expect(Ok, status);
    GdipDeletePath(path);

    expectf(156.0, bounds.X);
    expectf(156.0, bounds.Y);
    expectf(138.0, bounds.Width);
    expectf(88.0, bounds.Height);

    line2_points[2].X = 2 * line2_points[1].X - line2_points[0].X;
    line2_points[2].Y = 2 * line2_points[1].Y - line2_points[0].Y;

    GdipCreatePath(FillModeAlternate, &path);
    GdipAddPathLine2(path, &(line2_points[0]), 3);
    status = GdipGetPathWorldBounds(path, &bounds, NULL, pen);
    expect(Ok, status);
    GdipDeletePath(path);

    expectf(100.0, bounds.X);
    expectf(100.0, bounds.Y);
    expectf(300.0, bounds.Width);
    expectf(200.0, bounds.Height);

    GdipCreatePath(FillModeAlternate, &path);
    GdipAddPathArc(path, 100.0, 100.0, 500.0, 700.0, 45.0, 20.0);
    status = GdipGetPathWorldBounds(path, &bounds, NULL, pen);
    expect(Ok, status);
    GdipDeletePath(path);

    expectf(386.7, bounds.X);
    expectf(553.4, bounds.Y);
    expectf(266.8, bounds.Width);
    expectf(289.6, bounds.Height);

    GdipCreatePath(FillModeAlternate, &path);
    status = GdipGetPathWorldBounds(path, &bounds, matrix, pen);
    expect(Ok, status);
    GdipDeletePath(path);

    expectf(0.0, bounds.X);
    expectf(0.0, bounds.Y);
    expectf(0.0, bounds.Width);
    expectf(0.0, bounds.Height);

    GdipCreatePath(FillModeAlternate, &path);
    GdipAddPathLine2(path, &(line2_points[0]), 2);
    status = GdipGetPathWorldBounds(path, &bounds, matrix, pen);
    expect(Ok, status);
    GdipDeletePath(path);

    todo_wine{
        expectf(427.9, bounds.X);
        expectf(167.7, bounds.Y);
        expectf(239.9, bounds.Width);
        expectf(164.9, bounds.Height);
    }

    GdipDeleteMatrix(matrix);
    GdipCreateMatrix2(0.9, -0.5, -0.5, -1.2, 10.4, 10.2, &matrix);
    GdipCreatePath(FillModeAlternate, &path);
    GdipAddPathArc(path, 100.0, 100.0, 500.0, 700.0, 0.0, 100.0);
    GdipAddPathLine2(path, &(line2_points[0]), 10);
    status = GdipGetPathWorldBounds(path, &bounds, matrix, NULL);
    expect(Ok, status);
    GdipDeletePath(path);
    GdipDeleteMatrix(matrix);

    expectf(-209.6, bounds.X);
    expectf(-1274.8, bounds.Y);
    expectf(705.0, bounds.Width);
    expectf(945.0, bounds.Height);

    GdipDeletePen(pen);
}

static path_test_t pathpath_path[] = {
    {600.00, 450.00, PathPointTypeStart, 0, 0}, /*0*/
    {600.00, 643.30, PathPointTypeBezier, 0, 0}, /*1*/
    {488.07, 800.00, PathPointTypeBezier, 0, 0}, /*2*/
    {350.00, 800.00, PathPointTypeBezier, 0, 0}, /*3*/
    {319.61, 797.40, PathPointTypeStart, 0, 0}, /*4*/
    {182.56, 773.90, PathPointTypeBezier, 0, 0}, /*5*/
    {85.07, 599.31, PathPointTypeBezier, 0, 0}, /*6*/
    {101.85, 407.45, PathPointTypeBezier, 0, 0}, /*7*/
    {102.54, 399.66, PathPointTypeBezier, 0, 0}, /*8*/
    {103.40, 391.91, PathPointTypeBezier, 0, 0}, /*9*/
    {104.46, 384.21, PathPointTypeBezier, 0, 0}, /*10*/
    {409.92, 110.20, PathPointTypeLine, 0, 0}, /*11*/
    {543.96, 156.53, PathPointTypeBezier, 0, 0}, /*12*/
    {625.80, 346.22, PathPointTypeBezier, 0, 0}, /*13*/
    {592.71, 533.88, PathPointTypeBezier, 0, 0}, /*14*/
    {592.47, 535.28, PathPointTypeBezier, 0, 0}, /*15*/
    {592.22, 536.67, PathPointTypeBezier, 0, 0}, /*16*/
    {591.96, 538.06, PathPointTypeBezier, 0, 0}, /*17*/
    {319.61, 797.40, PathPointTypeLine, 0, 0}, /*18*/
    {182.56, 773.90, PathPointTypeBezier, 0, 0}, /*19*/
    {85.07, 599.31, PathPointTypeBezier, 0, 0}, /*20*/
    {101.85, 407.45, PathPointTypeBezier, 0, 0}, /*21*/
    {102.54, 399.66, PathPointTypeBezier, 0, 0}, /*22*/
    {103.40, 391.91, PathPointTypeBezier, 0, 0}, /*23*/
    {104.46, 384.21, PathPointTypeBezier, 0, 0} /*24*/
    };

static void test_pathpath(void)
{
    GpStatus status;
    GpPath* path1, *path2;

    GdipCreatePath(FillModeAlternate, &path2);
    GdipAddPathArc(path2, 100.0, 100.0, 500.0, 700.0, 95.0, 100.0);

    GdipCreatePath(FillModeAlternate, &path1);
    GdipAddPathArc(path1, 100.0, 100.0, 500.0, 700.0, 0.0, 90.0);
    status = GdipAddPathPath(path1, path2, FALSE);
    expect(Ok, status);
    GdipAddPathArc(path1, 100.0, 100.0, 500.0, 700.0, -80.0, 100.0);
    status = GdipAddPathPath(path1, path2, TRUE);
    expect(Ok, status);

    ok_path(path1, pathpath_path, ARRAY_SIZE(pathpath_path), FALSE);

    GdipDeletePath(path1);
    GdipDeletePath(path2);
}

static path_test_t ellipse_path[] = {
    {30.00, 125.25, PathPointTypeStart, 0, 0}, /*0*/
    {30.00, 139.20, PathPointTypeBezier, 0, 0}, /*1*/
    {25.52, 150.50, PathPointTypeBezier, 0, 0}, /*2*/
    {20.00, 150.50, PathPointTypeBezier, 0, 0}, /*3*/
    {14.48, 150.50, PathPointTypeBezier, 0, 0}, /*4*/
    {10.00, 139.20, PathPointTypeBezier, 0, 0}, /*5*/
    {10.00, 125.25, PathPointTypeBezier, 0, 0}, /*6*/
    {10.00, 111.30, PathPointTypeBezier, 0, 0}, /*7*/
    {14.48, 100.00, PathPointTypeBezier, 0, 0}, /*8*/
    {20.00, 100.00, PathPointTypeBezier, 0, 0}, /*9*/
    {25.52, 100.00, PathPointTypeBezier, 0, 0}, /*10*/
    {30.00, 111.30, PathPointTypeBezier, 0, 0}, /*11*/
    {30.00, 125.25, PathPointTypeBezier | PathPointTypeCloseSubpath, 0, 0}, /*12*/
    {7.00, 11.00, PathPointTypeStart, 0, 0}, /*13*/
    {13.00, 17.00, PathPointTypeLine, 0, 0}, /*14*/
    {5.00, 195.00, PathPointTypeStart, 0, 0}, /*15*/
    {5.00, 192.24, PathPointTypeBezier, 0, 0}, /*16*/
    {6.12, 190.00, PathPointTypeBezier, 0, 0}, /*17*/
    {7.50, 190.00, PathPointTypeBezier, 0, 0}, /*18*/
    {8.88, 190.00, PathPointTypeBezier, 0, 0}, /*19*/
    {10.00, 192.24, PathPointTypeBezier, 0, 0}, /*20*/
    {10.00, 195.00, PathPointTypeBezier, 0, 0}, /*21*/
    {10.00, 197.76, PathPointTypeBezier, 0, 0}, /*22*/
    {8.88, 200.00, PathPointTypeBezier, 0, 0}, /*23*/
    {7.50, 200.00, PathPointTypeBezier, 0, 0}, /*24*/
    {6.12, 200.00, PathPointTypeBezier, 0, 0}, /*25*/
    {5.00, 197.76, PathPointTypeBezier, 0, 0}, /*26*/
    {5.00, 195.00, PathPointTypeBezier | PathPointTypeCloseSubpath, 0, 0}, /*27*/
    {10.00, 300.50, PathPointTypeStart, 0, 0}, /*28*/
    {10.00, 300.78, PathPointTypeBezier, 0, 0}, /*29*/
    {10.00, 301.00, PathPointTypeBezier, 0, 0}, /*30*/
    {10.00, 301.00, PathPointTypeBezier, 0, 0}, /*31*/
    {10.00, 301.00, PathPointTypeBezier, 0, 0}, /*32*/
    {10.00, 300.78, PathPointTypeBezier, 0, 0}, /*33*/
    {10.00, 300.50, PathPointTypeBezier, 0, 0}, /*34*/
    {10.00, 300.22, PathPointTypeBezier, 0, 0}, /*35*/
    {10.00, 300.00, PathPointTypeBezier, 0, 0}, /*36*/
    {10.00, 300.00, PathPointTypeBezier, 0, 0}, /*37*/
    {10.00, 300.00, PathPointTypeBezier, 0, 0}, /*38*/
    {10.00, 300.22, PathPointTypeBezier, 0, 0}, /*39*/
    {10.00, 300.50, PathPointTypeBezier | PathPointTypeCloseSubpath, 0, 0} /*40*/
    };

static void test_ellipse(void)
{
    GpStatus status;
    GpPath *path;
    GpPointF points[2];

    points[0].X = 7.0;
    points[0].Y = 11.0;
    points[1].X = 13.0;
    points[1].Y = 17.0;

    GdipCreatePath(FillModeAlternate, &path);
    status = GdipAddPathEllipse(path, 10.0, 100.0, 20.0, 50.5);
    expect(Ok, status);
    GdipAddPathLine2(path, points, 2);
    status = GdipAddPathEllipse(path, 10.0, 200.0, -5.0, -10.0);
    expect(Ok, status);
    GdipClosePathFigure(path);
    status = GdipAddPathEllipse(path, 10.0, 300.0, 0.0, 1.0);
    expect(Ok, status);

    ok_path(path, ellipse_path, ARRAY_SIZE(ellipse_path), FALSE);

    GdipDeletePath(path);
}

static path_test_t linei_path[] = {
    {5.00, 5.00, PathPointTypeStart, 0, 0}, /*0*/
    {6.00, 8.00, PathPointTypeLine, 0, 0}, /*1*/
    {409.92, 110.20, PathPointTypeLine, 0, 0}, /*2*/
    {543.96, 156.53, PathPointTypeBezier, 0, 0}, /*3*/
    {625.80, 346.22, PathPointTypeBezier, 0, 0}, /*4*/
    {592.71, 533.88, PathPointTypeBezier, 0, 0}, /*5*/
    {592.47, 535.28, PathPointTypeBezier, 0, 0}, /*6*/
    {592.22, 536.67, PathPointTypeBezier, 0, 0}, /*7*/
    {591.96, 538.06, PathPointTypeBezier, 0, 0}, /*8*/
    {15.00, 15.00, PathPointTypeLine, 0, 0}, /*9*/
    {26.00, 28.00, PathPointTypeLine | PathPointTypeCloseSubpath, 0, 0}, /*10*/
    {35.00, 35.00, PathPointTypeStart, 0, 0}, /*11*/
    {36.00, 38.00, PathPointTypeLine, 0, 0}, /*12*/
    {39.00, 40.00, PathPointTypeLine, 0, 0} /*13*/
    };

static void test_linei(void)
{
    GpStatus status;
    GpPath *path;

    GdipCreatePath(FillModeAlternate, &path);
    status = GdipAddPathLineI(path, 5.0, 5.0, 6.0, 8.0);
    expect(Ok, status);
    GdipAddPathArc(path, 100.0, 100.0, 500.0, 700.0, -80.0, 100.0);
    status = GdipAddPathLineI(path, 15.0, 15.0, 26.0, 28.0);
    expect(Ok, status);
    GdipClosePathFigure(path);
    status = GdipAddPathLineI(path, 35.0, 35.0, 36.0, 38.0);
    expect(Ok, status);
    status = GdipAddPathLineI(path, 36, 38, 39, 40);
    expect(Ok, status);

    ok_path(path, linei_path, ARRAY_SIZE(linei_path), FALSE);

    GdipDeletePath(path);
}

static path_test_t poly_path[] = {
    {5.00, 5.00, PathPointTypeStart, 0, 0},   /*1*/
    {6.00, 8.00, PathPointTypeLine, 0, 0},    /*2*/
    {0.00,  0.00,  PathPointTypeStart, 0, 0}, /*3*/
    {10.00, 10.00, PathPointTypeLine, 0, 0},  /*4*/
    {10.00, 20.00, PathPointTypeLine, 0, 0},  /*5*/
    {30.00, 10.00, PathPointTypeLine, 0, 0},  /*6*/
    {20.00, 0.00, PathPointTypeLine | PathPointTypeCloseSubpath, 0, 0}, /*7*/
    };

static void test_polygon(void)
{
    GpStatus status;
    GpPath *path;
    GpPointF points[5];

    points[0].X = 0.0;
    points[0].Y = 0.0;
    points[1].X = 10.0;
    points[1].Y = 10.0;
    points[2].X = 10.0;
    points[2].Y = 20.0;
    points[3].X = 30.0;
    points[3].Y = 10.0;
    points[4].X = 20.0;
    points[4].Y = 0.0;

    GdipCreatePath(FillModeAlternate, &path);

    /* NULL args */
    status = GdipAddPathPolygon(NULL, points, 5);
    expect(InvalidParameter, status);
    status = GdipAddPathPolygon(path, NULL, 5);
    expect(InvalidParameter, status);
    /* Polygon should have 3 points at least */
    status = GdipAddPathPolygon(path, points, 2);
    expect(InvalidParameter, status);

    /* to test how it prolongs not empty path */
    status = GdipAddPathLine(path, 5.0, 5.0, 6.0, 8.0);
    expect(Ok, status);
    status = GdipAddPathPolygon(path, points, 5);
    expect(Ok, status);
    /* check resulting path */
    ok_path(path, poly_path, ARRAY_SIZE(poly_path), FALSE);

    GdipDeletePath(path);
}

static path_test_t rect_path[] = {
    {5.0, 5.0,       PathPointTypeStart, 0, 0}, /*0*/
    {105.0, 5.0,     PathPointTypeLine,  0, 0}, /*1*/
    {105.0, 55.0,    PathPointTypeLine,  0, 0}, /*2*/
    {5.0, 55.0,      PathPointTypeLine | PathPointTypeCloseSubpath, 0, 0}, /*3*/

    {100.0, 50.0,    PathPointTypeStart, 0, 0}, /*4*/
    {220.0, 50.0,    PathPointTypeLine,  0, 0}, /*5*/
    {220.0, 80.0,    PathPointTypeLine,  0, 0}, /*6*/
    {100.0, 80.0,    PathPointTypeLine | PathPointTypeCloseSubpath, 0, 0}  /*7*/
    };

static void test_rect(void)
{
    GpStatus status;
    GpPath *path;
    GpRectF rects[2];

    GdipCreatePath(FillModeAlternate, &path);
    status = GdipAddPathRectangle(path, 5.0, 5.0, 100.0, 50.0);
    expect(Ok, status);
    status = GdipAddPathRectangle(path, 100.0, 50.0, 120.0, 30.0);
    expect(Ok, status);

    ok_path(path, rect_path, ARRAY_SIZE(rect_path), FALSE);

    GdipDeletePath(path);

    GdipCreatePath(FillModeAlternate, &path);

    rects[0].X      = 5.0;
    rects[0].Y      = 5.0;
    rects[0].Width  = 100.0;
    rects[0].Height = 50.0;
    rects[1].X      = 100.0;
    rects[1].Y      = 50.0;
    rects[1].Width  = 120.0;
    rects[1].Height = 30.0;

    status = GdipAddPathRectangles(path, (GDIPCONST GpRectF*)&rects, 2);
    expect(Ok, status);

    ok_path(path, rect_path, ARRAY_SIZE(rect_path), FALSE);

    GdipDeletePath(path);
}

static void test_lastpoint(void)
{
    GpStatus status;
    GpPath *path;
    GpPointF ptf;

    GdipCreatePath(FillModeAlternate, &path);
    status = GdipAddPathRectangle(path, 5.0, 5.0, 100.0, 50.0);
    expect(Ok, status);

    /* invalid args */
    status = GdipGetPathLastPoint(NULL, &ptf);
    expect(InvalidParameter, status);
    status = GdipGetPathLastPoint(path, NULL);
    expect(InvalidParameter, status);
    status = GdipGetPathLastPoint(NULL, NULL);
    expect(InvalidParameter, status);

    status = GdipGetPathLastPoint(path, &ptf);
    expect(Ok, status);
    expect(TRUE, (ptf.X == 5.0) && (ptf.Y == 55.0));

    GdipDeletePath(path);
}

static path_test_t addcurve_path[] = {
    {0.0, 0.0,   PathPointTypeStart,  0, 0}, /*0*/
    {3.3, 3.3,   PathPointTypeBezier, 0, 0}, /*1*/
    {6.7, 3.3,   PathPointTypeBezier, 0, 0}, /*2*/
    {10.0, 10.0, PathPointTypeBezier, 0, 0}, /*3*/
    {13.3, 16.7, PathPointTypeBezier, 0, 0}, /*4*/
    {3.3,  20.0, PathPointTypeBezier, 0, 0}, /*5*/
    {10.0, 20.0, PathPointTypeBezier, 0, 0}, /*6*/
    {16.7, 20.0, PathPointTypeBezier, 0, 0}, /*7*/
    {23.3, 13.3, PathPointTypeBezier, 0, 0}, /*8*/
    {30.0, 10.0, PathPointTypeBezier, 0, 0}  /*9*/
    };
static path_test_t addcurve_path_default_tension[] = {
    {0.0, 0.0,   PathPointTypeStart,  0, 0}, /*0*/
    {1.66, 1.66, PathPointTypeBezier, 0, 0}, /*1*/
    {8.33, 6.66, PathPointTypeBezier, 0, 0}, /*2*/
    {10.0, 10.0, PathPointTypeBezier, 0, 0}, /*3*/
    {11.6, 13.3, PathPointTypeBezier, 0, 0}, /*4*/
    {6.66, 20.0, PathPointTypeBezier, 0, 0}, /*5*/
    {10.0, 20.0, PathPointTypeBezier, 0, 0}, /*6*/
    {13.3, 20.0, PathPointTypeBezier, 0, 0}, /*7*/
    {26.6, 11.6, PathPointTypeBezier, 0, 0}, /*8*/
    {30.0, 10.0, PathPointTypeBezier, 0, 0}  /*9*/
    };
static path_test_t addcurve_path2[] = {
    {100.0,120.0,PathPointTypeStart,  0, 0}, /*0*/
    {123.0,10.0, PathPointTypeLine,   0, 0}, /*1*/
    {0.0, 0.0,   PathPointTypeLine,   0, 0}, /*2*/
    {3.3, 3.3,   PathPointTypeBezier, 0, 0}, /*3*/
    {6.7, 3.3,   PathPointTypeBezier, 0, 0}, /*4*/
    {10.0, 10.0, PathPointTypeBezier, 0, 0}, /*5*/
    {13.3, 16.7, PathPointTypeBezier, 0, 0}, /*6*/
    {3.3,  20.0, PathPointTypeBezier, 0, 0}, /*7*/
    {10.0, 20.0, PathPointTypeBezier, 0, 0}, /*8*/
    {16.7, 20.0, PathPointTypeBezier, 0, 0}, /*9*/
    {23.3, 13.3, PathPointTypeBezier, 0, 0}, /*10*/
    {30.0, 10.0, PathPointTypeBezier, 0, 0}  /*11*/
    };
static path_test_t addcurve_path3[] = {
    {10.0, 10.0, PathPointTypeStart,  0, 0}, /*0*/
    {13.3, 16.7, PathPointTypeBezier, 0, 0}, /*1*/
    {3.3,  20.0, PathPointTypeBezier, 0, 0}, /*2*/
    {10.0, 20.0, PathPointTypeBezier, 0, 0}, /*3*/
    {16.7, 20.0, PathPointTypeBezier, 0, 0}, /*4*/
    {23.3, 13.3, PathPointTypeBezier, 0, 0}, /*5*/
    {30.0, 10.0, PathPointTypeBezier, 0, 0}  /*6*/
    };
static path_test_t addcurve_path4[] = {
    {0.0,  0.0,  PathPointTypeStart,  0, 0}, /*0*/
    {3.33, 3.33, PathPointTypeBezier, 0, 0}, /*1*/
    {6.66, 3.33, PathPointTypeBezier, 0, 0}, /*2*/
    {10.0, 10.0, PathPointTypeBezier, 0, 0}, /*3*/
    };
static path_test_t addcurve_path5[] = {
    {10.0, 10.0, PathPointTypeStart,  0, 0}, /*0*/
    {13.3, 16.6, PathPointTypeBezier, 0, 0}, /*1*/
    {3.33, 20.0, PathPointTypeBezier, 0, 0}, /*2*/
    {10.0, 20.0, PathPointTypeBezier, 0, 0}  /*3*/
    };

static void test_addcurve(void)
{
    GpStatus status;
    GpPath *path;
    GpPointF points[4];

    points[0].X = 0.0;
    points[0].Y = 0.0;
    points[1].X = 10.0;
    points[1].Y = 10.0;
    points[2].X = 10.0;
    points[2].Y = 20.0;
    points[3].X = 30.0;
    points[3].Y = 10.0;

    GdipCreatePath(FillModeAlternate, &path);

    /* NULL args */
    status = GdipAddPathCurve2(NULL, NULL, 0, 0.0);
    expect(InvalidParameter, status);
    status = GdipAddPathCurve2(path, NULL, 0, 0.0);
    expect(InvalidParameter, status);
    status = GdipAddPathCurve2(path, points, -1, 0.0);
    expect(InvalidParameter, status);
    status = GdipAddPathCurve2(path, points, 1, 1.0);
    expect(InvalidParameter, status);

    /* add to empty path */
    status = GdipAddPathCurve2(path, points, 4, 1.0);
    expect(Ok, status);
    ok_path(path, addcurve_path, ARRAY_SIZE(addcurve_path), FALSE);
    GdipDeletePath(path);

    /* add to empty path with default tension */
    GdipCreatePath(FillModeAlternate, &path);
    status = GdipAddPathCurve(path, points, 4);
    expect(Ok, status);
    ok_path(path, addcurve_path_default_tension, ARRAY_SIZE(addcurve_path_default_tension), FALSE);
    GdipDeletePath(path);

    /* add to notempty path and opened figure */
    GdipCreatePath(FillModeAlternate, &path);
    GdipAddPathLine(path, 100.0, 120.0, 123.0, 10.0);
    status = GdipAddPathCurve2(path, points, 4, 1.0);
    expect(Ok, status);
    ok_path(path, addcurve_path2, ARRAY_SIZE(addcurve_path2), FALSE);

    /* NULL args */
    GdipResetPath(path);
    status = GdipAddPathCurve3(NULL, NULL, 0, 0, 0, 0.0);
    expect(InvalidParameter, status);
    status = GdipAddPathCurve3(path, NULL, 0, 0, 0, 0.0);
    expect(InvalidParameter, status);
    /* wrong count, offset.. */
    status = GdipAddPathCurve3(path, points, 0, 0, 0, 0.0);
    expect(InvalidParameter, status);
    status = GdipAddPathCurve3(path, points, 4, 0, 0, 0.0);
    expect(InvalidParameter, status);
    status = GdipAddPathCurve3(path, points, 4, 0, 4, 0.0);
    expect(InvalidParameter, status);
    status = GdipAddPathCurve3(path, points, 4, 1, 3, 0.0);
    expect(InvalidParameter, status);
    status = GdipAddPathCurve3(path, points, 4, 1, 0, 0.0);
    expect(InvalidParameter, status);
    status = GdipAddPathCurve3(path, points, 4, 3, 1, 0.0);
    expect(InvalidParameter, status);

    /* use all points */
    status = GdipAddPathCurve3(path, points, 4, 0, 3, 1.0);
    expect(Ok, status);
    ok_path(path, addcurve_path, ARRAY_SIZE(addcurve_path), FALSE);
    GdipResetPath(path);

    /* Skip first point */
    status = GdipAddPathCurve3(path, points, 4, 1, 2, 1.0);
    expect(Ok, status);
    ok_path(path, addcurve_path3, ARRAY_SIZE(addcurve_path3), FALSE);
    GdipResetPath(path);

    /* Skip two last points */
    status = GdipAddPathCurve3(path, points, 4, 0, 1, 1.0);
    expect(Ok, status);
    ok_path(path, addcurve_path4, ARRAY_SIZE(addcurve_path4), FALSE);
    GdipResetPath(path);

    /* Skip first and last points */
    status = GdipAddPathCurve3(path, points, 4, 1, 1, 1.0);
    expect(Ok, status);
    ok_path(path, addcurve_path5, ARRAY_SIZE(addcurve_path5), FALSE);
    GdipResetPath(path);

    GdipDeletePath(path);
}

static path_test_t addclosedcurve_path[] = {
    {0.0, 0.0,   PathPointTypeStart,  0, 0}, /*0*/
    {-6.7, 0.0,  PathPointTypeBezier, 0, 0}, /*1*/
    {6.7, 3.3,   PathPointTypeBezier, 0, 0}, /*2*/
    {10.0, 10.0, PathPointTypeBezier, 0, 0}, /*3*/
    {13.3, 16.7, PathPointTypeBezier, 0, 0}, /*4*/
    {3.3,  20.0, PathPointTypeBezier, 0, 0}, /*5*/
    {10.0, 20.0, PathPointTypeBezier, 0, 0}, /*6*/
    {16.7, 20.0, PathPointTypeBezier, 0, 0}, /*7*/
    {33.3, 16.7, PathPointTypeBezier, 0, 0}, /*8*/
    {30.0, 10.0, PathPointTypeBezier, 0, 0}, /*9*/
    {26.7, 3.3,  PathPointTypeBezier, 0, 0}, /*10*/
    {6.7,  0.0,  PathPointTypeBezier, 0, 0}, /*11*/
    {0.0,  0.0,  PathPointTypeBezier | PathPointTypeCloseSubpath, 0, 0}  /*12*/
    };
static path_test_t addclosedcurve_path_default_tension[] = {
    {0.0, 0.0,   PathPointTypeStart,  0, 0}, /*0*/
    {-3.33, 0.0, PathPointTypeBezier, 0, 0}, /*1*/
    {8.33, 6.66, PathPointTypeBezier, 0, 0}, /*2*/
    {10.0, 10.0, PathPointTypeBezier, 0, 0}, /*3*/
    {11.6, 13.3, PathPointTypeBezier, 0, 0}, /*4*/
    {6.66, 20.0, PathPointTypeBezier, 0, 0}, /*5*/
    {10.0, 20.0, PathPointTypeBezier, 0, 0}, /*6*/
    {13.3, 20.0, PathPointTypeBezier, 0, 0}, /*7*/
    {31.6, 13.3, PathPointTypeBezier, 0, 0}, /*8*/
    {30.0, 10.0, PathPointTypeBezier, 0, 0}, /*9*/
    {28.3, 6.66, PathPointTypeBezier, 0, 0}, /*10*/
    {3.33, 0.0,  PathPointTypeBezier, 0, 0}, /*11*/
    {0.0,  0.0,  PathPointTypeBezier | PathPointTypeCloseSubpath, 0, 0}  /*12*/
    };
static void test_addclosedcurve(void)
{
    GpStatus status;
    GpPath *path;
    GpPointF points[4];

    points[0].X = 0.0;
    points[0].Y = 0.0;
    points[1].X = 10.0;
    points[1].Y = 10.0;
    points[2].X = 10.0;
    points[2].Y = 20.0;
    points[3].X = 30.0;
    points[3].Y = 10.0;

    GdipCreatePath(FillModeAlternate, &path);

    /* NULL args */
    status = GdipAddPathClosedCurve2(NULL, NULL, 0, 0.0);
    expect(InvalidParameter, status);
    status = GdipAddPathClosedCurve2(path, NULL, 0, 0.0);
    expect(InvalidParameter, status);
    status = GdipAddPathClosedCurve2(path, points, -1, 0.0);
    expect(InvalidParameter, status);
    status = GdipAddPathClosedCurve2(path, points, 1, 1.0);
    expect(InvalidParameter, status);

    /* add to empty path */
    status = GdipAddPathClosedCurve2(path, points, 4, 1.0);
    expect(Ok, status);
    ok_path(path, addclosedcurve_path, ARRAY_SIZE(addclosedcurve_path), FALSE);
    GdipDeletePath(path);

     /* add to empty path with default tension */
    GdipCreatePath(FillModeAlternate, &path);
    status = GdipAddPathClosedCurve(path, points, 4);
    expect(Ok, status);
    ok_path(path, addclosedcurve_path_default_tension, ARRAY_SIZE(addclosedcurve_path_default_tension), FALSE);
    GdipDeletePath(path);
}

static path_test_t reverse_path[] = {
    {0.0,  20.0, PathPointTypeStart, 0, 0}, /*0*/
    {25.0, 25.0, PathPointTypeLine,  0, 0}, /*1*/
    {0.0,  30.0, PathPointTypeLine,  0, 0}, /*2*/
    {15.0, 35.0, PathPointTypeStart, 0, 0}, /*3*/
    {0.0,  40.0, PathPointTypeLine,  0, 0}, /*4*/
    {5.0,  45.0, PathPointTypeLine,  0, 0}, /*5*/
    {0.0,  50.0, PathPointTypeLine | PathPointTypeCloseSubpath, 0, 0}  /*6*/
    };

static void test_reverse(void)
{
    GpStatus status;
    GpPath *path;
    GpPointF pts[7];
    INT i;

    for(i = 0; i < 7; i++){
        pts[i].X = i * 5.0 * (REAL)(i % 2);
        pts[i].Y = 50.0 - i * 5.0;
    }

    GdipCreatePath(FillModeAlternate, &path);

    /* NULL argument */
    status = GdipReversePath(NULL);
    expect(InvalidParameter, status);

    /* empty path */
    status = GdipReversePath(path);
    expect(Ok, status);

    GdipAddPathLine2(path, pts, 4);
    GdipClosePathFigure(path);
    GdipAddPathLine2(path, &(pts[4]), 3);

    status = GdipReversePath(path);
    expect(Ok, status);
    ok_path(path, reverse_path, ARRAY_SIZE(reverse_path), FALSE);

    GdipDeletePath(path);
}

static path_test_t addpie_path[] = {
    {50.0, 25.0, PathPointTypeStart, 0, 0}, /*0*/
    {97.2, 33.3, PathPointTypeLine,  0, 0}, /*1*/
    {91.8, 40.9, PathPointTypeBezier,0, 0}, /*2*/
    {79.4, 46.8, PathPointTypeBezier,0, 0}, /*3*/
    {63.9, 49.0, PathPointTypeBezier | PathPointTypeCloseSubpath,  0, 0} /*4*/
    };
static path_test_t addpie_path2[] = {
    {0.0, 30.0, PathPointTypeStart | PathPointTypeCloseSubpath, 0, 0} /*0*/
    };
static path_test_t addpie_path3[] = {
    {30.0, 0.0, PathPointTypeStart | PathPointTypeCloseSubpath, 0, 0} /*0*/
    };
static void test_addpie(void)
{
    GpStatus status;
    GpPath *path;

    GdipCreatePath(FillModeAlternate, &path);

    /* NULL argument */
    status = GdipAddPathPie(NULL, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    expect(InvalidParameter, status);

    status = GdipAddPathPie(path, 0.0, 0.0, 100.0, 50.0, 10.0, 50.0);
    expect(Ok, status);
    ok_path(path, addpie_path, ARRAY_SIZE(addpie_path), FALSE);
    status = GdipResetPath(path);
    expect(Ok, status);

    /* zero width base ellipse */
    status = GdipAddPathPie(path, 0.0, 0.0, 0.0, 60.0, -90.0, 24.0);
    expect(InvalidParameter, status);
    ok_path(path, addpie_path2, ARRAY_SIZE(addpie_path2), FALSE);
    status = GdipResetPath(path);
    expect(Ok, status);

    /* zero height base ellipse */
    status = GdipAddPathPie(path, 0.0, 0.0, 60.0, 0.0 , -90.0, 24.0);
    expect(InvalidParameter, status);
    ok_path(path, addpie_path3, ARRAY_SIZE(addpie_path3), FALSE);

    GdipDeletePath(path);
}

static path_test_t flattenellipse_path[] = {
    {100.0, 25.0,PathPointTypeStart, 0, 0}, /*0*/
    {99.0, 30.0, PathPointTypeLine,  0, 0}, /*1*/
    {96.0, 34.8, PathPointTypeLine,  0, 0}, /*2*/
    {91.5, 39.0, PathPointTypeLine,  0, 0}, /*3*/
    {85.5, 42.8, PathPointTypeLine,  0, 0}, /*4*/
    {69.5, 48.0, PathPointTypeLine,  0, 1}, /*5*/
    {50.0, 50.0, PathPointTypeLine,  0, 1}, /*6*/
    {30.5, 48.0, PathPointTypeLine,  0, 1}, /*7*/
    {14.8, 42.8, PathPointTypeLine,  0, 1}, /*8*/
    {8.5,  39.0, PathPointTypeLine,  0, 1}, /*9*/
    {4.0,  34.8, PathPointTypeLine,  0, 1}, /*10*/
    {1.0,  30.0, PathPointTypeLine,  0, 1}, /*11*/
    {0.0,  25.0, PathPointTypeLine,  0, 1}, /*12*/
    {1.0,  20.0, PathPointTypeLine,  0, 1}, /*13*/
    {4.0,  15.3, PathPointTypeLine,  0, 1}, /*14*/
    {8.5,  11.0, PathPointTypeLine,  0, 1}, /*15*/
    {14.8, 7.3,  PathPointTypeLine,  0, 1}, /*16*/
    {30.5, 2.0,  PathPointTypeLine,  0, 1}, /*17*/
    {50.0, 0.0,  PathPointTypeLine,  0, 1}, /*18*/
    {69.5, 2.0,  PathPointTypeLine,  0, 1}, /*19*/
    {85.5, 7.3,  PathPointTypeLine,  0, 1}, /*20*/
    {91.5, 11.0, PathPointTypeLine,  0, 1}, /*21*/
    {96.0, 15.3, PathPointTypeLine,  0, 1}, /*22*/
    {99.0, 20.0, PathPointTypeLine,  0, 1}, /*23*/
    {100.0,25.0, PathPointTypeLine | PathPointTypeCloseSubpath,  0, 1}  /*24*/
    };

static path_test_t flattenline_path[] = {
    {5.0, 10.0,PathPointTypeStart, 0, 0}, /*0*/
    {50.0, 100.0, PathPointTypeLine,  0, 0} /*1*/
    };

static path_test_t flattenarc_path[] = {
    {100.0, 25.0,PathPointTypeStart, 0, 0}, /*0*/
    {99.0, 30.0, PathPointTypeLine,  0, 0}, /*1*/
    {96.0, 34.8, PathPointTypeLine,  0, 0}, /*2*/
    {91.5, 39.0, PathPointTypeLine,  0, 0}, /*3*/
    {85.5, 42.8, PathPointTypeLine,  0, 0}, /*4*/
    {69.5, 48.0, PathPointTypeLine,  0, 1}, /*5*/
    {50.0, 50.0, PathPointTypeLine,  0, 1}  /*6*/
    };

static path_test_t flattenquater_path[] = {
    {100.0, 50.0,PathPointTypeStart, 0, 0}, /*0*/
    {99.0, 60.0, PathPointTypeLine,  0, 0}, /*1*/
    {96.0, 69.5, PathPointTypeLine,  0, 0}, /*2*/
    {91.5, 78.0, PathPointTypeLine,  0, 0}, /*3*/
    {85.5, 85.5, PathPointTypeLine,  0, 0}, /*4*/
    {78.0, 91.5, PathPointTypeLine,  0, 0}, /*5*/
    {69.5, 96.0, PathPointTypeLine,  0, 0}, /*6*/
    {60.0, 99.0, PathPointTypeLine,  0, 0}, /*7*/
    {50.0, 100.0,PathPointTypeLine,  0, 0}  /*8*/
    };

static void test_flatten(void)
{
    GpStatus status;
    GpPath *path;
    GpMatrix *m;

    status = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, status);
    status = GdipCreateMatrix(&m);
    expect(Ok, status);

    /* NULL arguments */
    status = GdipFlattenPath(NULL, NULL, 0.0);
    expect(InvalidParameter, status);
    status = GdipFlattenPath(NULL, m, 0.0);
    expect(InvalidParameter, status);

    /* flatten empty path */
    status = GdipFlattenPath(path, NULL, 1.0);
    expect(Ok, status);

    status = GdipTransformPath(path, 0);
    expect(Ok, status);

    status = GdipAddPathEllipse(path, 0.0, 0.0, 100.0, 50.0);
    expect(Ok, status);

    status = GdipFlattenPath(path, NULL, 1.0);
    expect(Ok, status);
    ok_path(path, flattenellipse_path, ARRAY_SIZE(flattenellipse_path), TRUE);

    status = GdipResetPath(path);
    expect(Ok, status);
    status = GdipAddPathLine(path, 5.0, 10.0, 50.0, 100.0);
    expect(Ok, status);
    status = GdipFlattenPath(path, NULL, 1.0);
    expect(Ok, status);
    ok_path(path, flattenline_path, ARRAY_SIZE(flattenline_path), FALSE);

    status = GdipResetPath(path);
    expect(Ok, status);
    status = GdipAddPathArc(path, 0.0, 0.0, 100.0, 50.0, 0.0, 90.0);
    expect(Ok, status);
    status = GdipFlattenPath(path, NULL, 1.0);
    expect(Ok, status);
    ok_path(path, flattenarc_path, ARRAY_SIZE(flattenarc_path), TRUE);

    /* easy case - quater of a full circle */
    status = GdipResetPath(path);
    expect(Ok, status);
    status = GdipAddPathArc(path, 0.0, 0.0, 100.0, 100.0, 0.0, 90.0);
    expect(Ok, status);
    status = GdipFlattenPath(path, NULL, 1.0);
    expect(Ok, status);
    ok_path(path, flattenquater_path, ARRAY_SIZE(flattenquater_path), FALSE);

    status = GdipResetPath(path);
    expect(Ok, status);
    status = GdipStartPathFigure(path);
    expect(Ok, status);

    /* path seen in the wild that caused a stack overflow */
    /* same path but redo with the manual points that caused a crash */
    status = GdipAddPathBezier(path, 154.950806, 33.391144, 221.586075, 15.536285, 291.747314, 15.536285, 358.382568, 33.391144);
    expect(Ok, status);
    status = GdipAddPathBezier(path, 256.666809, 412.999512, 256.666718, 412.999481, 256.666656, 412.999481, 256.666565, 412.999512);
    expect(Ok, status);
    status = GdipClosePathFigure(path);
    expect(Ok, status);
    status = GdipFlattenPath(path, NULL, 1.0);
    expect(Ok, status);

    GdipDeleteMatrix(m);
    GdipDeletePath(path);
}

static path_test_t widenline_path[] = {
    {5.0, 5.0,   PathPointTypeStart, 0, 0}, /*0*/
    {50.0, 5.0,  PathPointTypeLine,  0, 0}, /*1*/
    {50.0, 15.0, PathPointTypeLine,  0, 0}, /*2*/
    {5.0, 15.0,  PathPointTypeLine|PathPointTypeCloseSubpath,  0, 0} /*3*/
    };

static path_test_t widenline_wide_path[] = {
    {5.0, 0.0,   PathPointTypeStart, 0, 0}, /*0*/
    {50.0, 0.0,  PathPointTypeLine,  0, 0}, /*1*/
    {50.0, 20.0, PathPointTypeLine,  0, 0}, /*2*/
    {5.0, 20.0,  PathPointTypeLine|PathPointTypeCloseSubpath,  0, 0} /*3*/
    };

static path_test_t widenline_dash_path[] = {
    {5.0, 0.0,   PathPointTypeStart, 0, 0}, /*0*/
    {35.0, 0.0,  PathPointTypeLine,  0, 0}, /*1*/
    {35.0, 10.0, PathPointTypeLine,  0, 0}, /*2*/
    {5.0, 10.0,  PathPointTypeLine|PathPointTypeCloseSubpath,  0, 0}, /*3*/
    {45.0, 0.0,   PathPointTypeStart, 0, 0}, /*4*/
    {50.0, 0.0,  PathPointTypeLine,  0, 0}, /*5*/
    {50.0, 10.0, PathPointTypeLine,  0, 0}, /*6*/
    {45.0, 10.0,  PathPointTypeLine|PathPointTypeCloseSubpath,  0, 0}, /*7*/
    };

static path_test_t widenline_thin_dash_path[] = {
    {5.0, 4.75, PathPointTypeStart, 0, 0}, /*0*/
    {8.0, 4.75, PathPointTypeLine,  0, 0}, /*1*/
    {8.0, 5.25, PathPointTypeLine,  0, 0}, /*2*/
    {5.0, 5.25, PathPointTypeLine|PathPointTypeCloseSubpath,  0, 0}, /*3*/
    {9.0, 4.75, PathPointTypeStart, 0, 0}, /*4*/
    {9.5, 4.75, PathPointTypeLine,  0, 0}, /*5*/
    {9.5, 5.25, PathPointTypeLine,  0, 0}, /*6*/
    {9.0, 5.25, PathPointTypeLine|PathPointTypeCloseSubpath,  0, 0}, /*7*/
    };

static path_test_t widenline_unit_path[] = {
    {5.0, 9.5,   PathPointTypeStart, 0, 0}, /*0*/
    {50.0, 9.5,  PathPointTypeLine,  0, 0}, /*1*/
    {50.0, 10.5, PathPointTypeLine,  0, 0}, /*2*/
    {5.0, 10.5,  PathPointTypeLine|PathPointTypeCloseSubpath,  0, 0} /*3*/
    };

static void test_widen(void)
{
    GpStatus status;
    GpPath *path;
    GpPen *pen;
    GpMatrix *m;
    INT count=-1;

    status = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, status);
    status = GdipCreatePen1(0xffffffff, 10.0, UnitPixel, &pen);
    expect(Ok, status);
    status = GdipCreateMatrix(&m);
    expect(Ok, status);

    /* NULL arguments */
    status = GdipAddPathLine(path, 5.0, 10.0, 50.0, 10.0);
    expect(Ok, status);
    status = GdipWidenPath(NULL, NULL, NULL, 0.0);
    expect(InvalidParameter, status);
    status = GdipWidenPath(path, pen, m, 0.0);
    expect(Ok, status);
    status = GdipWidenPath(path, pen, NULL, 1.0);
    expect(Ok, status);
    status = GdipWidenPath(path, NULL, m, 1.0);
    expect(InvalidParameter, status);
    status = GdipWidenPath(NULL, pen, m, 1.0);
    expect(InvalidParameter, status);

    /* widen empty path */
    status = GdipResetPath(path);
    expect(Ok, status);
    status = GdipWidenPath(path, pen, m, 1.0);
    expect(OutOfMemory, status);

    /* horizontal line */
    status = GdipResetPath(path);
    expect(Ok, status);
    status = GdipAddPathLine(path, 5.0, 10.0, 50.0, 10.0);
    expect(Ok, status);

    status = GdipWidenPath(path, pen, m, 1.0);
    expect(Ok, status);
    ok_path(path, widenline_path, ARRAY_SIZE(widenline_path), FALSE);

    /* horizontal 2x stretch */
    status = GdipResetPath(path);
    expect(Ok, status);
    status = GdipAddPathLine(path, 2.5, 10.0, 25.0, 10.0);
    expect(Ok, status);

    status = GdipScaleMatrix(m, 2.0, 1.0, MatrixOrderAppend);
    expect(Ok, status);

    status = GdipWidenPath(path, pen, m, 1.0);
    expect(Ok, status);
    ok_path(path, widenline_path, ARRAY_SIZE(widenline_path), FALSE);

    /* vertical 2x stretch */
    status = GdipResetPath(path);
    expect(Ok, status);
    status = GdipAddPathLine(path, 5.0, 5.0, 50.0, 5.0);
    expect(Ok, status);

    status = GdipScaleMatrix(m, 0.5, 2.0, MatrixOrderAppend);
    expect(Ok, status);

    status = GdipWidenPath(path, pen, m, 1.0);
    expect(Ok, status);
    ok_path(path, widenline_path, ARRAY_SIZE(widenline_path), FALSE);

    status = GdipScaleMatrix(m, 1.0, 0.5, MatrixOrderAppend);
    expect(Ok, status);

    /* dashed line */
    status = GdipResetPath(path);
    expect(Ok, status);
    status = GdipAddPathLine(path, 5.0, 5.0, 50.0, 5.0);
    expect(Ok, status);

    status = GdipSetPenDashStyle(pen, DashStyleDash);
    expect(Ok, status);

    status = GdipWidenPath(path, pen, m, 1.0);
    expect(Ok, status);
    ok_path(path, widenline_dash_path, ARRAY_SIZE(widenline_dash_path), FALSE);

    status = GdipSetPenDashStyle(pen, DashStyleSolid);
    expect(Ok, status);

    /* dashed line less than 1 pixel wide */
    GdipDeletePen(pen);
    GdipCreatePen1(0xffffffff, 0.5, UnitPixel, &pen);
    GdipSetPenDashStyle(pen, DashStyleDash);

    GdipResetPath(path);
    GdipAddPathLine(path, 5.0, 5.0, 9.5, 5.0);

    status = GdipWidenPath(path, pen, m, 1.0);
    expect(Ok, status);
    ok_path_fudge(path, widenline_thin_dash_path, ARRAY_SIZE(widenline_thin_dash_path), FALSE, 0.000005);

    /* pen width in UnitWorld */
    GdipDeletePen(pen);
    status = GdipCreatePen1(0xffffffff, 10.0, UnitWorld, &pen);
    expect(Ok, status);

    status = GdipResetPath(path);
    expect(Ok, status);
    status = GdipAddPathLine(path, 5.0, 10.0, 50.0, 10.0);
    expect(Ok, status);

    status = GdipWidenPath(path, pen, m, 1.0);
    expect(Ok, status);
    ok_path(path, widenline_path, ARRAY_SIZE(widenline_path), FALSE);

    /* horizontal 2x stretch */
    status = GdipResetPath(path);
    expect(Ok, status);
    status = GdipAddPathLine(path, 2.5, 10.0, 25.0, 10.0);
    expect(Ok, status);

    status = GdipScaleMatrix(m, 2.0, 1.0, MatrixOrderAppend);
    expect(Ok, status);

    status = GdipWidenPath(path, pen, m, 1.0);
    expect(Ok, status);
    ok_path(path, widenline_path, ARRAY_SIZE(widenline_path), FALSE);

    /* vertical 2x stretch */
    status = GdipResetPath(path);
    expect(Ok, status);
    status = GdipAddPathLine(path, 5.0, 5.0, 50.0, 5.0);
    expect(Ok, status);

    status = GdipScaleMatrix(m, 0.5, 2.0, MatrixOrderAppend);
    expect(Ok, status);

    status = GdipWidenPath(path, pen, m, 1.0);
    expect(Ok, status);
    ok_path(path, widenline_wide_path, ARRAY_SIZE(widenline_wide_path), FALSE);

    status = GdipScaleMatrix(m, 1.0, 0.5, MatrixOrderAppend);
    expect(Ok, status);

    /* pen width in UnitInch */
    GdipDeletePen(pen);
    status = GdipCreatePen1(0xffffffff, 10.0, UnitWorld, &pen);
    expect(Ok, status);

    status = GdipResetPath(path);
    expect(Ok, status);
    status = GdipAddPathLine(path, 5.0, 10.0, 50.0, 10.0);
    expect(Ok, status);

    status = GdipWidenPath(path, pen, m, 1.0);
    expect(Ok, status);
    ok_path(path, widenline_path, ARRAY_SIZE(widenline_path), FALSE);

    /* pen width = 0 pixels - native fails to widen but can draw with this pen */
    GdipDeletePen(pen);
    status = GdipCreatePen1(0xffffffff, 0.0, UnitPixel, &pen);
    expect(Ok, status);

    status = GdipResetPath(path);
    expect(Ok, status);
    status = GdipAddPathLine(path, 5.0, 10.0, 50.0, 10.0);
    expect(Ok, status);

    status = GdipWidenPath(path, pen, m, 1.0);
    expect(Ok, status);

    status = GdipGetPointCount(path, &count);
    expect(Ok, status);
    ok(count == 0 || broken(count == 4), "expected 0, got %i\n", count);

    /* pen width = 0 pixels, UnitWorld - result is a path 1 unit wide */
    GdipDeletePen(pen);
    status = GdipCreatePen1(0xffffffff, 0.0, UnitWorld, &pen);
    expect(Ok, status);

    status = GdipResetPath(path);
    expect(Ok, status);
    status = GdipAddPathLine(path, 5.0, 10.0, 50.0, 10.0);
    expect(Ok, status);

    status = GdipWidenPath(path, pen, m, 1.0);
    expect(Ok, status);

    status = GdipGetPointCount(path, &count);
    expect(Ok, status);
    ok_path_fudge(path, widenline_unit_path, ARRAY_SIZE(widenline_unit_path), FALSE, 0.000005);

    GdipDeleteMatrix(m);
    GdipDeletePen(pen);
    GdipDeletePath(path);
}

static path_test_t widenline_capflat_path[] = {
    {5.0, 5.0,   PathPointTypeStart, 0, 0}, /*0*/
    {50.0, 5.0,  PathPointTypeLine,  0, 0}, /*1*/
    {50.0, 15.0, PathPointTypeLine,  0, 0}, /*2*/
    {5.0, 15.0,  PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0} /*3*/
    };

static path_test_t widenline_capsquare_path[] = {
    {0.0, 5.0,   PathPointTypeStart, 0, 0}, /*0*/
    {55.0, 5.0,  PathPointTypeLine,  0, 0}, /*1*/
    {55.0, 15.0, PathPointTypeLine,  0, 0}, /*2*/
    {0.0, 15.0,  PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0} /*3*/
    };

static path_test_t widenline_capround_path[] = {
    {5.0, 5.0,        PathPointTypeStart,  0, 0}, /*0*/
    {50.0, 5.0,       PathPointTypeLine,   0, 0}, /*1*/
    {52.761421, 5.0,  PathPointTypeBezier, 0, 0}, /*2*/
    {55.0, 7.238576,  PathPointTypeBezier, 0, 0}, /*3*/
    {55.0, 10.0,      PathPointTypeBezier, 0, 0}, /*4*/
    {55.0, 12.761423, PathPointTypeBezier, 0, 0}, /*5*/
    {52.761421, 15.0, PathPointTypeBezier, 0, 0}, /*6*/
    {50.0, 15.0,      PathPointTypeBezier, 0, 0}, /*7*/
    {5.0, 15.0,       PathPointTypeLine,   0, 0}, /*8*/
    {2.238576, 15.0,  PathPointTypeBezier, 0, 0}, /*9*/
    {0.0, 12.761423,  PathPointTypeBezier, 0, 0}, /*10*/
    {0.0, 10.0,       PathPointTypeBezier, 0, 0}, /*11*/
    {0.0, 7.238576,   PathPointTypeBezier, 0, 0}, /*12*/
    {2.238576, 5.0,   PathPointTypeBezier, 0, 0}, /*13*/
    {5.0, 5.0,        PathPointTypeBezier|PathPointTypeCloseSubpath, 0, 0}, /*14*/
    };

static path_test_t widenline_captriangle_path[] = {
    {5.0, 5.0,   PathPointTypeStart, 0, 0}, /*0*/
    {50.0, 5.0,  PathPointTypeLine,  0, 0}, /*1*/
    {55.0, 10.0, PathPointTypeLine,  0, 0}, /*2*/
    {50.0, 15.0, PathPointTypeLine,  0, 0}, /*3*/
    {5.0, 15.0,  PathPointTypeLine,  0, 0}, /*4*/
    {0.0, 10.0,  PathPointTypeLine,  0, 0}, /*5*/
    {5.0, 5.0,   PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0} /*6*/
    };

static path_test_t widenline_capsquareanchor_path[] = {
    {5.0, 5.0,             PathPointTypeStart, 0, 0}, /*0*/
    {50.0, 5.0,            PathPointTypeLine,  0, 0}, /*1*/
    {50.0, 15.0,           PathPointTypeLine,  0, 0}, /*2*/
    {5.0, 15.0,            PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*3*/
    {12.071068, 2.928932,  PathPointTypeStart, 0, 0}, /*4*/
    {12.071068, 17.071066, PathPointTypeLine,  0, 0}, /*5*/
    {-2.071068, 17.071066, PathPointTypeLine,  0, 0}, /*6*/
    {-2.071068, 2.928932,  PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*7*/
    {42.928928, 17.071068, PathPointTypeStart, 0, 0}, /*8*/
    {42.928928, 2.928932,  PathPointTypeLine,  0, 0}, /*9*/
    {57.071068, 2.928932,  PathPointTypeLine,  0, 0}, /*10*/
    {57.071068, 17.071068, PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*11*/
    };

static path_test_t widenline_caproundanchor_path[] = {
    {5.0, 5.0,        PathPointTypeStart,  0, 0}, /*0*/
    {50.0, 5.0,       PathPointTypeLine,   0, 0}, /*1*/
    {50.0, 15.0,      PathPointTypeLine,   0, 0}, /*2*/
    {5.0, 15.0,       PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*3*/
    {5.0, 20.0,       PathPointTypeStart,  0, 0}, /*4*/
    {-0.522847, 20.0, PathPointTypeBezier, 0, 0}, /*5*/
    {-5.0, 15.522846, PathPointTypeBezier, 0, 0}, /*6*/
    {-5.0, 10.0,      PathPointTypeBezier, 0, 0}, /*7*/
    {-5.0, 4.477152,  PathPointTypeBezier, 0, 0}, /*8*/
    {-0.522847, 0.0,  PathPointTypeBezier, 0, 0}, /*9*/
    {5.0, 0.0,        PathPointTypeBezier, 0, 0}, /*10*/
    {10.522847, 0.0,  PathPointTypeBezier, 0, 0}, /*11*/
    {15.0, 4.477152,  PathPointTypeBezier, 0, 0}, /*12*/
    {15.0, 10.0,      PathPointTypeBezier, 0, 0}, /*13*/
    {15.0, 15.522846, PathPointTypeBezier, 0, 0}, /*14*/
    {10.522847, 20.0, PathPointTypeBezier, 0, 0}, /*15*/
    {5.0, 20.0,       PathPointTypeBezier|PathPointTypeCloseSubpath, 0, 0}, /*16*/
    {50.0, 0.0,       PathPointTypeStart,  0, 0}, /*17*/
    {55.522846, 0.0,  PathPointTypeBezier, 0, 0}, /*18*/
    {60.0, 4.477153,  PathPointTypeBezier, 0, 0}, /*19*/
    {60.0, 10.0,      PathPointTypeBezier, 0, 0}, /*20*/
    {60.0, 15.522847, PathPointTypeBezier, 0, 0}, /*21*/
    {55.522846, 20.0, PathPointTypeBezier, 0, 0}, /*22*/
    {50.0, 20.0,      PathPointTypeBezier, 0, 0}, /*23*/
    {44.477150, 20.0, PathPointTypeBezier, 0, 0}, /*24*/
    {40.0, 15.522847, PathPointTypeBezier, 0, 0}, /*25*/
    {40.0, 10.0,      PathPointTypeBezier, 0, 0}, /*26*/
    {40.0, 4.477153,  PathPointTypeBezier, 0, 0}, /*27*/
    {44.477150, 0.0,  PathPointTypeBezier, 0, 0}, /*28*/
    {50.0, 0.0,       PathPointTypeBezier|PathPointTypeCloseSubpath, 0, 0}, /*29*/
    };

static path_test_t widenline_capdiamondanchor_path[] = {
    {5.0, 5.0,   PathPointTypeStart, 0, 0}, /*0*/
    {50.0, 5.0,  PathPointTypeLine,  0, 0}, /*1*/
    {50.0, 15.0, PathPointTypeLine,  0, 0}, /*2*/
    {5.0, 15.0,  PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*3*/
    {-5.0, 10.0, PathPointTypeStart, 0, 0}, /*4*/
    {5.0, 0.0,   PathPointTypeLine,  0, 0}, /*5*/
    {15.0, 10.0, PathPointTypeLine,  0, 0}, /*6*/
    {5.0, 20.0,  PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*7*/
    {60.0, 10.0, PathPointTypeStart, 0, 0}, /*8*/
    {50.0, 20.0, PathPointTypeLine,  0, 0}, /*9*/
    {40.0, 10.0, PathPointTypeLine,  0, 0}, /*10*/
    {50.0, 0.0,  PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*11*/
    };

static path_test_t widenline_caparrowanchor_path[] = {
    {15.0, 5.0,       PathPointTypeStart, 0, 1}, /*0*/
    {40.0, 5.0,       PathPointTypeLine,  0, 1}, /*1*/
    {40.0, 15.0,      PathPointTypeLine,  0, 1}, /*2*/
    {15.0, 15.0,      PathPointTypeLine|PathPointTypeCloseSubpath, 0, 1}, /*3*/
    {5.0, 10.0,       PathPointTypeStart, 0, 0}, /*4*/
    {22.320507, 0.0,  PathPointTypeLine,  0, 0}, /*5*/
    {22.320507, 20.0, PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*6*/
    {50.0, 10.0,      PathPointTypeStart, 0, 0}, /*7*/
    {32.679489, 20.0, PathPointTypeLine,  0, 0}, /*8*/
    {32.679489, 0.0,  PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*9*/
    };

static path_test_t widenline_capsquareanchor_thin_path[] = {
    {6.414213, 8.585786,   PathPointTypeStart, 0, 0}, /*0*/
    {6.414213, 11.414213,  PathPointTypeLine,  0, 0}, /*1*/
    {3.585786, 11.414213,  PathPointTypeLine,  0, 0}, /*2*/
    {3.585786, 8.585786,   PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*3*/
    {48.585785, 11.414213, PathPointTypeStart, 0, 0}, /*4*/
    {48.585785, 8.585786,  PathPointTypeLine,  0, 0}, /*5*/
    {51.414211, 8.585786,  PathPointTypeLine,  0, 0}, /*6*/
    {51.414211, 11.414213, PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*7*/
    };

static path_test_t widenline_capsquareanchor_dashed_path[] = {
    {5.0, 5.0,             PathPointTypeStart, 0, 0}, /*0*/
    {35.0, 5.0,            PathPointTypeLine,  0, 0}, /*1*/
    {35.0, 15.0,           PathPointTypeLine,  0, 0}, /*2*/
    {5.0, 15.0,            PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*3*/
    {45.0, 5.0,            PathPointTypeStart, 0, 0}, /*4*/
    {50.0, 5.0,            PathPointTypeLine,  0, 0}, /*5*/
    {50.0, 15.0,           PathPointTypeLine,  0, 0}, /*6*/
    {45.0, 15.0,           PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*7*/
    {12.071068, 2.928932,  PathPointTypeStart, 0, 0}, /*8*/
    {12.071068, 17.071066, PathPointTypeLine,  0, 0}, /*9*/
    {-2.071068, 17.071066, PathPointTypeLine,  0, 0}, /*10*/
    {-2.071068, 2.928932,  PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*11*/
    {42.928928, 17.071068, PathPointTypeStart, 0, 0}, /*12*/
    {42.928928, 2.928932,  PathPointTypeLine,  0, 0}, /*13*/
    {57.071068, 2.928932,  PathPointTypeLine,  0, 0}, /*14*/
    {57.071068, 17.071068, PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*15*/
    };

static path_test_t widenline_capsquareanchor_multifigure_path[] = {
    {5.0, 5.0,             PathPointTypeStart, 0, 0}, /*0*/
    {25.0, 5.0,            PathPointTypeLine,  0, 0}, /*1*/
    {25.0, 15.0,           PathPointTypeLine,  0, 0}, /*2*/
    {5.0, 15.0,            PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*3*/
    {30.0, 5.0,            PathPointTypeStart, 0, 0}, /*4*/
    {50.0, 5.0,            PathPointTypeLine,  0, 0}, /*5*/
    {50.0, 15.0,           PathPointTypeLine,  0, 0}, /*6*/
    {30.0, 15.0,           PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*7*/
    {12.071068, 2.928932,  PathPointTypeStart, 0, 0}, /*8*/
    {12.071068, 17.071066, PathPointTypeLine,  0, 0}, /*9*/
    {-2.071068, 17.071066, PathPointTypeLine,  0, 0}, /*10*/
    {-2.071068, 2.928932,  PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*11*/
    {17.928930, 17.071068, PathPointTypeStart, 0, 0}, /*12*/
    {17.928930, 2.928932,  PathPointTypeLine,  0, 0}, /*13*/
    {32.071068, 2.928932,  PathPointTypeLine,  0, 0}, /*14*/
    {32.071068, 17.071068, PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*15*/
    {37.071068, 2.928932,  PathPointTypeStart, 0, 0}, /*16*/
    {37.071068, 17.071066, PathPointTypeLine,  0, 0}, /*17*/
    {22.928930, 17.071066, PathPointTypeLine,  0, 0}, /*18*/
    {22.928930, 2.928932,  PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*19*/
    {42.928928, 17.071068, PathPointTypeStart, 0, 0}, /*20*/
    {42.928928, 2.928932,  PathPointTypeLine,  0, 0}, /*21*/
    {57.071068, 2.928932,  PathPointTypeLine,  0, 0}, /*22*/
    {57.071068, 17.071068, PathPointTypeLine|PathPointTypeCloseSubpath, 0, 0}, /*23*/
    };

static path_test_t widenline_customarrow_multifigure_path[] = {
    {6.0, 9.5,          PathPointTypeStart}, /*0*/
    {24.0, 9.5,         PathPointTypeLine}, /*1*/
    {24.0, 10.5,        PathPointTypeLine}, /*2*/
    {6.0, 10.5,         PathPointTypeLine|PathPointTypeCloseSubpath}, /*3*/
    {30.5, 11.0,        PathPointTypeStart}, /*4*/
    {30.5, 29.0,        PathPointTypeLine}, /*5*/
    {29.5, 29.0,        PathPointTypeLine}, /*6*/
    {29.5, 11.0,        PathPointTypeLine|PathPointTypeCloseSubpath}, /*7*/
    {13.0, 14.0,        PathPointTypeStart}, /*8*/
    {5.0, 10.0,         PathPointTypeLine}, /*9*/
    {13.0, 6.0,         PathPointTypeLine}, /*10*/
    {11.0, 10.0,        PathPointTypeLine|PathPointTypeCloseSubpath}, /*11*/
    {17.0, 6.0,         PathPointTypeStart}, /*12*/
    {25.0, 10.0,        PathPointTypeLine}, /*13*/
    {17.0, 14.0,        PathPointTypeLine}, /*14*/
    {19.0, 10.0,        PathPointTypeLine|PathPointTypeCloseSubpath}, /*15*/
    {26.0, 18.0,        PathPointTypeStart}, /*16*/
    {30.0, 10.0,        PathPointTypeLine}, /*17*/
    {34.0, 18.0,        PathPointTypeLine}, /*18*/
    {30.0, 16.0,        PathPointTypeLine|PathPointTypeCloseSubpath}, /*19*/
    {34.0, 22.0,        PathPointTypeStart}, /*20*/
    {30.0, 30.0,        PathPointTypeLine}, /*21*/
    {26.0, 22.0,        PathPointTypeLine}, /*22*/
    {30.0, 24.0,        PathPointTypeLine|PathPointTypeCloseSubpath}, /*23*/
    };

static void test_widen_cap(void)
{
    struct
    {
        LineCap type;
        REAL line_width;
        const path_test_t *expected;
        INT expected_size;
        BOOL dashed;
    }
    caps[] =
    {
        { LineCapFlat, 10.0, widenline_capflat_path,
                ARRAY_SIZE(widenline_capflat_path) },
        { LineCapSquare, 10.0, widenline_capsquare_path,
                ARRAY_SIZE(widenline_capsquare_path) },
        { LineCapRound, 10.0, widenline_capround_path,
                ARRAY_SIZE(widenline_capround_path) },
        { LineCapTriangle, 10.0, widenline_captriangle_path,
                ARRAY_SIZE(widenline_captriangle_path) },
        { LineCapNoAnchor, 10.0, widenline_capflat_path,
                ARRAY_SIZE(widenline_capflat_path) },
        { LineCapSquareAnchor, 10.0, widenline_capsquareanchor_path,
                ARRAY_SIZE(widenline_capsquareanchor_path) },
        { LineCapRoundAnchor, 10.0, widenline_caproundanchor_path,
                ARRAY_SIZE(widenline_caproundanchor_path) },
        { LineCapDiamondAnchor, 10.0, widenline_capdiamondanchor_path,
                ARRAY_SIZE(widenline_capdiamondanchor_path) },
        { LineCapArrowAnchor, 10.0, widenline_caparrowanchor_path,
                ARRAY_SIZE(widenline_caparrowanchor_path) },
        { LineCapSquareAnchor, 0.0, widenline_capsquareanchor_thin_path,
                ARRAY_SIZE(widenline_capsquareanchor_thin_path) },
        { LineCapSquareAnchor, 10.0, widenline_capsquareanchor_dashed_path,
                ARRAY_SIZE(widenline_capsquareanchor_dashed_path), TRUE },
    };

    GpAdjustableArrowCap *arrowcap;
    GpStatus status;
    GpPath *path;
    GpPen *pen;

    int i;

    status = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, status);

    for (i = 0; i < ARRAY_SIZE(caps); i++)
    {
        status = GdipCreatePen1(0xffffffff, caps[i].line_width, UnitPixel, &pen);
        expect(Ok, status);
        if (caps[i].dashed)
        {
            status = GdipSetPenDashStyle(pen, DashStyleDash);
            expect(Ok, status);
        }

        status = GdipResetPath(path);
        expect(Ok, status);
        status = GdipAddPathLine(path, 5.0, 10.0, 50.0, 10.0);
        expect(Ok, status);
        status = GdipSetPenStartCap(pen, caps[i].type);
        expect(Ok, status);
        status = GdipSetPenEndCap(pen, caps[i].type);
        expect(Ok, status);

        status = GdipWidenPath(path, pen, NULL, FlatnessDefault);
        expect(Ok, status);

        if (i == 9)
        {
            INT size;
            status = GdipGetPointCount(path, &size);
            expect(Ok, status);
            ok(size == caps[i].expected_size || broken(size == 12), "unexpected path size %i\n", size);

            if (size == 12)
            {
                GdipDeletePen(pen);
                continue;
            }
        }

        ok_path_fudge(path, caps[i].expected, caps[i].expected_size, FALSE, 0.000005);

        GdipDeletePen(pen);
    }

    status = GdipCreatePen1(0xffffffff, 10.0, UnitPixel, &pen);
    expect(Ok, status);
    status = GdipResetPath(path);
    expect(Ok, status);
    status = GdipAddPathLine(path, 5.0, 10.0, 25.0, 10.0);
    expect(Ok, status);
    status = GdipStartPathFigure(path);
    expect(Ok, status);
    status = GdipAddPathLine(path, 30.0, 10.0, 50.0, 10.0);
    expect(Ok, status);
    status = GdipSetPenStartCap(pen, LineCapSquareAnchor);
    expect(Ok, status);
    status = GdipSetPenEndCap(pen, LineCapSquareAnchor);
    expect(Ok, status);
    status = GdipWidenPath(path, pen, NULL, FlatnessDefault);
    expect(Ok, status);
    ok_path_fudge(path, widenline_capsquareanchor_multifigure_path,
        ARRAY_SIZE(widenline_capsquareanchor_multifigure_path), FALSE, 0.000005);

    status = GdipResetPath(path);
    expect(Ok, status);
    status = GdipAddPathLine(path, 5.0, 10.0, 25.0, 10.0);
    expect(Ok, status);
    status = GdipStartPathFigure(path);
    expect(Ok, status);
    status = GdipAddPathLine(path, 30.0, 10.0, 30.0, 30.0);
    expect(Ok, status);
    status = GdipCreateAdjustableArrowCap(4.0, 4.0, TRUE, &arrowcap);
    ok(status == Ok, "Failed to create adjustable cap, %d\n", status);
    status = GdipSetAdjustableArrowCapMiddleInset(arrowcap, 1.0);
    ok(status == Ok, "Failed to set middle inset inadjustable cap, %d\n", status);
    status = GdipSetPenCustomStartCap(pen, (GpCustomLineCap*)arrowcap);
    ok(status == Ok, "Failed to create custom end cap, %d\n", status);
    status = GdipSetPenCustomEndCap(pen, (GpCustomLineCap*)arrowcap);
    ok(status == Ok, "Failed to create custom end cap, %d\n", status);
    status = GdipSetPenWidth(pen, 1.0);
    expect(Ok, status);
    status = GdipWidenPath(path, pen, NULL, FlatnessDefault);
    expect(Ok, status);
    ok_path_fudge(path, widenline_customarrow_multifigure_path,
        ARRAY_SIZE(widenline_customarrow_multifigure_path), FALSE, 0.000005);

    GdipDeletePen(pen);

    GdipDeletePath(path);
}

static void test_isvisible(void)
{
    GpPath *path;
    GpGraphics *graphics = NULL;
    HDC hdc = GetDC(0);
    BOOL result;
    GpStatus status;

    status = GdipCreateFromHDC(hdc, &graphics);
    expect(Ok, status);
    status = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, status);

    /* NULL */
    status = GdipIsVisiblePathPoint(NULL, 0.0, 0.0, NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipIsVisiblePathPoint(path, 0.0, 0.0, NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipIsVisiblePathPoint(path, 0.0, 0.0, NULL, NULL);
    expect(InvalidParameter, status);
    status = GdipIsVisiblePathPoint(path, 0.0, 0.0, graphics, NULL);
    expect(InvalidParameter, status);

    /* empty path */
    result = TRUE;
    status = GdipIsVisiblePathPoint(path, 0.0, 0.0, NULL, &result);
    expect(Ok, status);
    expect(FALSE, result);
    /* rect */
    status = GdipAddPathRectangle(path, 0.0, 0.0, 10.0, 10.0);
    expect(Ok, status);
    result = FALSE;
    status = GdipIsVisiblePathPoint(path, 0.0, 0.0, NULL, &result);
    expect(Ok, status);
    expect(TRUE, result);
    result = TRUE;
    status = GdipIsVisiblePathPoint(path, 11.0, 11.0, NULL, &result);
    expect(Ok, status);
    expect(FALSE, result);
    /* not affected by clipping */
    status = GdipSetClipRect(graphics, 5.0, 5.0, 5.0, 5.0, CombineModeReplace);
    expect(Ok, status);
    result = FALSE;
    status = GdipIsVisiblePathPoint(path, 0.0, 0.0, graphics, &result);
    expect(Ok, status);
    expect(TRUE, result);
    /* not affected by world transform */
    status = GdipScaleWorldTransform(graphics, 2.0, 2.0, MatrixOrderPrepend);
    expect(Ok, status);
    result = FALSE;
    status = GdipIsVisiblePathPoint(path, 9.0, 9.0, graphics, &result);
    expect(Ok, status);
    expect(TRUE, result);
    result = TRUE;
    status = GdipIsVisiblePathPoint(path, 11.0, 11.0, graphics, &result);
    expect(Ok, status);
    expect(FALSE, result);
    GdipResetWorldTransform(graphics);

    GdipDeletePath(path);
    GdipDeleteGraphics(graphics);
    ReleaseDC(0, hdc);
}

static void test_is_outline_visible_path_point(void)
{
    BOOL result;
    GpBitmap *bitmap;
    GpGraphics *graphics = NULL;
    GpPath *path;
    GpPen *pen = NULL;
    GpStatus status;
    static const int width = 20, height = 20;

    /* Graphics associated with an Image object.*/
    status = GdipCreateBitmapFromScan0(width, height, 0, PixelFormat32bppRGB, NULL, &bitmap);
    expect(Ok, status);
    status = GdipGetImageGraphicsContext((GpImage *)bitmap, &graphics);
    expect(Ok, status);
    ok(graphics != NULL, "Expected the graphics context to be initialized.\n");

    status = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, status);

    status = GdipAddPathRectangle(path, 2.0, 0.0, 13.0, 15.0);
    expect(Ok, status);

    status = GdipCreatePen1((ARGB)0xffff00ff, 3.0f, UnitPixel, &pen);
    expect(Ok, status);
    ok(pen != NULL, "Expected pen to be initialized\n");

    /* With NULL pen */
    result = 9;
    status = GdipIsOutlineVisiblePathPoint(path, 0.0, 1.0, NULL, graphics, &result);
    expect(InvalidParameter, status);
    expect(9, result);

    /* Without transformation */
    result = TRUE;
    status = GdipIsOutlineVisiblePathPoint(path, 0.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(FALSE, result);
    result = FALSE;
    status = GdipIsOutlineVisiblePathPoint(path, 1.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(TRUE, result);
    result = FALSE;
    status = GdipIsOutlineVisiblePathPoint(path, 10.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(TRUE, result);
    result = FALSE;
    status = GdipIsOutlineVisiblePathPoint(path, 16.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(TRUE, result);
    result = TRUE;
    status = GdipIsOutlineVisiblePathPoint(path, 17.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(FALSE, result);

    /* Translating */
    status = GdipTranslateWorldTransform(graphics, 50.0, 50.0, MatrixOrderPrepend);
    expect(Ok, status);
    result = FALSE;
    status = GdipIsOutlineVisiblePathPoint(path, 10.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(TRUE, result);
    result = FALSE;
    status = GdipIsOutlineVisiblePathPoint(path, 15.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(TRUE, result);
    result = FALSE;
    status = GdipIsOutlineVisiblePathPoint(path, 16.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(TRUE, result);

    /* Scaling */
    status = GdipScaleWorldTransform(graphics, 2.0, 2.0, MatrixOrderPrepend);
    expect(Ok, status);
    result = TRUE;
    status = GdipIsOutlineVisiblePathPoint(path, 0.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(FALSE, result);
    result = TRUE;
    status = GdipIsOutlineVisiblePathPoint(path, 1.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(FALSE, result);
    result = FALSE;
    status = GdipIsOutlineVisiblePathPoint(path, 2.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(TRUE, result);
    result = TRUE;
    status = GdipIsOutlineVisiblePathPoint(path, 3.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(FALSE, result);
    result = TRUE;
    status = GdipIsOutlineVisiblePathPoint(path, 14.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(FALSE, result);
    result = FALSE;
    status = GdipIsOutlineVisiblePathPoint(path, 15.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(TRUE, result);
    result = TRUE;
    status = GdipIsOutlineVisiblePathPoint(path, 16.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(FALSE, result);

    /* Page Unit */
    GdipResetWorldTransform(graphics);
    status = GdipSetPageUnit(graphics, UnitMillimeter);
    expect(Ok, status);
    result = TRUE;
    status = GdipIsOutlineVisiblePathPoint(path, 0.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(FALSE, result);
    result = TRUE;
    status = GdipIsOutlineVisiblePathPoint(path, 1.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(FALSE, result);
    result = FALSE;
    status = GdipIsOutlineVisiblePathPoint(path, 2.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(TRUE, result);
    result = TRUE;
    status = GdipIsOutlineVisiblePathPoint(path, 3.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(FALSE, result);
    result = TRUE;
    status = GdipIsOutlineVisiblePathPoint(path, 14.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(FALSE, result);
    result = FALSE;
    status = GdipIsOutlineVisiblePathPoint(path, 15.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(TRUE, result);
    result = TRUE;
    status = GdipIsOutlineVisiblePathPoint(path, 16.0, 1.0, pen, graphics, &result);
    expect(Ok, status);
    expect(FALSE, result);

    GdipResetWorldTransform(graphics);
    GdipDeletePath(path);
    GdipDeleteGraphics(graphics);
}

static void test_empty_rect(void)
{
    GpPath *path;
    GpStatus status;
    INT count;
    BOOL result;

    status = GdipCreatePath(FillModeAlternate, &path);
    expect(Ok, status);

    status = GdipAddPathRectangle(path, 0.0, 0.0, -5.0, 5.0);
    expect(Ok, status);

    status = GdipGetPointCount(path, &count);
    expect(Ok, status);
    expect(0, count);

    status = GdipIsVisiblePathPoint(path, -2.0, 2.0, NULL, &result);
    expect(Ok, status);
    expect(FALSE, result);

    status = GdipAddPathRectangle(path, 0.0, 0.0, 5.0, -5.0);
    expect(Ok, status);

    status = GdipGetPointCount(path, &count);
    expect(Ok, status);
    expect(0, count);

    status = GdipAddPathRectangle(path, 0.0, 0.0, 0.0, 5.0);
    expect(Ok, status);

    status = GdipGetPointCount(path, &count);
    expect(Ok, status);
    expect(0, count);

    status = GdipAddPathRectangle(path, 0.0, 0.0, 5.0, 0.0);
    expect(Ok, status);

    status = GdipGetPointCount(path, &count);
    expect(Ok, status);
    expect(0, count);

    status = GdipAddPathRectangle(path, 0.0, 0.0, 5.0, 0.1);
    expect(Ok, status);

    status = GdipGetPointCount(path, &count);
    expect(Ok, status);
    expect(4, count);

    GdipDeletePath(path);
}

static path_test_t rect_line_path[] = {
    {1.0, 4.0, PathPointTypeStart, 0, 0}, /*0*/
    {17.0, 4.0, PathPointTypeLine, 0, 0}, /*1*/
    {17.0, 68.0, PathPointTypeLine, 0, 0}, /*2*/
    {1.0, 68.0, PathPointTypeLine | PathPointTypeCloseSubpath, 0, 0}, /*3*/
    {1.0, 8.0, PathPointTypeStart, 0, 0}, /*4*/
    {17.0, 8.0, PathPointTypeLine, 0, 0} /*5*/
    };

static void test_rect_line(void)
{
    GpStatus status;
    GpPath* path;

    GdipCreatePath(FillModeAlternate, &path);

    status = GdipAddPathRectangleI(path, 1, 4, 16, 64);
    expect(Ok, status);

    status = GdipAddPathLineI(path, 1, 8, 17, 8);
    expect(Ok, status);

    ok_path(path, rect_line_path, ARRAY_SIZE(rect_line_path), FALSE);

    GdipDeletePath(path);
}

START_TEST(graphicspath)
{
    struct GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    HMODULE hmsvcrt;
    int (CDECL * _controlfp_s)(unsigned int *cur, unsigned int newval, unsigned int mask);

    /* Enable all FP exceptions except _EM_INEXACT, which gdi32 can trigger */
    hmsvcrt = LoadLibraryA("msvcrt");
    _controlfp_s = (void*)GetProcAddress(hmsvcrt, "_controlfp_s");
    if (_controlfp_s) _controlfp_s(0, 0, 0x0008001e);

    gdiplusStartupInput.GdiplusVersion              = 1;
    gdiplusStartupInput.DebugEventCallback          = NULL;
    gdiplusStartupInput.SuppressBackgroundThread    = 0;
    gdiplusStartupInput.SuppressExternalCodecs      = 0;

    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    test_constructor_destructor();
    test_getpathdata();
    test_createpath2();
    test_line2();
    test_bezier();
    test_beziers();
    test_arc();
    test_worldbounds();
    test_pathpath();
    test_ellipse();
    test_linei();
    test_rect();
    test_polygon();
    test_lastpoint();
    test_addcurve();
    test_addclosedcurve();
    test_reverse();
    test_addpie();
    test_flatten();
    test_widen();
    test_widen_cap();
    test_isvisible();
    test_is_outline_visible_path_point();
    test_empty_rect();
    test_rect_line();

    GdiplusShutdown(gdiplusToken);
}
