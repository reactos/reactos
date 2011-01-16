/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for CreatePen
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>
#include <winddi.h>
#include <reactos/win32k/ntgdityp.h>
#include <reactos/win32k/ntgdihdl.h>

#define ok_int(x, y) ok(x == y, "Wrong value for " #x ", expected " #y ", got %ld\n", (long)x);
#define ok_flt(x, y) ok(x == y, "Wrong value for " #x ", expected " #y ", got %f\n", (double)x);

#define ok_xform(xform, m11, m12, m21, m22, dx, dy) \
    ok_flt(xform.eM11, m11); \
    ok_flt(xform.eM12, m12); \
    ok_flt(xform.eM21, m21); \
    ok_flt(xform.eM22, m22); \
    ok_flt(xform.eDx, dx); \
    ok_flt(xform.eDy, dy);

void Test_CombineTransform()
{
    XFORM xform1, xform2, xform3;
    BOOL ret;

    /* Test NULL paramters */
    SetLastError(ERROR_SUCCESS);
    ret = CombineTransform(&xform3, &xform1, NULL);
    ok_int(ret, 0);
    ret = CombineTransform(&xform3, NULL, &xform2);
    ok_int(ret, 0);
    ret = CombineTransform(NULL, &xform1, &xform2);
    ok_int(ret, 0);
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 0);
    ok_int(GetLastError(), ERROR_SUCCESS);

    /* 2 Identity matrices */
    xform1.eM11 = 1.0;
    xform1.eM12 = 0;
    xform1.eM21 = 0;
    xform1.eM22 = 1.0;
    xform1.eDx = 0;
    xform1.eDy = 0;
    xform2 = xform1;
    SetLastError(ERROR_SUCCESS);
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 1);
    ok_xform(xform3, 1.0, 0., 0., 1.0, 0., 0.);
    ok_int(GetLastError(), ERROR_SUCCESS);

    /* 2 Identity matrices with offsets */
    xform1.eDx = 20.0;
    xform1.eDy = -100;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 1);
    ok_xform(xform3, 1.0, 0., 0., 1.0, 20.0, -100.0);

    xform2.eDx = -60.0;
    xform2.eDy = -20;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 1);
    ok_flt(xform3.eDx, -40.0);
    ok_flt(xform3.eDy, -120.0);

    /* add some stretching */
    xform2.eM11 = 2;
    xform2.eM22 = 4;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 1);
    ok_xform(xform3, 2.0, 0., 0., 4.0, -20.0, -420.0);

    /* add some more stretching */
    xform1.eM11 = -2.5;
    xform1.eM22 = 0.5;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 1);
    ok_xform(xform3, -5.0, 0., 0., 2.0, -20.0, -420.0);

    xform1.eM12 = 2.0;
    xform1.eM21 = -0.5;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 1);
    ok_xform(xform3, -5.0, 8.0, -1.0, 2.0, -20.0, -420.0);

    xform2.eM12 = 4.0;
    xform2.eM21 = 6.5;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 1);
    ok_xform(xform3, 8.0, -2.0, 2.25, 0.0, -670.0, -340.0);

    xform1.eM11 = 1.;
    xform1.eM12 = 0;
    xform1.eM21 = 0;
    xform1.eM22 = 1.;
    xform1.eDx = 0;
    xform1.eDy = 0;
    xform2 = xform1;

    xform1.eDx = 4294967167.999999761;
    ok(xform1.eDx == 4294967040.0, "float rounding error.\n");
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok(ret == 1, "expected ret = 1, got %d\n", ret);

    xform1.eDx = 4294967167.999999762;
    ok(xform1.eDx == 4294967296.0, "float rounding error.\n");
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 0);
    ok_int(GetLastError(), ERROR_SUCCESS);

    xform1.eDx = -4294967167.999999761;
    ok(xform1.eDx == -4294967040.0, "float rounding error.\n");
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 1);

    xform1.eDx = -4294967167.999999762;
    ok(xform1.eDx == -4294967296.0, "float rounding error.\n");
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 0);
    ok_int(GetLastError(), ERROR_SUCCESS);

    xform1.eDx = 0;
    xform1.eDy = 4294967167.999999761;
    ok(xform1.eDy == 4294967040.0, "float rounding error.\n");
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 1);

    xform2.eDy = 1;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 1);

    xform1.eDy = 4294967167.999999762;
    ok(xform1.eDy == 4294967296.0, "float rounding error.\n");
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 0);
    ok_int(GetLastError(), ERROR_SUCCESS);

    xform1.eDy = -4294967167.999999761;
    ok(xform1.eDy == -4294967040.0, "float rounding error.\n");
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 1);

    xform1.eDy = -4294967167.999999762;
    ok(xform1.eDy == -4294967296.0, "float rounding error.\n");
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 0);
    ok_int(GetLastError(), ERROR_SUCCESS);

    xform2.eDy = 10000;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 1);

    xform1.eDx = -4294967167.999999762;
    xform1.eM11 = 1000.0;
    xform2.eM11 = 1000.0;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 0);
    ok_int(GetLastError(), ERROR_SUCCESS);

    xform1.eDx = 100000.0;
    xform2.eM11 = 100000.0;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 0);
    ok_int(GetLastError(), ERROR_SUCCESS);

    /* Some undefined values */
    *(DWORD*)&xform1.eM11 = 0xffc00000; // (0.0F/0.0F)
    xform1.eM12 = 0;
    xform1.eM21 = 0;
    *(DWORD*)&xform1.eM22 = 0x7f800000; // (1.0F/0.0F)
    xform1.eDx = 0;
    xform1.eDy = 0;
    xform2 = xform1;
    SetLastError(ERROR_SUCCESS);
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 1);
    ok(*(DWORD*)&xform3.eM11 == 0xffc00000, "eM11: Expected 0xffc00000, got 0x%lx\n", *(DWORD*)&xform3.eM11);
    ok(xform3.eM12 == 0, "eM12: Expected 0, got %f\n", xform3.eM12);
    ok(xform3.eM21 == 0, "eM21: Expected 0, got %f\n", xform3.eM21);
    ok(*(DWORD*)&xform3.eM22 == 0x7f800000, "eM22: Expected 0x7f800000, got 0x%lx\n", *(DWORD*)&xform3.eM22);
    ok(xform3.eDx == 0, "eDx: Expected 0, got %f\n", xform3.eDx);
    ok(xform3.eDy == 0, "eDy: Expected 0, got %f\n", xform3.eDy);
    ok_int(GetLastError(), ERROR_SUCCESS);

    /* Some undefined values */
    xform2.eM11 = 1.;
    xform2.eM22 = 1.;
    xform2.eM12 = 1.;
    xform2.eM21 = 1.;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok_int(ret, 1);

    ok_int(*(DWORD*)&xform3.eM11, 0xffc00000);
    ok_int(*(DWORD*)&xform3.eM12, 0xffc00000);
    ok_int(*(DWORD*)&xform3.eM21, 0x7f800000);
    ok_int(*(DWORD*)&xform3.eM22, 0x7f800000);
    ok(xform3.eDx == 0, "eDx: Expected 0, got %f\n", xform3.eDx);
    ok(xform3.eDy == 0, "eDy: Expected 0, got %f\n", xform3.eDy);
    ok_int(GetLastError(), ERROR_SUCCESS);

}

START_TEST(CombineTransform)
{
    Test_CombineTransform();
}

