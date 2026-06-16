/*
 * Unit test suite for pathiterator
 *
 * Copyright (C) 2008 Nikolay Sivov
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

#define expect(expected, got) ok(got == expected, "Expected %.8x, got %.8x\n", expected, got)

static void test_constructor_destructor(void)
{
    GpPath *path;
    GpPathIterator *iter;
    GpStatus stat;

    GdipCreatePath(FillModeAlternate, &path);
    GdipAddPathRectangle(path, 5.0, 5.0, 100.0, 50.0);

    /* NULL args */
    stat = GdipCreatePathIter(NULL, NULL);
    expect(InvalidParameter, stat);
    iter = NULL;
    stat = GdipCreatePathIter(&iter, NULL);
    expect(Ok, stat);
    ok(iter != NULL, "Expected iterator to be created\n");
    GdipDeletePathIter(iter);
    stat = GdipCreatePathIter(NULL, path);
    expect(InvalidParameter, stat);
    stat = GdipDeletePathIter(NULL);
    expect(InvalidParameter, stat);

    /* valid args */
    stat = GdipCreatePathIter(&iter, path);
    expect(Ok, stat);

    GdipDeletePathIter(iter);
    GdipDeletePath(path);
}

static void test_hascurve(void)
{
    GpPath *path;
    GpPathIterator *iter;
    GpStatus stat;
    BOOL hasCurve;

    GdipCreatePath(FillModeAlternate, &path);
    GdipAddPathRectangle(path, 5.0, 5.0, 100.0, 50.0);

    stat = GdipCreatePathIter(&iter, path);
    expect(Ok, stat);

    /* NULL args
       BOOL out argument is local in wrapper class method,
       so it always has not-NULL address */
    stat = GdipPathIterHasCurve(NULL, &hasCurve);
    expect(InvalidParameter, stat);

    /* valid args */
    stat = GdipPathIterHasCurve(iter, &hasCurve);
    expect(Ok, stat);
    expect(FALSE, hasCurve);

    GdipDeletePathIter(iter);

    stat = GdipAddPathEllipse(path, 0.0, 0.0, 35.0, 70.0);
    expect(Ok, stat);

    stat = GdipCreatePathIter(&iter, path);
    expect(Ok, stat);

    stat = GdipPathIterHasCurve(iter, &hasCurve);
    expect(Ok, stat);
    expect(TRUE, hasCurve);

    GdipDeletePathIter(iter);
    GdipDeletePath(path);
}

