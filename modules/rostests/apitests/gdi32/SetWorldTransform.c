/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetWorldTransform
 * PROGRAMMERS:     Timo Kreuzer
 *                  Katayama Hirofumi MZ
 */

#include "precomp.h"

void Test_SetWorldTransform()
{
    HDC hdcScreen, hdc;
    XFORM xform;
    BOOL result;
    //PGDI_TABLE_ENTRY pEntry;
    //DC_ATTR* pdcattr;

    /* Create a DC */
    hdcScreen = GetDC(NULL);
    hdc = CreateCompatibleDC(hdcScreen);
    ReleaseDC(NULL, hdcScreen);
    SetGraphicsMode(hdc, GM_ADVANCED);

    /* Set identity transform */
    xform.eM11 = 1;
    xform.eM12 = 0;
    xform.eM21 = 0;
    xform.eM22 = 1;
    xform.eDx = 0;
    xform.eDy = 0;
    result = SetWorldTransform(hdc, &xform);
    ok(result == 1, "SetWorldTransform should succeed\n");

    /* Set eM11 to 0 */
    xform.eM11 = 0;
    result = SetWorldTransform(hdc, &xform);
    ok(result == 0, "SetWorldTransform should fail\n");

    /* Set eM22 to 0 */
    xform.eM11 = 1;
    xform.eM22 = 0;
    result = SetWorldTransform(hdc, &xform);
    ok(result == 0, "SetWorldTransform should fail\n");

    /* Set values that result in the determinant being 0 */
    xform.eM11 = 2;
    xform.eM12 = 3;
    xform.eM21 = 4;
    xform.eM22 = 6;
    result = SetWorldTransform(hdc, &xform);
    ok(result == 0, "SetWorldTransform should fail\n");

    /* Small modification to make the determinant != 0 */
    xform.eM12 = (FLOAT)3.0001;
    result = SetWorldTransform(hdc, &xform);
    ok(result == 1, "SetWorldTransform should succeed\n");

    /* Set values that result in the determinant being 0 due to rounding */
    xform.eM11 = 1;
    xform.eM12 = (FLOAT)0.9999999;
    xform.eM21 = (FLOAT)1.0000001;
    xform.eM22 = 1;
    ok(xform.eM12 != (FLOAT)1.0, "xform.eM12 shouldn't be 1.0\n");
    ok(xform.eM21 != (FLOAT)1.0, "xform.eM21 shouldn't be 1.0\n");
    ok(xform.eM12 * xform.eM21 != (FLOAT)1.0, "xform.eM12 * xform.eM21 shouldn't be 1.0\n");
    result = SetWorldTransform(hdc, &xform);
    ok(result == 0, "SetWorldTransform should fail\n");

    /* Test world transform (should be unchanged by previous failure) */
    result = GetWorldTransform(hdc, &xform);
    ok(result == 1, "GetWorldTransform should succeed\n");
    ok(xform.eM11 == 2, "xform.eM11 should be 2\n");
    ok(xform.eM12 == (FLOAT)3.0001, "xform.eM12 should be 3.0001\n");
    ok(xform.eM21 == 4, "xform.eM21 should be 4\n");
    ok(xform.eM22 == 6, "xform.eM22 should be 6\n");

    /* Set smallest possible values */
    xform.eM11 = 1.17549435e-38f;
    xform.eM12 = 0;
    xform.eM21 = 0;
    xform.eM22 = 1.17549435e-38f;
    ok(xform.eM11 != (FLOAT)0.0, "xform.eM11 shouldn't be 0.0\n");
    ok(xform.eM22 != (FLOAT)0.0, "xform.eM22 shouldn't be 0.0\n");
    ok(xform.eM11 * xform.eM22 != (FLOAT)0.0, "xform.eM12 * xform.eM21 shouldn't be 0.0\n");
    result = SetWorldTransform(hdc, &xform);
    ok(result == 1, "SetWorldTransform should succeed\n");

    /* Test world transform */
    result = GetWorldTransform(hdc, &xform);
    ok(result == 1, "GetWorldTransform should succeed\n");
    ok(xform.eM11 > 0, "xform.eM11 should not be 0\n");
    ok(xform.eM12 == 0, "xform.eM12 should be 0\n");
    ok(xform.eM21 == 0, "xform.eM21 should be 0\n");
    ok(xform.eM22 > 0, "xform.eM22 should not be 0\n");

    xform.eM11 = 0;
    xform.eM12 = 1;
    xform.eM21 = 1;
    xform.eM22 = 0;
    result = SetWorldTransform(hdc, &xform);
    ok_int(result, 1);

    xform.eM11 = 1;
    xform.eM12 = 1;
    xform.eM21 = 1;
    xform.eM22 = 1;
    result = SetWorldTransform(hdc, &xform);
    ok_int(result, 0);

    result = GetWorldTransform(hdc, &xform);
    ok_int(result, 1);
    ok(xform.eM11 == 0, "xform.eM11 should be 0\n");
    ok(xform.eM12 == 1, "xform.eM12 should be 1\n");
    ok(xform.eM21 == 1, "xform.eM21 should be 1\n");
    ok(xform.eM22 == 0, "xform.eM22 should be 0\n");

    DeleteDC(hdc);
}

START_TEST(SetWorldTransform)
{
    Test_SetWorldTransform();
}
