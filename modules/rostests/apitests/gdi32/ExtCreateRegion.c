/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for ExtCeateRegion
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

#include <math.h>

VOID
InitXFORM(
    PXFORM pxform,
    FLOAT eM11,
    FLOAT eM12,
    FLOAT eM21,
    FLOAT eM22,
    FLOAT eDx,
    FLOAT eDy)
{
    pxform->eM11 = eM11;
    pxform->eM12 = eM12;
    pxform->eM21 = eM21;
    pxform->eM22 = eM22;
    pxform->eDx = eDx;
    pxform->eDy = eDy;
}

#if 0
void Test_ExtCreateRegion_Parameters()
{
    hrgn = ExtCreateRegion(NULL, 1, pRgnData);
    ok(hrgn != NULL, "ExtCreateRegion failed\n");
}
#endif // 0

#define CheckRect(prect, _left, _top, _right, _bottom) \
do { \
    ok(((prect)->left == _left) && ((prect)->top == _top) && \
       ((prect)->right == _right) && ((prect)->bottom == _bottom), \
       "Region does not match, expected (%d,%d,%d,%d) got (%ld,%ld,%ld,%ld)\n", \
       _left, _top, _right, _bottom, \
       (prect)->left, (prect)->top, (prect)->right, (prect)->bottom); \
} while (0)

#define CheckRectRegion(hrgn, _left, _top, _right, _bottom) \
do { \
    HRGN hrgnTemp = CreateRectRgn(_left, _top, _right, _bottom); \
    RECT rcTemp; \
    ok(GetRgnBox(hrgn, &rcTemp) == SIMPLEREGION, "Region is not SIMPLEREGION\n"); \
    CheckRect(&rcTemp, _left, _top, _right, _bottom); \
    ok(EqualRgn(hrgn, hrgnTemp), "Region does not match\n"); \
    DeleteObject(hrgnTemp); \
} while (0)

void Test_ExtCreateRegion_Transform()
{
    struct
    {
        RGNDATA rgndata;
        CHAR data[sizeof(RECT) - 1];
    } RgnDataBuffer;
    PRECT prect = (PRECT)&RgnDataBuffer.rgndata.Buffer;
    const RGNDATA *pRgnData = (const RGNDATA *)&RgnDataBuffer;
    XFORM xform;
    FLOAT eAngle;
    HRGN hrgn;
    RECT rcTemp;

    RgnDataBuffer.rgndata.rdh.dwSize = sizeof(RGNDATAHEADER);
    RgnDataBuffer.rgndata.rdh.iType = RDH_RECTANGLES;
    RgnDataBuffer.rgndata.rdh.nCount = 1;
    RgnDataBuffer.rgndata.rdh.nRgnSize = sizeof(RGNDATAHEADER) + sizeof(RECT);
    RgnDataBuffer.rgndata.rdh.rcBound.left = 0;
    RgnDataBuffer.rgndata.rdh.rcBound.top = 0;
    RgnDataBuffer.rgndata.rdh.rcBound.right = 10;
    RgnDataBuffer.rgndata.rdh.rcBound.bottom = 10;
    prect->left = 0;
    prect->top = 0;
    prect->right = 10;
    prect->bottom = 10;

    SetRectEmpty(&RgnDataBuffer.rgndata.rdh.rcBound);

    hrgn = ExtCreateRegion(NULL, sizeof(RgnDataBuffer), pRgnData);
    ok(hrgn != NULL, "ExtCreateRegion failed with without transform\n");
    CheckRectRegion(hrgn, 0, 0, 10, 10);

    InitXFORM(&xform, 1., 0., 0., 1., 0., 0.);
    hrgn = ExtCreateRegion(&xform, sizeof(RgnDataBuffer), pRgnData);
    ok(hrgn != NULL, "ExtCreateRegion failed with identity transform\n");
    CheckRectRegion(hrgn, 0, 0, 10, 10);

    InitXFORM(&xform, 1., 0., 0., 1., 10., 10.);
    hrgn = ExtCreateRegion(&xform, sizeof(RgnDataBuffer), pRgnData);
    ok(hrgn != NULL, "ExtCreateRegion failed with offset transform\n");
    CheckRectRegion(hrgn, 10, 10, 20, 20);

    InitXFORM(&xform, 2.5, 0., 0., 1.5, 0., 0.);
    hrgn = ExtCreateRegion(&xform, sizeof(RgnDataBuffer), pRgnData);
    ok(hrgn != NULL, "ExtCreateRegion failed with scaling transform\n");
    CheckRectRegion(hrgn, 0, 0, 25, 15);

    InitXFORM(&xform, 2.5, 0., 0., 1.5, 20., 40.);
    hrgn = ExtCreateRegion(&xform, sizeof(RgnDataBuffer), pRgnData);
    ok(hrgn != NULL, "ExtCreateRegion failed with scaling+offset transform\n");
    CheckRectRegion(hrgn, 20, 40, 45, 55);

    InitXFORM(&xform, 1., 10., 0., 1., 0., 0.);
    hrgn = ExtCreateRegion(&xform, sizeof(RgnDataBuffer), pRgnData);
    ok(hrgn != NULL, "ExtCreateRegion failed with shearing transform\n");
    ok(GetRgnBox(hrgn, &rcTemp) == COMPLEXREGION, "not a complex region\n");
    CheckRect(&rcTemp, 0, 1, 10, 101);

    eAngle = 23.6f;
    InitXFORM(&xform, cosf(eAngle), -sinf(eAngle), sinf(eAngle), cosf(eAngle), 10., 10.);
    hrgn = ExtCreateRegion(&xform, sizeof(RgnDataBuffer), pRgnData);
    ok(hrgn != NULL, "ExtCreateRegion failed with rotating transform\n");
    CheckRectRegion(hrgn, 0, 10, 10, 20);

}

START_TEST(ExtCreateRegion)
{
    Test_ExtCreateRegion_Transform();
}

