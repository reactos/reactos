/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for ...
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <apitest.h>
#include <windows.h>

typedef struct _RECT_TEST
{
    RECT rcSrc1;
    RECT rcSrc2;
    struct
    {
        INT iComplexity;
        RECT rcDstBounds;
    } res[4];
} RECT_TEST, *PRECT_TEST;

#define SetRectRgnIndirect(hrgn, prect) \
    SetRectRgn(hrgn, (prect)->left, (prect)->top, (prect)->right, (prect)->bottom)

PSTR apszRgnOp[6] = { "invalid", "RGN_AND", "RGN_OR", "RGN_XOR", "RGN_DIFF", "RGN_COPY" };

void Test_RectRegions()
{
    RECT_TEST aRectTests[] = {

    /* rcSrc1    rcSrc2           RGN_AND                 RGN_OR                 RGN_XOR                    RGN_DIFF  */
    {{0,0,0,0},{0,0,0,0}, {{NULLREGION,   {0,0,0,0}}, {NULLREGION,   {0,0,0,0}}, {NULLREGION,   {0,0,0,0}}, {NULLREGION,   {0,0,0,0}}}},
    {{0,0,1,1},{0,0,0,0}, {{NULLREGION,   {0,0,0,0}}, {SIMPLEREGION, {0,0,1,1}}, {SIMPLEREGION, {0,0,1,1}}, {SIMPLEREGION, {0,0,1,1}}}},
    {{0,0,0,0},{0,0,1,1}, {{NULLREGION,   {0,0,0,0}}, {SIMPLEREGION, {0,0,1,1}}, {SIMPLEREGION, {0,0,1,1}}, {NULLREGION,   {0,0,0,0}}}},

    /* Equal rgn */
    {{0,0,4,4},{0,0,4,4}, {{SIMPLEREGION, {0,0,4,4}}, {SIMPLEREGION, {0,0,4,4}}, {NULLREGION,   {0,0,0,0}}, {NULLREGION,   {0,0,0,0}}}},

    /* rgn 2 is within rgn 1 */
    {{0,0,4,4},{0,0,2,4}, {{SIMPLEREGION, {0,0,2,4}}, {SIMPLEREGION, {0,0,4,4}}, {SIMPLEREGION ,{2,0,4,4}}, {SIMPLEREGION, {2,0,4,4}}}}, // left
    {{0,0,4,4},{0,0,4,2}, {{SIMPLEREGION, {0,0,4,2}}, {SIMPLEREGION, {0,0,4,4}}, {SIMPLEREGION ,{0,2,4,4}}, {SIMPLEREGION, {0,2,4,4}}}}, // top
    {{0,0,4,4},{2,0,4,4}, {{SIMPLEREGION, {2,0,4,4}}, {SIMPLEREGION, {0,0,4,4}}, {SIMPLEREGION ,{0,0,2,4}}, {SIMPLEREGION, {0,0,2,4}}}}, // right
    {{0,0,4,4},{0,2,4,4}, {{SIMPLEREGION, {0,2,4,4}}, {SIMPLEREGION, {0,0,4,4}}, {SIMPLEREGION ,{0,0,4,2}}, {SIMPLEREGION, {0,0,4,2}}}}, // bottom
    {{0,0,4,4},{0,0,2,2}, {{SIMPLEREGION, {0,0,2,2}}, {SIMPLEREGION, {0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}}}, // top-left
    {{0,0,4,4},{2,0,4,2}, {{SIMPLEREGION, {2,0,4,2}}, {SIMPLEREGION, {0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}}}, // top-right
    {{0,0,4,4},{0,2,2,4}, {{SIMPLEREGION, {0,2,2,4}}, {SIMPLEREGION, {0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}}}, // bottom-left
    {{0,0,4,4},{2,2,4,4}, {{SIMPLEREGION, {2,2,4,4}}, {SIMPLEREGION, {0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}}}, // bottom-right
    {{0,0,4,4},{0,1,2,3}, {{SIMPLEREGION, {0,1,2,3}}, {SIMPLEREGION, {0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}}}, // center-left
    {{0,0,4,4},{1,0,3,2}, {{SIMPLEREGION, {1,0,3,2}}, {SIMPLEREGION, {0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}}}, // center-top
    {{0,0,4,4},{2,1,4,3}, {{SIMPLEREGION, {2,1,4,3}}, {SIMPLEREGION, {0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}}}, // center-right
    {{0,0,4,4},{1,2,3,4}, {{SIMPLEREGION, {1,2,3,4}}, {SIMPLEREGION, {0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}}}, // center-bottom
    {{0,0,4,4},{1,1,3,4}, {{SIMPLEREGION, {1,1,3,4}}, {SIMPLEREGION, {0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}}}, // center

    /* rgn 1 is within rgn 2 */
    {{0,0,2,4},{0,0,4,4}, {{SIMPLEREGION, {0,0,2,4}}, {SIMPLEREGION, {0,0,4,4}}, {SIMPLEREGION ,{2,0,4,4}}, {NULLREGION,   {0,0,0,0}}}}, // left
    {{0,0,4,2},{0,0,4,4}, {{SIMPLEREGION, {0,0,4,2}}, {SIMPLEREGION, {0,0,4,4}}, {SIMPLEREGION ,{0,2,4,4}}, {NULLREGION,   {0,0,0,0}}}}, // top
    {{2,0,4,4},{0,0,4,4}, {{SIMPLEREGION, {2,0,4,4}}, {SIMPLEREGION, {0,0,4,4}}, {SIMPLEREGION ,{0,0,2,4}}, {NULLREGION,   {0,0,0,0}}}}, // right
    {{0,2,4,4},{0,0,4,4}, {{SIMPLEREGION, {0,2,4,4}}, {SIMPLEREGION, {0,0,4,4}}, {SIMPLEREGION ,{0,0,4,2}}, {NULLREGION,   {0,0,0,0}}}}, // bottom
    {{0,0,2,2},{0,0,4,4}, {{SIMPLEREGION, {0,0,2,2}}, {SIMPLEREGION, {0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {NULLREGION,   {0,0,0,0}}}}, // top-left
    {{2,0,4,2},{0,0,4,4}, {{SIMPLEREGION, {2,0,4,2}}, {SIMPLEREGION, {0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {NULLREGION,   {0,0,0,0}}}}, // top-right
    {{0,2,2,4},{0,0,4,4}, {{SIMPLEREGION, {0,2,2,4}}, {SIMPLEREGION, {0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {NULLREGION,   {0,0,0,0}}}}, // bottom-left
    {{2,2,4,4},{0,0,4,4}, {{SIMPLEREGION, {2,2,4,4}}, {SIMPLEREGION, {0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {NULLREGION,   {0,0,0,0}}}}, // bottom-right
    {{0,1,2,3},{0,0,4,4}, {{SIMPLEREGION, {0,1,2,3}}, {SIMPLEREGION, {0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {NULLREGION,   {0,0,0,0}}}}, // center-left
    {{1,0,3,2},{0,0,4,4}, {{SIMPLEREGION, {1,0,3,2}}, {SIMPLEREGION, {0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {NULLREGION,   {0,0,0,0}}}}, // center-top
    {{2,1,4,3},{0,0,4,4}, {{SIMPLEREGION, {2,1,4,3}}, {SIMPLEREGION, {0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {NULLREGION,   {0,0,0,0}}}}, // center-right
    {{1,2,3,4},{0,0,4,4}, {{SIMPLEREGION, {1,2,3,4}}, {SIMPLEREGION, {0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {NULLREGION,   {0,0,0,0}}}}, // center-bottom
    {{1,1,3,4},{0,0,4,4}, {{SIMPLEREGION, {1,1,3,4}}, {SIMPLEREGION, {0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {NULLREGION,   {0,0,0,0}}}}, // center

    /* rgn 2 is adjacent to rgn 1 */
    {{2,2,4,4},{0,2,2,4}, {{NULLREGION,   {0,0,0,0}}, {SIMPLEREGION, {0,2,4,4}}, {SIMPLEREGION, {0,2,4,4}}, {SIMPLEREGION, {2,2,4,4}}}}, // left-fit
    {{2,2,4,4},{0,1,2,5}, {{NULLREGION,   {0,0,0,0}}, {COMPLEXREGION,{0,1,4,5}}, {COMPLEXREGION,{0,1,4,5}}, {SIMPLEREGION, {2,2,4,4}}}}, // left-larger
    {{2,2,4,4},{0,3,2,4}, {{NULLREGION,   {0,0,0,0}}, {COMPLEXREGION,{0,2,4,4}}, {COMPLEXREGION,{0,2,4,4}}, {SIMPLEREGION, {2,2,4,4}}}}, // left-smaller
    {{2,2,4,4},{2,0,4,2}, {{NULLREGION,   {0,0,0,0}}, {SIMPLEREGION, {2,0,4,4}}, {SIMPLEREGION, {2,0,4,4}}, {SIMPLEREGION, {2,2,4,4}}}}, // top-fit
    {{2,2,4,4},{1,0,5,2}, {{NULLREGION,   {0,0,0,0}}, {COMPLEXREGION,{1,0,5,4}}, {COMPLEXREGION,{1,0,5,4}}, {SIMPLEREGION, {2,2,4,4}}}}, // top-larger
    {{2,2,4,4},{3,0,4,2}, {{NULLREGION,   {0,0,0,0}}, {COMPLEXREGION,{2,0,4,4}}, {COMPLEXREGION,{2,0,4,4}}, {SIMPLEREGION, {2,2,4,4}}}}, // top-smaller
    {{2,2,4,4},{4,2,6,4}, {{NULLREGION,   {0,0,0,0}}, {SIMPLEREGION, {2,2,6,4}}, {SIMPLEREGION, {2,2,6,4}}, {SIMPLEREGION, {2,2,4,4}}}}, // right-fit
    {{2,2,4,4},{4,1,6,5}, {{NULLREGION,   {0,0,0,0}}, {COMPLEXREGION,{2,1,6,5}}, {COMPLEXREGION,{2,1,6,5}}, {SIMPLEREGION, {2,2,4,4}}}}, // right-larger
    {{2,2,4,4},{4,3,6,4}, {{NULLREGION,   {0,0,0,0}}, {COMPLEXREGION,{2,2,6,4}}, {COMPLEXREGION,{2,2,6,4}}, {SIMPLEREGION, {2,2,4,4}}}}, // right-smaller
    {{2,2,4,4},{2,4,4,6}, {{NULLREGION,   {0,0,0,0}}, {SIMPLEREGION, {2,2,4,6}}, {SIMPLEREGION, {2,2,4,6}}, {SIMPLEREGION, {2,2,4,4}}}}, // bottom-fit
    {{2,2,4,4},{1,4,5,6}, {{NULLREGION,   {0,0,0,0}}, {COMPLEXREGION,{1,2,5,6}}, {COMPLEXREGION,{1,2,5,6}}, {SIMPLEREGION, {2,2,4,4}}}}, // bottom-larger
    {{2,2,4,4},{3,4,4,6}, {{NULLREGION,   {0,0,0,0}}, {COMPLEXREGION,{2,2,4,6}}, {COMPLEXREGION,{2,2,4,6}}, {SIMPLEREGION, {2,2,4,4}}}}, // bottom-smaller

    /* rgn 2 overlaps rgn 1 */
    {{2,2,4,4},{0,2,3,4}, {{SIMPLEREGION, {2,2,3,4}}, {SIMPLEREGION, {0,2,4,4}}, {COMPLEXREGION,{0,2,4,4}}, {SIMPLEREGION, {3,2,4,4}}}}, // left-fit
    {{2,2,4,4},{0,1,3,5}, {{SIMPLEREGION, {2,2,3,4}}, {COMPLEXREGION,{0,1,4,5}}, {COMPLEXREGION,{0,1,4,5}}, {SIMPLEREGION, {3,2,4,4}}}}, // left-larger
    {{2,2,4,4},{0,3,3,4}, {{SIMPLEREGION, {2,3,3,4}}, {COMPLEXREGION,{0,2,4,4}}, {COMPLEXREGION,{0,2,4,4}}, {COMPLEXREGION,{2,2,4,4}}}}, // left-smaller
    {{2,2,4,4},{2,0,4,3}, {{SIMPLEREGION, {2,2,4,3}}, {SIMPLEREGION, {2,0,4,4}}, {COMPLEXREGION,{2,0,4,4}}, {SIMPLEREGION, {2,3,4,4}}}}, // top-fit
    {{2,2,4,4},{1,0,5,3}, {{SIMPLEREGION, {2,2,4,3}}, {COMPLEXREGION,{1,0,5,4}}, {COMPLEXREGION,{1,0,5,4}}, {SIMPLEREGION, {2,3,4,4}}}}, // top-larger
    {{2,2,4,4},{3,0,4,3}, {{SIMPLEREGION, {3,2,4,3}}, {COMPLEXREGION,{2,0,4,4}}, {COMPLEXREGION,{2,0,4,4}}, {COMPLEXREGION,{2,2,4,4}}}}, // top-smaller
    {{2,2,4,4},{3,2,6,4}, {{SIMPLEREGION, {3,2,4,4}}, {SIMPLEREGION, {2,2,6,4}}, {COMPLEXREGION,{2,2,6,4}}, {SIMPLEREGION, {2,2,3,4}}}}, // right-fit
    {{2,2,4,4},{3,1,6,5}, {{SIMPLEREGION, {3,2,4,4}}, {COMPLEXREGION,{2,1,6,5}}, {COMPLEXREGION,{2,1,6,5}}, {SIMPLEREGION, {2,2,3,4}}}}, // right-larger
    {{2,2,4,4},{3,3,6,4}, {{SIMPLEREGION, {3,3,4,4}}, {COMPLEXREGION,{2,2,6,4}}, {COMPLEXREGION,{2,2,6,4}}, {COMPLEXREGION,{2,2,4,4}}}}, // right-smaller
    {{2,2,4,4},{2,3,4,6}, {{SIMPLEREGION, {2,3,4,4}}, {SIMPLEREGION, {2,2,4,6}}, {COMPLEXREGION,{2,2,4,6}}, {SIMPLEREGION, {2,2,4,3}}}}, // bottom-fit
    {{2,2,4,4},{1,3,5,6}, {{SIMPLEREGION, {2,3,4,4}}, {COMPLEXREGION,{1,2,5,6}}, {COMPLEXREGION,{1,2,5,6}}, {SIMPLEREGION, {2,2,4,3}}}}, // bottom-larger
    {{2,2,4,4},{3,3,4,6}, {{SIMPLEREGION, {3,3,4,4}}, {COMPLEXREGION,{2,2,4,6}}, {COMPLEXREGION,{2,2,4,6}}, {COMPLEXREGION,{2,2,4,4}}}}, // bottom-smaller
    {{2,2,4,4},{0,0,3,3}, {{SIMPLEREGION, {2,2,3,3}}, {COMPLEXREGION,{0,0,4,4}}, {COMPLEXREGION,{0,0,4,4}}, {COMPLEXREGION,{2,2,4,4}}}}, // top-left
    {{2,2,4,4},{3,0,6,3}, {{SIMPLEREGION, {3,2,4,3}}, {COMPLEXREGION,{2,0,6,4}}, {COMPLEXREGION,{2,0,6,4}}, {COMPLEXREGION,{2,2,4,4}}}}, // top-right
    {{2,2,4,4},{0,3,3,6}, {{SIMPLEREGION, {2,3,3,4}}, {COMPLEXREGION,{0,2,4,6}}, {COMPLEXREGION,{0,2,4,6}}, {COMPLEXREGION,{2,2,4,4}}}}, // bottom-left
    {{2,2,4,4},{3,3,6,6}, {{SIMPLEREGION, {3,3,4,4}}, {COMPLEXREGION,{2,2,6,6}}, {COMPLEXREGION,{2,2,6,6}}, {COMPLEXREGION,{2,2,4,4}}}}, // bottom-right

    };

    HRGN hrgn1, hrgn2, hrgn3, hrgnRes;
    RECT rc;
    INT iComplexity;
    UINT i;

    hrgn1 =  CreateRectRgn(0, 0, 0, 0);
    hrgn2 =  CreateRectRgn(0, 0, 0, 0);
    hrgn3 =  CreateRectRgn(0, 0, 0, 0);
    hrgnRes = CreateRectRgn(0, 0, 0, 0);

    for (i = 0; i < sizeof(aRectTests) / sizeof(aRectTests[0]); i++)
    {
        INT iCombine;

        SetRectRgnIndirect(hrgn1, &aRectTests[i].rcSrc1);
        SetRectRgnIndirect(hrgn2, &aRectTests[i].rcSrc2);

        for (iCombine = RGN_AND; iCombine <= RGN_DIFF; iCombine++)
        {
            iComplexity = CombineRgn(hrgnRes, hrgn1, hrgn2, iCombine);
            ok(iComplexity == aRectTests[i].res[iCombine-1].iComplexity,
               "#%u (%s), iComplexity does not match: expected %u, got %u\n",
               i, apszRgnOp[iCombine], aRectTests[i].res[iCombine-1].iComplexity, iComplexity);

            ok(GetRgnBox(hrgnRes, &rc), "GetRgnBox failed!\n");
            ok(EqualRect(&rc, &aRectTests[i].res[iCombine-1].rcDstBounds),
               "#%u (%s), Bounding box does not match: expected {%ld,%ld,%ld,%ld} got {%ld,%ld,%ld,%ld}\n",
               i, apszRgnOp[iCombine],
               aRectTests[i].res[iCombine-1].rcDstBounds.left, aRectTests[i].res[iCombine-1].rcDstBounds.top,
               aRectTests[i].res[iCombine-1].rcDstBounds.right, aRectTests[i].res[iCombine-1].rcDstBounds.bottom,
               rc.left, rc.top, rc.right, rc.bottom);

            if (aRectTests[i].res[iCombine-1].iComplexity == SIMPLEREGION)
            {
                SetRectRgnIndirect(hrgn3, &aRectTests[i].res[iCombine-1].rcDstBounds);
                ok(EqualRgn(hrgnRes, hrgn3), "#%u (%s), regions are not equal\n",
                   i, apszRgnOp[iCombine]);
            }
        }
    }

    DeleteObject(hrgn1);
    DeleteObject(hrgn2);
    DeleteObject(hrgn3);
    DeleteObject(hrgnRes);
}


void Test_CombineRgn_Params()
{
    HRGN hrgn1, hrgn2, hrgn3;

    hrgn1 = CreateRectRgn(0, 0, 0, 0);
    hrgn2 = CreateRectRgn(0, 0, 10, 10);
    hrgn3 = CreateRectRgn(5, 5, 20, 20);

    SetLastError(0xbadbabe);
    ok_long(CombineRgn(NULL, NULL, NULL, 0), ERROR);
    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, 0), ERROR);
    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, 6), ERROR);
    ok((GetLastError() == 0xbadbabe) || (GetLastError() == ERROR_INVALID_PARAMETER), "wrong error: %ld\n", GetLastError());

}

void Test_CombineRgn_COPY()
{
    HRGN hrgn1, hrgn2, hrgn3;

    hrgn1 = CreateRectRgn(0, 0, 0, 0);
    hrgn2 = CreateRectRgn(0, 0, 10, 10);
    hrgn3 = CreateRectRgn(5, 5, 20, 20);

    SetLastError(0xbadbabe);
    ok_long(CombineRgn(NULL, NULL, NULL, RGN_COPY), ERROR);
    ok_long(CombineRgn(NULL, hrgn1, NULL, RGN_COPY), ERROR);
    ok_long(CombineRgn(NULL, NULL, hrgn1, RGN_COPY), ERROR);
    ok_long(CombineRgn(NULL, hrgn1, hrgn2, RGN_COPY), ERROR);
    ok((GetLastError() == 0xbadbabe) || (GetLastError() == ERROR_INVALID_HANDLE), "wrong error: %ld\n", GetLastError());

    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, RGN_COPY), SIMPLEREGION);
    ok(EqualRgn(hrgn1, hrgn2), "Region is not correct\n");

    ok_long(CombineRgn(hrgn1, hrgn1, NULL, RGN_COPY), SIMPLEREGION);
    ok(EqualRgn(hrgn1, hrgn2), "Region is not correct\n");

    ok_long(CombineRgn(hrgn1, hrgn3, GetStockObject(BLACK_PEN), RGN_COPY), SIMPLEREGION);
    ok(EqualRgn(hrgn1, hrgn3), "Region is not correct\n");

    ok_long(CombineRgn(hrgn1, GetStockObject(BLACK_PEN), hrgn2, RGN_COPY), ERROR);
    ok(EqualRgn(hrgn1, hrgn3), "Region is not correct\n");

    ok_long(CombineRgn(hrgn1, NULL, hrgn2, RGN_COPY), ERROR);
    ok(EqualRgn(hrgn1, hrgn3), "Region is not correct\n");

    ok_long(CombineRgn(hrgn1, NULL, NULL, RGN_COPY), ERROR);
    ok(EqualRgn(hrgn1, hrgn3), "Region is not correct\n");

    ok((GetLastError() == 0xbadbabe) || (GetLastError() == ERROR_INVALID_HANDLE), "wrong error: %ld\n", GetLastError());

}

void Test_CombineRgn_AND()
{
    HRGN hrgn1, hrgn2, hrgn3;

    hrgn1 = CreateRectRgn(0, 0, 0, 0);
    hrgn2 = CreateRectRgn(0, 0, 10, 10);
    hrgn3 = CreateRectRgn(5, 5, 20, 20);

    SetLastError(0xbadbabe);
    ok_long(CombineRgn(NULL, NULL, NULL, RGN_AND), ERROR);
    ok_long(CombineRgn(hrgn1, NULL, NULL, RGN_AND), ERROR);
    ok_long(CombineRgn(hrgn1, hrgn2, NULL, RGN_AND), ERROR);
    ok_long(CombineRgn(hrgn1, NULL, hrgn2, RGN_AND), ERROR);
    ok_long(CombineRgn(hrgn1, GetStockObject(BLACK_PEN), hrgn2, RGN_AND), ERROR);
    ok_long(CombineRgn(hrgn1, hrgn2, GetStockObject(BLACK_PEN), RGN_AND), ERROR);
    ok((GetLastError() == 0xbadbabe) || (GetLastError() == ERROR_INVALID_HANDLE), "wrong error: %ld\n", GetLastError());


    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, RGN_AND), SIMPLEREGION);
    SetRectRgn(hrgn2, 5, 5, 10, 10);
    ok(EqualRgn(hrgn1, hrgn2), "Region is not correct\n");

    SetRectRgn(hrgn2, 0, 0, 5, 5);
    SetRectRgn(hrgn3, 5, 0, 10, 5);
    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, RGN_AND), NULLREGION);
    SetRectRgn(hrgn2, 0, 0, 0, 0);
    ok(EqualRgn(hrgn1, hrgn2), "Region is not correct\n");

    SetRectRgn(hrgn2, 0, 0, 20, 20);
    SetRectRgn(hrgn3, 5, 5, 10, 10);
    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, RGN_AND), SIMPLEREGION);
    SetRectRgn(hrgn2, 5, 5, 10, 10);
    ok(EqualRgn(hrgn1, hrgn2), "Region is not correct\n");


    SetRectRgn(hrgn2, 0, 0, 30, 10);
    SetRectRgn(hrgn3, 10, 10, 20, 30);
    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, RGN_OR), COMPLEXREGION);
    SetRectRgn(hrgn2, 10, 0, 30, 30);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_AND), COMPLEXREGION);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn1, RGN_AND), COMPLEXREGION);
    SetRectRgn(hrgn2, 10, 10, 30, 30);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_AND), SIMPLEREGION);
    SetRectRgn(hrgn2, 0, 0, 10, 10);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_AND), NULLREGION);

    SetRectRgn(hrgn1, 0, 0, 30, 10);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn1, RGN_AND), SIMPLEREGION);

}

void Test_CombineRgn_OR()
{
    HRGN hrgn1, hrgn2, hrgn3;

    hrgn1 = CreateRectRgn(0, 0, 0, 0);
    hrgn2 = CreateRectRgn(0, 0, 5, 5);
    hrgn3 = CreateRectRgn(5, 0, 10, 5);
    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, RGN_OR), SIMPLEREGION);
    SetRectRgn(hrgn2, 0, 0, 10, 5);
    ok(EqualRgn(hrgn1, hrgn2), "Region is not correct\n");

    SetRectRgn(hrgn2, 0, 0, 10, 10);
    SetRectRgn(hrgn3, 10, 10, 20, 20);
    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, RGN_OR), COMPLEXREGION);
    SetRectRgn(hrgn2, 10, 0, 20, 10);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_OR), COMPLEXREGION);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn1, RGN_OR), COMPLEXREGION);
    SetRectRgn(hrgn2, 0, 10, 10, 20);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_OR), SIMPLEREGION);
    SetRectRgn(hrgn2, 0, 0, 20, 20);
    ok(EqualRgn(hrgn1, hrgn2), "Region is not correct\n");

}

