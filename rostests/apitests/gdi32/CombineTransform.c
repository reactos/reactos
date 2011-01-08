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

#define MAX_OFFSET 4294967040.0

BOOL
WINAPI
CombineTransform(
    LPXFORM pxfResult,
    const XFORM *pxf1,
    const XFORM *pxf2)
{
    XFORM xformTmp;

    /* Do matrix multiplication */
    xformTmp.eM11 = pxf1->eM11 * pxf2->eM11 + pxf1->eM12 * pxf2->eM21;
    xformTmp.eM12 = pxf1->eM11 * pxf2->eM12 + pxf1->eM12 * pxf2->eM22;
    xformTmp.eM21 = pxf1->eM21 * pxf2->eM11 + pxf1->eM22 * pxf2->eM21;
    xformTmp.eM22 = pxf1->eM21 * pxf2->eM12 + pxf1->eM22 * pxf2->eM22;
    xformTmp.eDx = pxf1->eDx * pxf2->eM11 + pxf1->eDy * pxf2->eM21 + pxf2->eDx;
    xformTmp.eDy = pxf1->eDx * pxf2->eM12 + pxf1->eDy * pxf2->eM22 + pxf2->eDy;

#if 0
    ok(pxf1->eM11 * pxf2->eM11 == pxf2->eM11 * pxf1->eM11, "1: %f, %f -> %f, %f\n",
       pxf1->eM11, pxf2->eM11, pxf1->eM11 * pxf2->eM11, pxf2->eM11 * pxf1->eM11);
    ok(pxf1->eM11 * pxf2->eM12 == pxf2->eM12 * pxf1->eM11, "2: %f, %f -> %f, %f\n",
       pxf1->eM11, pxf2->eM12, pxf1->eM11 * pxf2->eM12, pxf2->eM12 * pxf1->eM11);
    ok(pxf1->eM21 * pxf2->eM11 == pxf2->eM11 * pxf1->eM21, "3\n");
    ok(pxf1->eM21 * pxf2->eM12 == pxf2->eM12 * pxf1->eM21, "4\n");

    ok(pxf1->eM12 * pxf2->eM21 == pxf2->eM21 * pxf1->eM12, "5\n");
    ok(pxf1->eM12 * pxf2->eM22 == pxf2->eM22 * pxf1->eM12, "6\n");
    ok(pxf1->eM22 * pxf2->eM21 == pxf2->eM21 * pxf1->eM22, "7\n");
    ok(pxf1->eM22 * pxf2->eM22 == pxf2->eM22 * pxf1->eM22, "8\n");
#endif

    *pxfResult = xformTmp;

    /* windows compatibility fixups */
    if (_isnan(xformTmp.eM12))
    {
        if (pxf1->eM11 == 0 || pxf2->eM12 == 0) pxfResult->eM12 = 0.;
    }

    if (_isnan(xformTmp.eM21)) pxfResult->eM21 = 0.;
    if (_isnan(xformTmp.eDx)) pxfResult->eDx = 0.;
    if (_isnan(xformTmp.eDy)) pxfResult->eDy = 0.;

    /* Check for invalid offset range */
    if (xformTmp.eDx > MAX_OFFSET || xformTmp.eDx < -MAX_OFFSET ||
        xformTmp.eDy > MAX_OFFSET || xformTmp.eDy < -MAX_OFFSET)
    {
        return FALSE;
    }

    return TRUE;
}

void
RandXform(XFORM *pxform)
{
    XFORML *pxforml = (XFORML*)pxform;

    pxforml->eM11 ^= (rand() << 16) ^ (rand() << (rand() % 16)) ^ rand();
    pxforml->eM12 ^= (rand() << 16) ^ (rand() << (rand() % 16)) ^ rand();
    pxforml->eM21 ^= (rand() << 16) ^ (rand() << (rand() % 16)) ^ rand();
    pxforml->eM22 ^= (rand() << 16) ^ (rand() << (rand() % 16)) ^ rand();
    pxforml->eDx ^= (rand() << 16) ^ (rand() << (rand() % 16)) ^ rand();
    pxforml->eDy ^= (rand() << 16) ^ (rand() << (rand() % 16)) ^ rand();


}

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

