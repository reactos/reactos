/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for CombineTransform
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

typedef union
{
    float e;
    long l;
} FLT_LONG;

#define ok_flt(x, y) \
{ \
    FLT_LONG __x, __y; \
    __x.e = (x); \
    __y.e = (y); \
    if (_isnan(y)) {\
      ok((__x.l == __y.l) || (__x.l == 0), "Wrong value for " #x ", expected " #y " (%f), got %f\n", (double)(y), (double)(x)); \
    } else {\
      ok(__x.l == __y.l, "Wrong value for " #x ", expected " #y " (%f), got %f\n", (double)(y), (double)(x)); \
    } \
}

#define ok_xform(xform, m11, m12, m21, m22, dx, dy) \
    ok_flt(xform.eM11, m11); \
    ok_flt(xform.eM12, m12); \
    ok_flt(xform.eM21, m21); \
    ok_flt(xform.eM22, m22); \
    ok_flt(xform.eDx, dx); \
    ok_flt(xform.eDy, dy);

#define set_xform(pxform, m11, m12, m21, m22, dx, dy) \
    (pxform)->eM11 = m11; \
    (pxform)->eM12 = m12; \
    (pxform)->eM21 = m21; \
    (pxform)->eM22 = m22; \
    (pxform)->eDx = dx; \
    (pxform)->eDy = dy;

float geINF;
float geIND;
float geQNAN;

void Test_CombineTransform()
{
    XFORM xform1, xform2, xform3;

    /* Test NULL paramters */
    set_xform(&xform1, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
    set_xform(&xform2, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
    SetLastError(ERROR_SUCCESS);
    ok_int(CombineTransform(&xform3, &xform1, NULL), 0);
    ok_int(CombineTransform(&xform3, NULL, &xform2), 0);
    ok_int(CombineTransform(NULL, &xform1, &xform2), 0);
    ok_int(GetLastError(), ERROR_SUCCESS);

    /* 2 zero matrices */
    set_xform(&xform1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    set_xform(&xform2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    SetLastError(ERROR_SUCCESS);
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    ok_int(GetLastError(), ERROR_SUCCESS);

    /* 2 Identity matrices */
    set_xform(&xform1, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
    set_xform(&xform2, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
    SetLastError(ERROR_SUCCESS);
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, 1.0, 0., 0., 1.0, 0., 0.);
    ok_int(GetLastError(), ERROR_SUCCESS);

    /* 2 Identity matrices with offsets */
    set_xform(&xform1, 1.0, 0.0, 0.0, 1.0, 20.0, -100.0);
    set_xform(&xform2, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, 1.0, 0., 0., 1.0, 20.0, -100.0);

    xform2.eDx = -60.0;
    xform2.eDy = -20;
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_flt(xform3.eDx, -40.0);
    ok_flt(xform3.eDy, -120.0);

    /* add some stretching */
    xform2.eM11 = 2;
    xform2.eM22 = 4;
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, 2.0, 0., 0., 4.0, -20.0, -420.0);

    /* add some more stretching */
    xform1.eM11 = -2.5;
    xform1.eM22 = 0.5;
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, -5.0, 0., 0., 2.0, -20.0, -420.0);

    xform1.eM12 = 2.0;
    xform1.eM21 = -0.5;
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, -5.0, 8.0, -1.0, 2.0, -20.0, -420.0);

    xform2.eM12 = 4.0;
    xform2.eM21 = 6.5;
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, 8.0, -2.0, 2.25, 0.0, -670.0, -340.0);

    set_xform(&xform1, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
    set_xform(&xform2, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
    set_xform(&xform3, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    xform1.eDx = (FLOAT)4294967167.999999761;
    ok(xform1.eDx == 4294967040.0, "float rounding error.\n");
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, 1.0, 0.0, 0.0, 1.0, 4294967040.0, 0.0);

    set_xform(&xform3, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    xform1.eDx = (FLOAT)4294967167.999999762;
    ok(xform1.eDx == 4294967296.0, "float rounding error.\n");
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 0);
    ok_int(GetLastError(), ERROR_SUCCESS);
    ok_xform(xform3, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

    xform1.eDx = (FLOAT)-4294967167.999999761;
    ok(xform1.eDx == -4294967040.0, "float rounding error.\n");
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);

    xform1.eDx = (FLOAT)-4294967167.999999762;
    ok(xform1.eDx == -4294967296.0, "float rounding error.\n");
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 0);
    ok_int(GetLastError(), ERROR_SUCCESS);

    xform1.eDx = 0;
    xform1.eDy = (FLOAT)4294967167.999999761;
    ok(xform1.eDy == 4294967040.0, "float rounding error.\n");
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);

    xform2.eDy = 1;
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_flt(xform3.eDy, 4294967040.0);

    xform1.eDy = (FLOAT)4294967167.999999762;
    ok(xform1.eDy == 4294967296.0, "float rounding error.\n");
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 0);
    ok_int(GetLastError(), ERROR_SUCCESS);

    xform1.eDy = (FLOAT)-4294967167.999999761;
    ok(xform1.eDy == -4294967040.0, "float rounding error.\n");
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);

    xform1.eDy = (FLOAT)-4294967167.999999762;
    ok(xform1.eDy == -4294967296.0, "float rounding error.\n");
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 0);
    ok_int(GetLastError(), ERROR_SUCCESS);

    xform2.eDy = 10000;
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);

    set_xform(&xform1, 1000.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    xform1.eDx = (FLOAT)-4294967167.999999762;
    xform2.eM11 = 1000.0;
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 0);
    ok_int(GetLastError(), ERROR_SUCCESS);

    xform1.eDx = 100000.0;
    xform2.eM11 = 100000.0;
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 0);
    ok_int(GetLastError(), ERROR_SUCCESS);

    /* Some undefined values */
    set_xform(&xform1, geIND, 0.0, 0.0, geINF, 0.0, 0.0);
    xform2 = xform1;
    SetLastError(ERROR_SUCCESS);
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, geIND, 0.0, 0.0, geINF, 0.0, 0.0);
    ok_int(GetLastError(), ERROR_SUCCESS);

    set_xform(&xform2, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0);
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, geIND, geIND, geINF, geINF, 0.0, 0.0);
    ok_int(GetLastError(), ERROR_SUCCESS);

    set_xform(&xform1, (FLOAT)18446743500000000000.0, 0.0, 1.0, 0.0, 0.0, 0.0);
    xform2 = xform1;
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_flt(xform3.eM11, 340282326356119260000000000000000000000.0);

    xform1.eM11 = (FLOAT)18446745000000000000.0;
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_flt(xform3.eM11, 340282346638528860000000000000000000000.0);

    xform1.eM11 = (FLOAT)18446746000000000000.0;
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_long(*(DWORD*)&xform3.eM11, 0x7f800000);

    /* zero matrix + 1 invalid */
    set_xform(&xform1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    set_xform(&xform2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    *(DWORD*)&xform2.eM22 = 0x7f800000; // (0.0F/0.0F)
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, 0.0, 0.0, 0.0, geIND, 0.0, 0.0);

    /* zero matrix + 1 invalid */
    xform2 = xform1;
    *(DWORD*)&xform2.eM12 = 0x7f800000; // (0.0F/0.0F)
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, 0.0, geIND, 0.0, geIND, 0.0, 0.0);

    /* Some undefined values */
    set_xform(&xform1, 0.0, geIND, 0.0, 0.0, 0.0, 0.0);
    set_xform(&xform2, geIND, 0.0, 0.0, geINF, 0.0, 0.0);
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, geIND, geIND, geIND, geIND, 0.0, 0.0);

}

