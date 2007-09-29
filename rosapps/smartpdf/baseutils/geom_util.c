/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#include "base_util.h"
#include "geom_util.h"

/* Return true if 'r1' and 'r2' intersect. Put the intersect area into
   'rIntersectOut'.
   Return false if there is no intersection. */
int RectI_Intersect(RectI *r1, RectI *r2, RectI *rIntersectOut)
{
    int     x1s, x1e, x2s, x2e;
    int     y1s, y1e, y2s, y2e;

    int     xIntersectS, xIntersectE;
    int     yIntersectS, yIntersectE;

    assert(r1 && r2 && rIntersectOut);
    if (!r1 || !r2 || !rIntersectOut)
        return 0;

    x1s = r1->x;
    x2s = r2->x;
    x1e = x1s + r1->dx;
    x2e = x2s + r2->dx;

    /* { } visualizes r1 and | | visualizes r2 */

    /* problem is symmetric, so reduce the number of different cases by
       consistent ordering where r1 is always before r2 in axis-x */
    if (x2s < x1s) {
        swap_int(&x1s, &x2s);
        swap_int(&x1e, &x2e);
    }

    /* case of non-overlapping rectangles i.e.:
       { }   | | */
    if (x2s > x1e)
        return 0;

    /* partially overlapped i.e.:
       {  |  } |
       and one inside the other i.e.:
       {  | |  } */

    assert(x2s >= x1s);
    assert(x2s <= x1e);
    xIntersectS = x2s;
    xIntersectE = MinInt(x1e, x2e);
    assert(xIntersectE >= xIntersectS);

    /* the logic for y is the same */
    y1s = r1->y;
    y2s = r2->y;
    y1e = y1s + r1->dy;
    y2e = y2s + r2->dy;
    if (y2s < y1s) {
        swap_int(&y1s, &y2s);
        swap_int(&y1e, &y2e);
    }
    if (y2s > y1e)
        return 0;
    assert(y2s >= y1s);
    assert(y2s <= y1e);
    yIntersectS = y2s;
    yIntersectE = MinInt(y1e, y2e);

    rIntersectOut->x = xIntersectS;
    rIntersectOut->y = yIntersectS;
    assert(xIntersectE >= xIntersectS);
    assert(yIntersectE >= yIntersectS);
    rIntersectOut->dx = xIntersectE - xIntersectS;
    rIntersectOut->dy = yIntersectE - yIntersectS;
    return 1;
}

void RectI_FromXY(RectI *rOut, int xs, int xe, int ys, int ye)
{
    assert(rOut);
    if (!rOut)
        return;
    assert(xs <= xe);
    assert(ys <= ye);
    rOut->x = xs;
    rOut->y = ys;
    rOut->dx = xe - xs;
    rOut->dy = ye - ys;
}

void RectD_FromRectI(RectD *rOut, RectI *rIn)
{
    rOut->x = (double)rIn->x;
    rOut->y = (double)rIn->y;
    rOut->dx = (double)rIn->dx;
    rOut->dy = (double)rIn->dy;
}

int intFromDouble (double d) {
    double i1 = (int) d;
    double i2 = i1 + 1;

    if (d - i1 < i2 - d)
        return i1;
    else
        return i2;
}

void RectI_FromRectD(RectI *rOut, RectD *rIn)
{
    rOut->x = intFromDouble(rIn->x);
    rOut->y = intFromDouble(rIn->y);
    rOut->dx = intFromDouble(rIn->dx);
    rOut->dy = intFromDouble(rIn->dy);
}

void RectD_Copy(RectD *rOut, RectD *rIn) {
    rOut->x = (double)rIn->x;
    rOut->y = (double)rIn->y;
    rOut->dx = (double)rIn->dx;
    rOut->dy = (double)rIn->dy;
}

void RectD_FromXY(RectD *rOut, double xs, double xe, double ys, double ye)
{
    assert(rOut);
    if (!rOut)
        return;
    if (xs > xe)
        swap_double(&xs, &xe);
    if (ys > ye)
        swap_double(&ys, &ye);

    rOut->x = xs;
    rOut->y = ys;
    rOut->dx = xe - xs;
    assert(rOut->dx >= 0.0);
    rOut->dy = ye - ys;
    assert(rOut->dy >= 0.0);
}