void Test_CombineRgn_DIFF()
{
    HRGN hrgn1, hrgn2, hrgn3;

    hrgn1 = CreateRectRgn(0, 0, 0, 0);
    hrgn2 = CreateRectRgn(0, 0, 10, 10);
    hrgn3 = CreateRectRgn(5, 0, 10, 5);

    SetLastError(0xbadbabe);
    ok_long(CombineRgn(NULL, NULL, NULL, RGN_DIFF), ERROR);
    ok_long(CombineRgn(hrgn1, NULL, NULL, RGN_DIFF), ERROR);
    ok_long(CombineRgn(hrgn1, hrgn2, NULL, RGN_DIFF), ERROR);
    ok_long(CombineRgn(hrgn1, NULL, hrgn2, RGN_DIFF), ERROR);
    ok_long(CombineRgn(hrgn1, GetStockObject(BLACK_PEN), hrgn2, RGN_DIFF), ERROR);
    ok_long(CombineRgn(hrgn1, hrgn2, GetStockObject(BLACK_PEN), RGN_DIFF), ERROR);
    ok((GetLastError() == 0xbadbabe) || (GetLastError() == ERROR_INVALID_HANDLE), "wrong error: %ld\n", GetLastError());

    ok_long(CombineRgn(hrgn1, hrgn1, hrgn1, RGN_DIFF), NULLREGION);
    ok_long(CombineRgn(hrgn1, hrgn2, hrgn2, RGN_DIFF), NULLREGION);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_DIFF), NULLREGION);

    ok_long(CombineRgn(hrgn1, hrgn2, hrgn1, RGN_DIFF), SIMPLEREGION);
    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, RGN_DIFF), COMPLEXREGION);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn3, RGN_DIFF), COMPLEXREGION);
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_DIFF), NULLREGION);


}