void Test_CombineTransform_Inval(float eInval, float eOut)
{
    XFORM xform1, xform2, xform3;

    /* zero matrix / M11 invalid */
    set_xform(&xform1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    set_xform(&xform2, eInval, 0.0, 0.0, 0.0, 0.0, 0.0);
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, eOut, 0.0, 0.0, 0.0, 0.0, 0.0); // -> M21
    ok_int(CombineTransform(&xform3, &xform2, &xform1), 1);
    ok_xform(xform3, eOut, 0.0, 0.0, 0.0, 0.0, 0.0); // -> M12

    /* zero matrix / M12 invalid */
    set_xform(&xform1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    set_xform(&xform2, 0.0, eInval, 0.0, 0.0, 0.0, 0.0);
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, 0.0, eOut, 0.0, eOut, 0.0, 0.0);
    ok_int(CombineTransform(&xform3, &xform2, &xform1), 1);
    ok_xform(xform3, eOut, eOut, 0.0, 0.0, 0.0, 0.0);

    /* zero matrix / M21 invalid */
    set_xform(&xform1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    set_xform(&xform2, 0.0, 0.0, eInval, 0.0, 0.0, 0.0);
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, eOut, 0.0, eOut, 0.0, 0.0, 0.0);
    ok_int(CombineTransform(&xform3, &xform2, &xform1), 1);
    ok_xform(xform3, 0.0, 0.0, eOut, eOut, 0.0, 0.0);

    /* zero matrix / M22 invalid */
    set_xform(&xform1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    set_xform(&xform2, 0.0, 0.0, 0.0, eInval, 0.0, 0.0);
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, 0.0, 0.0, 0.0, eOut, 0.0, 0.0); // -> M12
    ok_int(CombineTransform(&xform3, &xform2, &xform1), 1);
    ok_xform(xform3, 0.0, 0.0, 0.0, eOut, 0.0, 0.0); // -> M21

    /* zero matrix / M11,M12 invalid */
    set_xform(&xform1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    set_xform(&xform2, eInval, eInval, 0.0, 0.0, 0.0, 0.0);
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, eOut, eOut, eOut, eOut, 0.0, 0.0);
    ok_int(CombineTransform(&xform3, &xform2, &xform1), 1);
    ok_xform(xform3, eOut, eOut, 0.0, 0.0, 0.0, 0.0);

    /* zero matrix / M11,M21 invalid */
    set_xform(&xform1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    set_xform(&xform2, eInval, 0.0, eInval, 0.0, 0.0, 0.0);
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, eOut, 0.0, eOut, 0.0, 0.0, 0.0);
    ok_int(CombineTransform(&xform3, &xform2, &xform1), 1);
    ok_xform(xform3, eOut, eOut, eOut, eOut, 0.0, 0.0);

    /* zero matrix / M11,M22 invalid */
    set_xform(&xform1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    set_xform(&xform2, eInval, 0.0, 0.0, eInval, 0.0, 0.0);
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, eOut, 0.0, 0.0, eOut, 0.0, 0.0); // -> M12, M21
    ok_int(CombineTransform(&xform3, &xform2, &xform1), 1);
    ok_xform(xform3, eOut, 0.0, 0.0, eOut, 0.0, 0.0);

    /* zero matrix / M12,M21 invalid */
    set_xform(&xform1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    set_xform(&xform2, 0.0, eInval, eInval, 0.0, 0.0, 0.0);
    ok_int(CombineTransform(&xform3, &xform1, &xform2), 1);
    ok_xform(xform3, eOut, eOut, eOut, eOut, 0.0, 0.0);
    ok_int(CombineTransform(&xform3, &xform2, &xform1), 1);
    ok_xform(xform3, eOut, eOut, eOut, eOut, 0.0, 0.0);
}

START_TEST(CombineTransform)
{
    *(DWORD*)&geINF = 0x7f800000;
    *(DWORD*)&geIND = 0xffc00000;
    *(DWORD*)&geQNAN = 0x7fc00000;

    Test_CombineTransform();

    Test_CombineTransform_Inval(geINF, geIND);
    Test_CombineTransform_Inval(geIND, geIND);
    Test_CombineTransform_Inval(geQNAN, geQNAN);

}