/* Return TRUE if point 'x'/'y' is inside rectangle 'r' */
int RectI_Inside(RectI *r, int x, int y)
{
    if (x < r->x)
        return FALSE;
    if (x > r->x + r->dx)
        return FALSE;
    if (y < r->y)
        return FALSE;
    if (y > r->y + r->dy)
        return FALSE;
    return TRUE;
}

#ifndef NDEBUG
void RectI_AssertEqual(RectI *rIntersect, RectI *rExpected)
{
    assert(rIntersect->x == rExpected->x);
    assert(rIntersect->y == rExpected->y);
    assert(rIntersect->dx == rExpected->dx);
    assert(rIntersect->dy == rExpected->dy);
}

void u_RectI_Intersect(void)
{
    int         i, dataLen;
    RectI       r1, r2, rIntersect, rExpected, rExpectedSwaped;
    int         doIntersect, doIntersectExpected;

    struct SRIData {
        int     x1s, x1e, y1s, y1e;
        int     x2s, x2e, y2s, y2e;
        int     intersect;
        int     i_xs, i_xe, i_ys, i_ye;
    } testData[] = {
        { 0,10, 0,10,   0,10, 0,10,  1,  0,10, 0,10 }, /* complete intersect */
        { 0,10, 0,10,  20,30,20,30,  0,  0, 0, 0, 0 }, /* no intersect */
        { 0,10, 0,10,   5,15, 0,10,  1,  5,10, 0,10 }, /* { | } | */
        { 0,10, 0,10,   5, 7, 0,10,  1,  5, 7, 0,10 }, /* { | | } */

        { 0,10, 0,10,   5, 7, 5, 7,  1,  5, 7, 5, 7 },
        { 0,10, 0,10,   5, 15,5,15,  1,  5,10, 5,10 },
    };
    dataLen = dimof(testData);
    for (i = 0; i < dataLen; i++) {
        struct SRIData *curr;
        curr = &(testData[i]);
        RectI_FromXY(&rExpected, curr->i_xs, curr->i_xe, curr->i_ys, curr->i_ye);
        RectI_FromXY(&rExpectedSwaped, curr->i_ys, curr->i_ye, curr->i_xs, curr->i_xe);

        RectI_FromXY(&r1, curr->x1s, curr->x1e, curr->y1s, curr->y1e);
        RectI_FromXY(&r2, curr->x2s, curr->x2e, curr->y2s, curr->y2e);
        doIntersectExpected = curr->intersect;

        doIntersect = RectI_Intersect(&r1, &r2, &rIntersect);
        assert(doIntersect == doIntersectExpected);
        if (doIntersect)
            RectI_AssertEqual(&rIntersect, &rExpected);

        /* if we swap rectangles, the results should be the same */
        RectI_FromXY(&r2, curr->x1s, curr->x1e, curr->y1s, curr->y1e);
        RectI_FromXY(&r1, curr->x2s, curr->x2e, curr->y2s, curr->y2e);
        doIntersect = RectI_Intersect(&r1, &r2, &rIntersect);
        assert(doIntersect == doIntersectExpected);
        if (doIntersect)
            RectI_AssertEqual(&rIntersect, &rExpected);

        /* if we swap x with y coordinates in a rectangle, results should be the same */
        RectI_FromXY(&r1, curr->y1s, curr->y1e, curr->x1s, curr->x1e);
        RectI_FromXY(&r2, curr->y2s, curr->y2e, curr->x2s, curr->x2e);
        doIntersect = RectI_Intersect(&r1, &r2, &rIntersect);
        assert(doIntersect == doIntersectExpected);
        if (doIntersect)
            RectI_AssertEqual(&rIntersect, &rExpectedSwaped);

        /* swap both rectangles and x with y, results should be the same */
        RectI_FromXY(&r2, curr->y1s, curr->y1e, curr->x1s, curr->x1e);
        RectI_FromXY(&r1, curr->y2s, curr->y2e, curr->x2s, curr->x2e);
        doIntersect = RectI_Intersect(&r1, &r2, &rIntersect);
        assert(doIntersect == doIntersectExpected);
        if (doIntersect)
            RectI_AssertEqual(&rIntersect, &rExpectedSwaped);
    }
}
#endif