void Test_CombineRgn_XOR()
{
    HRGN hrgn1, hrgn2, hrgn3, hrgn4;

    hrgn1 = CreateRectRgn(0, 0, 0, 0);
    hrgn2 = CreateRectRgn(0, 0, 5, 5);
    hrgn3 = CreateRectRgn(5, 5, 10, 10);
    hrgn4 = CreateRectRgn(0, 0, 0, 0);

    SetLastError(0xbadbabe);
    ok_long(CombineRgn(NULL, NULL, NULL, RGN_XOR), ERROR);
    ok_long(CombineRgn(hrgn1, NULL, NULL, RGN_XOR), ERROR);
    ok_long(CombineRgn(hrgn1, hrgn2, NULL, RGN_XOR), ERROR);
    ok_long(CombineRgn(hrgn1, NULL, hrgn2, RGN_XOR), ERROR);
    ok_long(CombineRgn(hrgn1, GetStockObject(BLACK_PEN), hrgn2, RGN_XOR), ERROR);
    ok_long(CombineRgn(hrgn1, hrgn2, GetStockObject(BLACK_PEN), RGN_XOR), ERROR);
    ok((GetLastError() == 0xbadbabe) || (GetLastError() == ERROR_INVALID_HANDLE), "wrong error: %ld\n", GetLastError());

    ok_long(CombineRgn(hrgn1, hrgn2, hrgn3, RGN_XOR), COMPLEXREGION);
    ok_long(CombineRgn(hrgn4, hrgn2, hrgn3, RGN_OR), COMPLEXREGION);
    ok(EqualRgn(hrgn1, hrgn4), "Region is not correct\n");

    ok_long(CombineRgn(hrgn1, hrgn1, hrgn2, RGN_XOR), SIMPLEREGION);
    ok(EqualRgn(hrgn1, hrgn3), "Region is not correct\n");
    ok_long(CombineRgn(hrgn1, hrgn1, hrgn3, RGN_XOR), NULLREGION);


}

START_TEST(CombineRgn)
{
    Test_CombineRgn_Params();
    Test_CombineRgn_COPY();
    Test_CombineRgn_AND();
    Test_CombineRgn_OR();
    Test_CombineRgn_DIFF();
    Test_CombineRgn_XOR();
    Test_RectRegions();
}