static void test_nextmarker(void)
{
    GpPath *path;
    GpPathIterator *iter;
    GpStatus stat;
    INT start, end;
    INT result;

    /* NULL args
       BOOL out argument is local in wrapper class method,
       so it always has not-NULL address */
    stat = GdipPathIterNextMarker(NULL, &result, NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipPathIterNextMarker(NULL, &result, &start, NULL);
    expect(InvalidParameter, stat);
    stat = GdipPathIterNextMarker(NULL, &result, NULL, &end);
    expect(InvalidParameter, stat);

    GdipCreatePath(FillModeAlternate, &path);
    GdipAddPathRectangle(path, 5.0, 5.0, 100.0, 50.0);

    /* no markers */
    GdipCreatePathIter(&iter, path);
    start = end = result = (INT)0xdeadbeef;
    stat = GdipPathIterNextMarker(iter, &result, &start, &end);
    expect(Ok, stat);
    expect(0, start);
    expect(3, end);
    expect(4, result);
    start = end = result = (INT)0xdeadbeef;
    stat = GdipPathIterNextMarker(iter, &result, &start, &end);
    expect(Ok, stat);
    /* start/end remain unchanged */
    expect((INT)0xdeadbeef, start);
    expect((INT)0xdeadbeef, end);
    expect(0, result);
    GdipDeletePathIter(iter);

    /* one marker */
    GdipSetPathMarker(path);
    GdipCreatePathIter(&iter, path);
    start = end = result = (INT)0xdeadbeef;
    stat = GdipPathIterNextMarker(iter, &result, &start, &end);
    expect(Ok, stat);
    expect(0, start);
    expect(3, end);
    expect(4, result);
    start = end = result = (INT)0xdeadbeef;
    stat = GdipPathIterNextMarker(iter, &result, &start, &end);
    expect(Ok, stat);
    expect((INT)0xdeadbeef, start);
    expect((INT)0xdeadbeef, end);
    expect(0, result);
    GdipDeletePathIter(iter);

    /* two markers */
    GdipAddPathLine(path, 0.0, 0.0, 10.0, 30.0);
    GdipSetPathMarker(path);
    GdipCreatePathIter(&iter, path);
    start = end = result = (INT)0xdeadbeef;
    stat = GdipPathIterNextMarker(iter, &result, &start, &end);
    expect(Ok, stat);
    expect(0, start);
    expect(3, end);
    expect(4, result);
    start = end = result = (INT)0xdeadbeef;
    stat = GdipPathIterNextMarker(iter, &result, &start, &end);
    expect(Ok, stat);
    expect(4, start);
    expect(5, end);
    expect(2, result);
    start = end = result = (INT)0xdeadbeef;
    stat = GdipPathIterNextMarker(iter, &result, &start, &end);
    expect(Ok, stat);
    expect((INT)0xdeadbeef, start);
    expect((INT)0xdeadbeef, end);
    expect(0, result);
    GdipDeletePathIter(iter);

    GdipDeletePath(path);
}

static void test_nextmarkerpath(void)
{
    GpPath *path, *retpath;
    GpPathIterator *iter;
    GpStatus stat;
    INT result, count;

    GdipCreatePath(FillModeAlternate, &path);

    /* NULL */
    stat = GdipPathIterNextMarkerPath(NULL, NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipPathIterNextMarkerPath(NULL, &result, NULL);
    expect(InvalidParameter, stat);
    stat = GdipPathIterNextMarkerPath(NULL, &result, path);
    expect(InvalidParameter, stat);

    GdipAddPathRectangle(path, 5.0, 5.0, 100.0, 50.0);

    /* no markers */
    GdipCreatePath(FillModeAlternate, &retpath);
    GdipCreatePathIter(&iter, path);
    result = -1;
    stat = GdipPathIterNextMarkerPath(iter, &result, retpath);
    expect(Ok, stat);
    expect(4, result);
    count = -1;
    GdipGetPointCount(retpath, &count);
    expect(4, count);
    result = -1;
    stat = GdipPathIterNextMarkerPath(iter, &result, retpath);
    expect(Ok, stat);
    expect(0, result);
    count = -1;
    GdipGetPointCount(retpath, &count);
    expect(4, count);
    GdipDeletePathIter(iter);
    GdipDeletePath(retpath);

    /* one marker */
    GdipSetPathMarker(path);
    GdipCreatePath(FillModeAlternate, &retpath);
    GdipCreatePathIter(&iter, path);
    result = -1;
    stat = GdipPathIterNextMarkerPath(iter, &result, retpath);
    expect(Ok, stat);
    expect(4, result);
    count = -1;
    GdipGetPointCount(retpath, &count);
    expect(4, count);
    result = -1;
    stat = GdipPathIterNextMarkerPath(iter, &result, retpath);
    expect(Ok, stat);
    expect(0, result);
    count = -1;
    GdipGetPointCount(retpath, &count);
    expect(4, count);
    GdipDeletePathIter(iter);
    GdipDeletePath(retpath);

    /* two markers */
    GdipAddPathLine(path, 0.0, 0.0, 10.0, 30.0);
    GdipSetPathMarker(path);
    GdipCreatePath(FillModeAlternate, &retpath);
    GdipCreatePathIter(&iter, path);
    result = -1;
    stat = GdipPathIterNextMarkerPath(iter, &result, retpath);
    expect(Ok, stat);
    expect(4, result);
    result = -1;
    stat = GdipPathIterNextMarkerPath(iter, &result, retpath);
    expect(Ok, stat);
    expect(2, result);
    result = -1;
    stat = GdipPathIterNextMarkerPath(iter, &result, retpath);
    expect(Ok, stat);
    expect(0, result);
    GdipDeletePathIter(iter);
    GdipDeletePath(retpath);

    GdipDeletePath(path);
}

static void test_getsubpathcount(void)
{
    GpPath *path;
    GpPathIterator *iter;
    GpStatus stat;
    INT count;

    /* NULL args */
    stat = GdipPathIterGetSubpathCount(NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipPathIterGetSubpathCount(NULL, &count);
    expect(InvalidParameter, stat);

    GdipCreatePath(FillModeAlternate, &path);

    /* empty path */
    GdipCreatePathIter(&iter, path);
    stat = GdipPathIterGetSubpathCount(iter, &count);
    expect(Ok, stat);
    expect(0, count);
    GdipDeletePathIter(iter);

    GdipAddPathLine(path, 5.0, 5.0, 100.0, 50.0);

    /* open figure */
    GdipCreatePathIter(&iter, path);
    stat = GdipPathIterGetSubpathCount(iter, &count);
    expect(Ok, stat);
    expect(1, count);
    GdipDeletePathIter(iter);

    /* manually start new figure */
    GdipStartPathFigure(path);
    GdipAddPathLine(path, 50.0, 50.0, 110.0, 40.0);
    GdipCreatePathIter(&iter, path);
    stat = GdipPathIterGetSubpathCount(iter, &count);
    expect(Ok, stat);
    expect(2, count);
    GdipDeletePathIter(iter);

    GdipDeletePath(path);
}

static void test_isvalid(void)
{
    GpPath *path;
    GpPathIterator *iter;
    GpStatus stat;
    BOOL isvalid;
    INT start, end, result;

    GdipCreatePath(FillModeAlternate, &path);

    /* NULL args */
    GdipCreatePathIter(&iter, path);
    stat = GdipPathIterIsValid(NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipPathIterIsValid(iter, NULL);
    expect(InvalidParameter, stat);
    stat = GdipPathIterIsValid(NULL, &isvalid);
    expect(InvalidParameter, stat);
    GdipDeletePathIter(iter);

    /* on empty path */
    GdipCreatePathIter(&iter, path);
    isvalid = FALSE;
    stat = GdipPathIterIsValid(iter, &isvalid);
    expect(Ok, stat);
    expect(TRUE, isvalid);
    GdipDeletePathIter(iter);

    /* no markers */
    stat = GdipAddPathLine(path, 50.0, 50.0, 110.0, 40.0);
    expect(Ok, stat);
    stat = GdipCreatePathIter(&iter, path);
    expect(Ok, stat);
    stat = GdipPathIterNextMarker(iter, &result, &start, &end);
    expect(Ok, stat);
    isvalid = FALSE;
    stat = GdipPathIterIsValid(iter, &isvalid);
    expect(Ok, stat);
    expect(TRUE, isvalid);
    GdipDeletePathIter(iter);

    GdipDeletePath(path);
}

static void test_nextsubpathpath(void)
{
    GpPath *path, *retpath;
    GpPathIterator *iter;
    GpStatus stat;
    BOOL closed;
    INT count, result;

    GdipCreatePath(FillModeAlternate, &path);

    /* NULL args */
    GdipCreatePath(FillModeAlternate, &retpath);
    GdipCreatePathIter(&iter, path);
    stat = GdipPathIterNextSubpathPath(NULL, NULL, NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipPathIterNextSubpathPath(iter, NULL, NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipPathIterNextSubpathPath(NULL, &result, NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipPathIterNextSubpathPath(iter, &result, NULL, &closed);
    expect(Ok, stat);
    stat = GdipPathIterNextSubpathPath(iter, NULL, NULL, &closed);
    expect(InvalidParameter, stat);
    stat = GdipPathIterNextSubpathPath(iter, NULL, retpath, NULL);
    expect(InvalidParameter, stat);
    stat = GdipPathIterNextSubpathPath(iter, &result, retpath, NULL);
    expect(InvalidParameter, stat);
    GdipDeletePathIter(iter);
    GdipDeletePath(retpath);

    /* empty path */
    GdipCreatePath(FillModeAlternate, &retpath);
    GdipCreatePathIter(&iter, path);
    result = -2;
    closed = TRUE;
    stat = GdipPathIterNextSubpathPath(iter, &result, retpath, &closed);
    expect(Ok, stat);
    expect(0, result);
    expect(TRUE, closed);
    count = -1;
    GdipGetPointCount(retpath, &count);
    expect(0, count);
    GdipDeletePathIter(iter);
    GdipDeletePath(retpath);

    /* open figure */
    GdipAddPathLine(path, 5.0, 5.0, 100.0, 50.0);

    GdipCreatePath(FillModeAlternate, &retpath);
    GdipCreatePathIter(&iter, path);
    result = -2;
    closed = TRUE;
    stat = GdipPathIterNextSubpathPath(iter, &result, retpath, &closed);
    expect(Ok, stat);
    expect(2, result);
    expect(FALSE, closed);
    count = -1;
    GdipGetPointCount(retpath, &count);
    expect(2, count);
    /* subsequent call */
    result = -2;
    closed = TRUE;
    stat = GdipPathIterNextSubpathPath(iter, &result, retpath, &closed);
    expect(Ok, stat);
    expect(0, result);
    expect(TRUE, closed);
    count = -1;
    GdipGetPointCount(retpath, &count);
    expect(2, count);
    GdipDeletePathIter(iter);

    /* closed figure, check does it extend retpath or reset it */
    GdipAddPathLine(retpath, 50.0, 55.0, 200.0, 150.0);

    GdipClosePathFigure(path);
    GdipAddPathLine(path, 50.0, 55.0, 200.0, 150.0);
    GdipClosePathFigure(path);

    GdipCreatePathIter(&iter, path);
    result = -2;
    closed = FALSE;
    stat = GdipPathIterNextSubpathPath(iter, &result, retpath, &closed);
    expect(Ok, stat);
    expect(2, result);
    expect(TRUE, closed);
    count = -1;
    GdipGetPointCount(retpath, &count);
    expect(2, count);
    /* subsequent call */
    result = -2;
    closed = FALSE;
    stat = GdipPathIterNextSubpathPath(iter, &result, retpath, &closed);
    expect(Ok, stat);
    expect(2, result);
    expect(TRUE, closed);
    count = -1;
    GdipGetPointCount(retpath, &count);
    expect(2, count);
    result = -2;
    closed = FALSE;
    stat = GdipPathIterNextSubpathPath(iter, &result, retpath, &closed);
    expect(Ok, stat);
    expect(0, result);
    expect(TRUE, closed);
    count = -1;
    GdipGetPointCount(retpath, &count);
    expect(2, count);
    GdipDeletePathIter(iter);

    GdipDeletePath(retpath);
    GdipDeletePath(path);
}

static void test_nextsubpath(void)
{
    GpPath *path;
    GpPathIterator *iter;
    GpStatus stat;
    INT result, start, end;
    BOOL closed;

    /* empty path */
    GdipCreatePath(FillModeAlternate, &path);
    GdipCreatePathIter(&iter, path);

    result = start = end = closed = (INT)0xdeadbeef;
    stat = GdipPathIterNextSubpath(iter, &result, &start, &end, &closed);
    expect(Ok, stat);
    expect(0, result);
    expect((INT)0xdeadbeef, start);
    expect((INT)0xdeadbeef, end);
    expect((INT)0xdeadbeef, closed);

    /* four points figure */
    GdipAddPathLine(path, 0.0, 0.0, 10.0, 30.0);
    GdipAddPathLine(path, -10.0, 5.0, 15.0, 8.0);
    GdipCreatePathIter(&iter, path);
    stat = GdipPathIterNextSubpath(iter, &result, &start, &end, &closed);
    expect(Ok, stat);
    expect(4, result);
    expect(0, start);
    expect(3, end);
    expect(FALSE, closed);

    /* No more subpaths*/
    stat = GdipPathIterNextSubpath(iter, &result, &start, &end, &closed);
    expect(Ok, stat);
    expect(0, result);
    expect(0, start);
    expect(0, end);
    expect(TRUE, closed);

    GdipDeletePathIter(iter);
    GdipDeletePath(path);
}

static void test_nextpathtype(void)
{
    GpPath *path;
    GpPathIterator *iter;
    GpStatus stat;
    INT count, result, start, end;
    BYTE type;
    BOOL closed;
    GpPointF point;

    GdipCreatePath(FillModeAlternate, &path);
    GdipCreatePathIter(&iter, path);

    /* NULL arguments */
    stat = GdipPathIterNextPathType(NULL, NULL, NULL, NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipPathIterNextPathType(iter, NULL, NULL, NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipPathIterNextPathType(iter, &result, NULL, NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipPathIterNextPathType(iter, NULL, &type, NULL, NULL);
    expect(InvalidParameter, stat);
    stat = GdipPathIterNextPathType(iter, NULL, NULL, &start, &end);
    expect(InvalidParameter, stat);
    stat = GdipPathIterNextPathType(iter, NULL, &type, &start, &end);
    expect(InvalidParameter, stat);
    stat = GdipPathIterNextPathType(iter, &result, &type, NULL, NULL);
    expect(InvalidParameter, stat);

    /* empty path */
    result = start = end = (INT)0xdeadbeef;
    type = 255; /* out of range */
    stat = GdipPathIterNextPathType(iter, &result, &type, &start, &end);
    expect(Ok, stat);
    expect(0, result);
    expect(255, type);
    expect((INT)0xdeadbeef, start);
    expect((INT)0xdeadbeef, end);
    GdipDeletePathIter(iter);


    /* Single PathPointTypeStart point */
    point.X = 1.0;
    point.Y = 2.0;
    stat = GdipAddPathLine2(path, &point, 1);
    expect(Ok, stat);
    stat = GdipCreatePathIter(&iter, path);
    expect(Ok, stat);

    result = start = end = closed = (INT)0xdeadbeef;
    stat = GdipPathIterNextSubpath(iter, &result, &start, &end, &closed);
    expect(Ok, stat);
    expect(1, result);
    expect(0, start);
    expect(0, end);
    expect(FALSE, closed);

    result = start = end = (INT)0xdeadbeef;
    type = 255; /* out of range */
    stat = GdipPathIterNextPathType(iter, &result, &type, &start, &end);
    expect(Ok, stat);
    expect(1, result);
    expect(255, type);
    expect(0, start);
    expect(0, end);

    GdipDeletePathIter(iter);
    GdipDeletePath(path);


    /* Figure with three subpaths */
    GdipCreatePath(FillModeAlternate, &path);
    stat = GdipAddPathLine(path, 0.0, 0.0, 10.0, 30.0);
    expect(Ok, stat);
    stat = GdipAddPathRectangle(path, 1.0, 2.0, 3.0, 4.0);
    expect(Ok, stat);
    stat = GdipAddPathEllipse(path, 0.0, 0.0, 35.0, 70.0);
    expect(Ok, stat);
    stat = GdipCreatePathIter(&iter, path);
    expect(Ok, stat);

    /* When subPath is not set,
       it is not possible to get Path Type */
    result = start = end = (INT)0xdeadbeef;
    type = 255; /* out of range */
    stat = GdipPathIterNextPathType(iter, &result, &type, &start, &end);
    expect(Ok, stat);
    expect(0, result);
    expect(255, type);
    expect((INT)0xdeadbeef, start);
    expect((INT)0xdeadbeef, end);

    /* When subPath is not set,
       it is possible to get number of path points */
    stat = GdipPathIterGetCount(iter, &count);
    expect(Ok, stat);
    expect(19, count);

    /* Set subPath (Line) */
    result = start = end = closed = (INT)0xdeadbeef;
    stat = GdipPathIterNextSubpath(iter, &result, &start, &end, &closed);
    expect(Ok, stat);
    expect(2, result);
    expect(0, start);
    expect(1, end);
    expect(FALSE, closed);

    /* When subPath is set (Line), return position of points with the same type */
    result = start = end = (INT)0xdeadbeef;
    type = 255; /* out of range */
    stat = GdipPathIterNextPathType(iter, &result, &type, &start, &end);
    expect(Ok, stat);
    expect(2, result);
    expect(PathPointTypeLine, type);
    expect(0, start);
    expect(1, end);

    /* When NextSubpath is running twice, without invocation NextPathType
       (skipping Rectangle figure), make sure that PathType index is updated */
    result = start = end = closed = (INT)0xdeadbeef;
    stat = GdipPathIterNextSubpath(iter, &result, &start, &end, &closed);
    expect(Ok, stat);
    expect(4, result);
    expect(2, start);
    expect(5, end);
    expect(TRUE, closed);

    /* Increment subPath (to Ellipse figure) */
    result = start = end = closed = (INT)0xdeadbeef;
    stat = GdipPathIterNextSubpath(iter, &result, &start, &end, &closed);
    expect(Ok, stat);
    expect(13, result);
    expect(6, start);
    expect(18, end);
    expect(TRUE, closed);

    /* When subPath is set to Ellipse figure, the type is PathPointTypeBezier */
    result = start = end = (INT)0xdeadbeef;
    type = 255; /* out of range */
    stat = GdipPathIterNextPathType(iter, &result, &type, &start, &end);
    expect(Ok, stat);
    expect(13, result);
    expect(PathPointTypeBezier, type);
    expect(6, start);
    expect(18, end);

    /* As there is no more subpaths, when subPath is incremented,
       it returns zeros */
    result = start = end = closed = (INT)0xdeadbeef;
    stat = GdipPathIterNextSubpath(iter, &result, &start, &end, &closed);
    expect(Ok, stat);
    expect(0, result);
    expect(0, start);
    expect(0, end);
    expect(TRUE, closed);

    /* When no more elements, after invoking NextPathType
       the output numbers are not changed */
    start = end = result = (INT)0xdeadbeef;
    type = 255; /* out of range */
    stat = GdipPathIterNextPathType(iter, &result, &type, &start, &end);
    expect(Ok, stat);
    expect(0, result);
    expect(255, type);
    expect((INT)0xdeadbeef, start);
    expect((INT)0xdeadbeef, end);

    GdipDeletePathIter(iter);
    GdipDeletePath(path);


    /* Single PathPointTypeStart point */
    GdipCreatePath(FillModeAlternate, &path);
    point.X = 1.0;
    point.Y = 2.0;
    stat = GdipAddPathLine2(path, &point, 1);
    expect(Ok, stat);
    stat = GdipCreatePathIter(&iter, path);
    expect(Ok, stat);

    result = start = end = closed = (INT)0xdeadbeef;
    stat = GdipPathIterNextSubpath(iter, &result, &start, &end, &closed);
    expect(Ok, stat);
    expect(1, result);
    expect(0, start);
    expect(0, end);
    expect(FALSE, closed);

    result = start = end = (INT)0xdeadbeef;
    type = 255; /* out of range */
    stat = GdipPathIterNextPathType(iter, &result, &type, &start, &end);
    expect(Ok, stat);
    expect(1, result);
    expect(255, type);
    expect(0, start);
    expect(0, end);

    GdipDeletePathIter(iter);
    GdipDeletePath(path);


    /* Mixed PathPointTypeLine and PathPointTypeBezier points */
    GdipCreatePath(FillModeAlternate, &path);
    stat = GdipAddPathLine(path, -1.0, 1.0, 10.0, 15.0);
    expect(Ok, stat);
    /* Starting point of Bezier figure, is the same
       as ending point of previous figure (10.0, 15.0) */
    stat = GdipAddPathBezier(path, 10.0, 15.0, 9.0, 9.0, 6.0, 7.0, 9.0, 0.0);
    expect(Ok, stat);
    /* Starting point of figure, is different that ending point of previous figure */
    stat = GdipAddPathLine(path, -1.0, -1.0, 7.5, 12.0);
    expect(Ok, stat);
    stat = GdipStartPathFigure(path);
    expect(Ok, stat);
    stat = GdipAddPathLine(path, 50.0, 50.0, 110.0, 40.0);
    expect(Ok, stat);
    stat = GdipCreatePathIter(&iter, path);
    expect(Ok, stat);

    result = start = end = closed = (INT)0xdeadbeef;
    stat = GdipPathIterNextSubpath(iter, &result, &start, &end, &closed);
    expect(Ok, stat);
    expect(7, result);
    expect(0, start);
    expect(6, end);
    expect(FALSE, closed);

    result = start = end = (INT)0xdeadbeef;
    type = 255; /* out of range */
    stat = GdipPathIterNextPathType(iter, &result, &type, &start, &end);
    expect(Ok, stat);
    expect(2, result);
    expect(PathPointTypeLine, type);
    expect(0, start);
    expect(1, end);

    result = start = end = (INT)0xdeadbeef;
    type = 255; /* out of range */
    stat = GdipPathIterNextPathType(iter, &result, &type, &start, &end);
    expect(Ok, stat);
    expect(4, result);
    expect(PathPointTypeBezier, type);
    expect(1, start);
    expect(4, end);

    result = start = end = (INT)0xdeadbeef;
    type = 255; /* out of range */
    stat = GdipPathIterNextPathType(iter, &result, &type, &start, &end);
    expect(Ok, stat);
    expect(3, result);
    expect(PathPointTypeLine, type);
    expect(4, start);
    expect(6, end);

    result = start = end = (INT)0xdeadbeef;
    type = 255; /* out of range */
    stat = GdipPathIterNextPathType(iter, &result, &type, &start, &end);
    expect(Ok, stat);
    expect(0, result);
    expect(255, type);
    expect((INT)0xdeadbeef, start);
    expect((INT)0xdeadbeef, end);

    result = start = end = closed = (INT)0xdeadbeef;
    stat = GdipPathIterNextSubpath(iter, &result, &start, &end, &closed);
    expect(Ok, stat);
    expect(2, result);
    expect(7, start);
    expect(8, end);
    expect(FALSE, closed);

    result = start = end = (INT)0xdeadbeef;
    type = 255; /* out of range */
    stat = GdipPathIterNextPathType(iter, &result, &type, &start, &end);
    expect(Ok, stat);
    expect(2, result);
    expect(PathPointTypeLine, type);
    expect(7, start);
    expect(8, end);

    GdipDeletePathIter(iter);
    GdipDeletePath(path);
}

START_TEST(pathiterator)
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
    test_hascurve();
    test_nextmarker();
    test_nextmarkerpath();
    test_getsubpathcount();
    test_isvalid();
    test_nextsubpathpath();
    test_nextsubpath();
    test_nextpathtype();

    GdiplusShutdown(gdiplusToken);
}
