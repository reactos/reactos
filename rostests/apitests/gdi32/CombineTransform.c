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

void Test_CombineTransform()
{
    XFORM xform1, xform2, xform3;
    BOOL ret;

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
    ok(ret == 1, "expected ret = 1, got %d\n", ret);
    ok(xform3.eM11 == 1.0, "eM11: Expected 1.0, got %f\n", xform3.eM11);
    ok(xform3.eM12 == 0, "eM11: Expected 0, got %f\n", xform3.eM12);
    ok(xform3.eM21 == 0, "eM11: Expected 0, got %f\n", xform3.eM21);
    ok(xform3.eM22 == 1.0, "eM11: Expected 1.0, got %f\n", xform3.eM22);
    ok(xform3.eDx == 0, "eM11: Expected 0, got %f\n", xform3.eDx);
    ok(xform3.eDy == 0, "eM11: Expected 0, got %f\n", xform3.eDy);
	ok(GetLastError() == ERROR_SUCCESS, "Exected ERROR_SUCCESS, got %ld\n", GetLastError());

    /* 2 Identity matrices with offsets */
    xform1.eDx = 20.0;
    xform1.eDy = -100;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok(ret == 1, "expected ret = 1, got %d\n", ret);
    ok(xform3.eM11 == 1.0, "eM11: Expected 1.0, got %f\n", xform3.eM11);
    ok(xform3.eM12 == 0, "eM11: Expected 0, got %f\n", xform3.eM12);
    ok(xform3.eM21 == 0, "eM11: Expected 0, got %f\n", xform3.eM21);
    ok(xform3.eM22 == 1.0, "eM11: Expected 1.0, got %f\n", xform3.eM22);
    ok(xform3.eDx == 20.0, "eM11: Expected 20.0, got %f\n", xform3.eDx);
    ok(xform3.eDy == -100, "eM11: Expected -100, got %f\n", xform3.eDy);
    xform2.eDx = -60.0;
    xform2.eDy = -20;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok(ret == 1, "expected ret = 1, got %d\n", ret);
    ok(xform3.eDx == -40.0, "eM11: Expected 40.0, got %f\n", xform3.eDx);
    ok(xform3.eDy == -120.0, "eM11: Expected -120.0, got %f\n", xform3.eDy);

    /* add some stretching */
    xform2.eM11 = 2;
    xform2.eM22 = 4;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok(ret == 1, "expected ret = 1, got %d\n", ret);
    ok(xform3.eM11 == 2.0, "eM11: Expected 2.0, got %f\n", xform3.eM11);
    ok(xform3.eM12 == 0, "eM11: Expected 0, got %f\n", xform3.eM12);
    ok(xform3.eM21 == 0, "eM11: Expected 0, got %f\n", xform3.eM21);
    ok(xform3.eM22 == 4.0, "eM11: Expected 4.0, got %f\n", xform3.eM22);
    ok(xform3.eDx == -20.0, "eM11: Expected 20.0, got %f\n", xform3.eDx);
    ok(xform3.eDy == -420.0, "eM11: Expected 420.0, got %f\n", xform3.eDy);

    /* add some more stretching */
    xform1.eM11 = -2.5;
    xform1.eM22 = 0.5;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok(ret == 1, "expected ret = 1, got %d\n", ret);
    ok(xform3.eM11 == -5.0, "eM11: Expected -5.0, got %f\n", xform3.eM11);
    ok(xform3.eM12 == 0, "eM11: Expected 0, got %f\n", xform3.eM12);
    ok(xform3.eM21 == 0, "eM11: Expected 0, got %f\n", xform3.eM21);
    ok(xform3.eM22 == 2.0, "eM11: Expected 2.0, got %f\n", xform3.eM22);
    ok(xform3.eDx == -20.0, "eM11: Expected 20.0, got %f\n", xform3.eDx);
    ok(xform3.eDy == -420.0, "eM11: Expected 420.0, got %f\n", xform3.eDy);

    xform1.eM12 = 2.0;
    xform1.eM21 = -0.5;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok(ret == 1, "expected ret = 1, got %d\n", ret);
    ok(xform3.eM11 == -5.0, "eM11: Expected -5.0, got %f\n", xform3.eM11);
    ok(xform3.eM12 == 8.0, "eM11: Expected 8.0, got %f\n", xform3.eM12);
    ok(xform3.eM21 == -1.0, "eM11: Expected -1.0, got %f\n", xform3.eM21);
    ok(xform3.eM22 == 2.0, "eM11: Expected 2.0, got %f\n", xform3.eM22);
    ok(xform3.eDx == -20.0, "eM11: Expected 20.0, got %f\n", xform3.eDx);
    ok(xform3.eDy == -420.0, "eM11: Expected 420.0, got %f\n", xform3.eDy);

    xform2.eM12 = 4.0;
    xform2.eM21 = 6.5;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok(ret == 1, "expected ret = 1, got %d\n", ret);
    ok(xform3.eM11 == 8.0, "eM11: Expected 8.0, got %f\n", xform3.eM11);
    ok(xform3.eM12 == -2.0, "eM11: Expected -2.0, got %f\n", xform3.eM12);
    ok(xform3.eM21 == 2.25, "eM11: Expected 2.25, got %f\n", xform3.eM21);
    ok(xform3.eM22 == 0.00, "eM11: Expected 0.00, got %f\n", xform3.eM22);
    ok(xform3.eDx == -670.0, "eM11: Expected -670.0, got %f\n", xform3.eDx);
    ok(xform3.eDy == -340.0, "eM11: Expected -340.0, got %f\n", xform3.eDy);
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok(ret == 1, "expected ret = 1, got %d\n", ret);

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
    ok(ret == 0, "expected ret = 0, got %d\n", ret);
	ok(GetLastError() == ERROR_SUCCESS, "Exected ERROR_SUCCESS, got %ld\n", GetLastError());

    xform1.eDx = -4294967167.999999761;
    ok(xform1.eDx == -4294967040.0, "float rounding error.\n");
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok(ret == 1, "expected ret = 1, got %d\n", ret);

    xform1.eDx = -4294967167.999999762;
    ok(xform1.eDx == -4294967296.0, "float rounding error.\n");
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok(ret == 0, "expected ret = 0, got %d\n", ret);
	ok(GetLastError() == ERROR_SUCCESS, "Exected ERROR_SUCCESS, got %ld\n", GetLastError());

    xform1.eDx = 0;
    xform1.eDy = 4294967167.999999761;
    ok(xform1.eDy == 4294967040.0, "float rounding error.\n");
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok(ret == 1, "expected ret = 1, got %d\n", ret);

    xform1.eDy = 4294967167.999999762;
    ok(xform1.eDy == 4294967296.0, "float rounding error.\n");
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok(ret == 0, "expected ret = 0, got %d\n", ret);
	ok(GetLastError() == ERROR_SUCCESS, "Exected ERROR_SUCCESS, got %ld\n", GetLastError());

    xform1.eDy = -4294967167.999999761;
    ok(xform1.eDy == -4294967040.0, "float rounding error.\n");
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok(ret == 1, "expected ret = 1, got %d\n", ret);

    xform1.eDy = -4294967167.999999762;
    ok(xform1.eDy == -4294967296.0, "float rounding error.\n");
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok(ret == 0, "expected ret = 0, got %d\n", ret);
	ok(GetLastError() == ERROR_SUCCESS, "Exected ERROR_SUCCESS, got %ld\n", GetLastError());

    xform1.eDx = 100000.0;
    xform2.eM11 = 100000.0;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok(ret == 0, "expected ret = 0, got %d\n", ret);
	ok(GetLastError() == ERROR_SUCCESS, "Exected ERROR_SUCCESS, got %ld\n", GetLastError());

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
    ok(ret == 1, "expected ret = 1, got %d\n", ret);
    ok(*(DWORD*)&xform3.eM11 == 0xffc00000, "eM11: Expected 0xffc00000, got 0x%lx\n", *(DWORD*)&xform3.eM11);
    ok(xform3.eM12 == 0, "eM12: Expected 0, got %f\n", xform3.eM12);
    ok(xform3.eM21 == 0, "eM21: Expected 0, got %f\n", xform3.eM21);
    ok(*(DWORD*)&xform3.eM22 == 0x7f800000, "eM22: Expected 0x7f800000, got 0x%lx\n", *(DWORD*)&xform3.eM22);
    ok(xform3.eDx == 0, "eDx: Expected 0, got %f\n", xform3.eDx);
    ok(xform3.eDy == 0, "eDy: Expected 0, got %f\n", xform3.eDy);
	ok(GetLastError() == ERROR_SUCCESS, "Exected ERROR_SUCCESS, got %ld\n", GetLastError());

    /* Some undefined values */
    xform2.eM11 = 1.;
    xform2.eM22 = 1.;
    xform2.eM12 = 1.;
    xform2.eM21 = 1.;
    ret = CombineTransform(&xform3, &xform1, &xform2);
    ok(ret == 1, "expected ret = 1, got %d\n", ret);
    ok(*(DWORD*)&xform3.eM11 == 0xffc00000, "eM11: Expected 0xffc00000, got 0x%lx\n", *(DWORD*)&xform3.eM11);
    ok(*(DWORD*)&xform3.eM12 == 0xffc00000, "eM12: Expected 0xffc00000, got 0x%lx\n", *(DWORD*)&xform3.eM12);
    ok(*(DWORD*)&xform3.eM21 == 0x7f800000, "eM21: Expected 0x7f800000, got 0x%lx\n", *(DWORD*)&xform3.eM21);
    ok(*(DWORD*)&xform3.eM22 == 0x7f800000, "eM22: Expected 0x7f800000, got 0x%lx\n", *(DWORD*)&xform3.eM22);
    ok(xform3.eDx == 0, "eDx: Expected 0, got %f\n", xform3.eDx);
    ok(xform3.eDy == 0, "eDy: Expected 0, got %f\n", xform3.eDy);
	ok(GetLastError() == ERROR_SUCCESS, "Exected ERROR_SUCCESS, got %ld\n", GetLastError());

}

START_TEST(CombineTransform)
{
    Test_CombineTransform();
}

