/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for NtGdiTransformPoints
 * COPYRIGHT:   Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <win32nt.h>

START_TEST(NtGdiTransformPoints)
{
    HDC hDC;
    POINT apt1[3], apt2[3];
    BOOL ret;
    SIZE siz;

    /* NULL HDC */

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints(NULL, NULL, NULL, 0, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints(NULL, NULL, NULL, 1, GdiDpToLp);
    ok_int(ret, FALSE);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints(NULL, apt1, NULL, 1, GdiDpToLp);
    ok_int(ret, FALSE);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints(NULL, NULL, apt2, 1, GdiDpToLp);
    ok_int(ret, FALSE);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints(NULL, apt1, apt1, 0, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints(NULL, apt1, apt2, 0, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints(NULL, apt1, apt2, 1, GdiDpToLp);
    ok_int(ret, FALSE);
    ok_err(0xDEADBEEF);

    /* (HDC)1 */

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints((HDC)1, NULL, NULL, 0, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints((HDC)1, NULL, NULL, 1, GdiDpToLp);
    ok_int(ret, FALSE);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints((HDC)1, apt1, NULL, 1, GdiDpToLp);
    ok_int(ret, FALSE);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints((HDC)1, NULL, apt2, 1, GdiDpToLp);
    ok_int(ret, FALSE);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints((HDC)1, apt1, apt1, 0, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints((HDC)1, apt1, apt2, 0, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints((HDC)1, apt1, apt2, 1, GdiDpToLp);
    ok_int(ret, FALSE);
    ok_err(0xDEADBEEF);

    /* hDC */

    hDC = CreateCompatibleDC(NULL);
    ok(hDC != NULL, "hDC was NULL\n");

    SetMapMode(hDC, MM_TEXT);

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints(hDC, NULL, NULL, 0, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints(hDC, NULL, NULL, 1, GdiDpToLp);
    ok_int(ret, FALSE);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints(hDC, apt1, NULL, 1, GdiDpToLp);
    ok_int(ret, FALSE);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints(hDC, NULL, apt2, 1, GdiDpToLp);
    ok_int(ret, FALSE);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints(hDC, apt1, apt1, 0, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    ret = NtGdiTransformPoints(hDC, apt1, apt2, 0, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);

    SetLastError(0xDEADBEEF);
    apt1[0].x = apt1[0].y = 256;
    ret = NtGdiTransformPoints(hDC, apt1, apt1, 1, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);
    ok_long(apt1[0].x, 256);
    ok_long(apt1[0].y, 256);

    SetLastError(0xDEADBEEF);
    apt1[0].x = apt1[0].y = 256;
    apt2[0].x = apt2[0].y = 0xBEEFDEAD;
    ret = NtGdiTransformPoints(hDC, apt1, apt2, 0, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);
    ok_long(apt1[0].x, 256);
    ok_long(apt1[0].y, 256);
    ok_long(apt2[0].x, 0xBEEFDEAD);
    ok_long(apt2[0].x, 0xBEEFDEAD);

    SetLastError(0xDEADBEEF);
    apt1[0].x = apt1[0].y = 256;
    apt2[0].x = apt2[0].y = 0xBEEFDEAD;
    ret = NtGdiTransformPoints(hDC, apt1, apt2, 1, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);
    ok_long(apt1[0].x, 256);
    ok_long(apt1[0].y, 256);
    ok_long(apt2[0].x, 256);
    ok_long(apt2[0].x, 256);

    /* MM_ISOTROPIC */

    SetMapMode(hDC, MM_ISOTROPIC);
    ok_int(SetWindowExtEx(hDC, 10, 10, NULL), TRUE);
    ok_int(GetWindowExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 10);
    ok_long(siz.cy, 10);
    ok_int(SetViewportExtEx(hDC, 100, 100, NULL), TRUE);
    ok_int(GetViewportExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 100);
    ok_long(siz.cy, 100);
    SetLastError(0xDEADBEEF);
    apt1[0].x = apt1[0].y = 256;
    apt2[0].x = apt2[0].y = 0xBEEFDEAD;
    ret = NtGdiTransformPoints(hDC, apt1, apt2, 1, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);
    ok_long(apt1[0].x, 256);
    ok_long(apt1[0].y, 256);
    ok_long(apt2[0].x, 26);
    ok_long(apt2[0].x, 26);

    SetMapMode(hDC, MM_ISOTROPIC);
    ok_int(SetWindowExtEx(hDC, 10, 10, NULL), TRUE);
    ok_int(GetWindowExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 10);
    ok_long(siz.cy, 10);
    ok_int(SetViewportExtEx(hDC, 20, 100, NULL), TRUE);
    ok_int(GetViewportExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 20);
    ok_long(siz.cy, 20);
    SetLastError(0xDEADBEEF);
    apt1[0].x = apt1[0].y = 256;
    apt2[0].x = apt2[0].y = 0xBEEFDEAD;
    ret = NtGdiTransformPoints(hDC, apt1, apt2, 1, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);
    ok_long(apt1[0].x, 256);
    ok_long(apt1[0].y, 256);
    ok_long(apt2[0].x, 128);
    ok_long(apt2[0].x, 128);

    SetMapMode(hDC, MM_ISOTROPIC);
    ok_int(SetWindowExtEx(hDC, 10, 10, NULL), TRUE);
    ok_int(GetWindowExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 10);
    ok_long(siz.cy, 10);
    ok_int(SetViewportExtEx(hDC, 100, 0, NULL), TRUE);
    ok_int(GetViewportExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 20);
    ok_long(siz.cy, 20);
    SetLastError(0xDEADBEEF);
    apt1[0].x = apt1[0].y = 256;
    apt2[0].x = apt2[0].y = 0xBEEFDEAD;
    ret = NtGdiTransformPoints(hDC, apt1, apt2, 1, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);
    ok_long(apt1[0].x, 256);
    ok_long(apt1[0].y, 256);
    ok_long(apt2[0].x, 128);
    ok_long(apt2[0].x, 128);

    SetMapMode(hDC, MM_ISOTROPIC);
    ok_int(SetWindowExtEx(hDC, 10, 10, NULL), TRUE);
    ok_int(GetWindowExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 10);
    ok_long(siz.cy, 10);
    ok_int(SetViewportExtEx(hDC, 0, 100, NULL), TRUE);
    ok_int(GetViewportExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 20);
    ok_long(siz.cy, 20);
    SetLastError(0xDEADBEEF);
    apt1[0].x = apt1[0].y = 256;
    apt2[0].x = apt2[0].y = 0xBEEFDEAD;
    ret = NtGdiTransformPoints(hDC, apt1, apt2, 1, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);
    ok_long(apt1[0].x, 256);
    ok_long(apt1[0].y, 256);
    ok_long(apt2[0].x, 128);
    ok_long(apt2[0].x, 128);

    SetMapMode(hDC, MM_ISOTROPIC);
    ok_int(SetWindowExtEx(hDC, 10, 10, NULL), TRUE);
    ok_int(GetWindowExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 10);
    ok_long(siz.cy, 10);
    ok_int(SetViewportExtEx(hDC, 0, 0, NULL), TRUE);
    ok_int(GetViewportExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 20);
    ok_long(siz.cy, 20);
    SetLastError(0xDEADBEEF);
    apt1[0].x = apt1[0].y = 256;
    apt2[0].x = apt2[0].y = 0xBEEFDEAD;
    ret = NtGdiTransformPoints(hDC, apt1, apt2, 1, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);
    ok_long(apt1[0].x, 256);
    ok_long(apt1[0].y, 256);
    ok_long(apt2[0].x, 128);
    ok_long(apt2[0].x, 128);

    /* MM_ANISOTROPIC */

    SetMapMode(hDC, MM_ANISOTROPIC);
    ok_int(SetWindowExtEx(hDC, 10, 10, NULL), TRUE);
    ok_int(GetWindowExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 10);
    ok_long(siz.cy, 10);
    ok_int(SetViewportExtEx(hDC, 100, 100, NULL), TRUE);
    ok_int(GetViewportExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 100);
    ok_long(siz.cy, 100);
    SetLastError(0xDEADBEEF);
    apt1[0].x = apt1[0].y = 256;
    apt2[0].x = apt2[0].y = 0xBEEFDEAD;
    ret = NtGdiTransformPoints(hDC, apt1, apt2, 1, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);
    ok_long(apt1[0].x, 256);
    ok_long(apt1[0].y, 256);
    ok_long(apt2[0].x, 26);
    ok_long(apt2[0].x, 26);

    SetMapMode(hDC, MM_ANISOTROPIC);
    ok_int(SetWindowExtEx(hDC, 10, 10, NULL), TRUE);
    ok_int(GetWindowExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 10);
    ok_long(siz.cy, 10);
    ok_int(SetViewportExtEx(hDC, 20, 100, NULL), TRUE);
    ok_int(GetViewportExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 20);
    ok_long(siz.cy, 100);
    SetLastError(0xDEADBEEF);
    apt1[0].x = apt1[0].y = 256;
    apt2[0].x = apt2[0].y = 0xBEEFDEAD;
    ret = NtGdiTransformPoints(hDC, apt1, apt2, 1, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);
    ok_long(apt1[0].x, 256);
    ok_long(apt1[0].y, 256);
    ok_long(apt2[0].x, 128);
    ok_long(apt2[0].x, 128);

    SetMapMode(hDC, MM_ANISOTROPIC);
    ok_int(SetWindowExtEx(hDC, 10, 10, NULL), TRUE);
    ok_int(GetWindowExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 10);
    ok_long(siz.cy, 10);
    ok_int(SetViewportExtEx(hDC, 100, 0, NULL), TRUE);
    ok_int(GetViewportExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 20);
    ok_long(siz.cy, 100);
    SetLastError(0xDEADBEEF);
    apt1[0].x = apt1[0].y = 256;
    apt2[0].x = apt2[0].y = 0xBEEFDEAD;
    ret = NtGdiTransformPoints(hDC, apt1, apt2, 1, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);
    ok_long(apt1[0].x, 256);
    ok_long(apt1[0].y, 256);
    ok_long(apt2[0].x, 128);
    ok_long(apt2[0].x, 128);

    SetMapMode(hDC, MM_ANISOTROPIC);
    ok_int(SetWindowExtEx(hDC, 10, 10, NULL), TRUE);
    ok_int(GetWindowExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 10);
    ok_long(siz.cy, 10);
    ok_int(SetViewportExtEx(hDC, 0, 100, NULL), TRUE);
    ok_int(GetViewportExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 20);
    ok_long(siz.cy, 100);
    SetLastError(0xDEADBEEF);
    apt1[0].x = apt1[0].y = 256;
    apt2[0].x = apt2[0].y = 0xBEEFDEAD;
    ret = NtGdiTransformPoints(hDC, apt1, apt2, 1, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);
    ok_long(apt1[0].x, 256);
    ok_long(apt1[0].y, 256);
    ok_long(apt2[0].x, 128);
    ok_long(apt2[0].x, 128);

    SetMapMode(hDC, MM_ANISOTROPIC);
    ok_int(SetWindowExtEx(hDC, 10, 10, NULL), TRUE);
    ok_int(GetWindowExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 10);
    ok_long(siz.cy, 10);
    ok_int(SetViewportExtEx(hDC, 0, 0, NULL), TRUE);
    ok_int(GetViewportExtEx(hDC, &siz), TRUE);
    ok_long(siz.cx, 20);
    ok_long(siz.cy, 100);
    SetLastError(0xDEADBEEF);
    apt1[0].x = apt1[0].y = 256;
    apt2[0].x = apt2[0].y = 0xBEEFDEAD;
    ret = NtGdiTransformPoints(hDC, apt1, apt2, 1, GdiDpToLp);
    ok_int(ret, TRUE);
    ok_err(0xDEADBEEF);
    ok_long(apt1[0].x, 256);
    ok_long(apt1[0].y, 256);
    ok_long(apt2[0].x, 128);
    ok_long(apt2[0].x, 128);

    ret = DeleteDC(hDC);
    ok_int(ret, TRUE);
}
