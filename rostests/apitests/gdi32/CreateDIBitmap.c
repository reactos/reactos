/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for CreateDIBitmap
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <apitest.h>

#include <wingdi.h>
#include <winuser.h>

#include "init.h"

#define CBM_CREATDIB 2

BOOL
GetExpected(
    DWORD *pdwError,
    HDC hdc,
    const BITMAPINFOHEADER *lpbmih,
    DWORD fdwInit,
    const VOID *lpbInit,
    const BITMAPINFO *lpbmi,
    UINT fuUsage)
{
    if (fuUsage > 2)
    {
        *pdwError = ERROR_INVALID_PARAMETER;
        return FALSE;
    }

    if (fuUsage != DIB_RGB_COLORS)
    {
        if (hdc == (HDC)-1)
        {
            return FALSE;
        }
    }

    if (fdwInit & CBM_INIT)
    {
        if (!lpbmih)
        {
            if (!lpbInit || (lpbInit == (PVOID)0xC0000000)) return FALSE;
        }
        else
        {
            if (lpbInit)
            {
                if (lpbInit == (PVOID)0xC0000000) return FALSE;
                if (!lpbmi || (lpbmi == (PVOID)0xC0000000)) return FALSE;
                if (lpbmi->bmiHeader.biSize == 0) return FALSE;
                if (fuUsage == 2) return FALSE;
            }
        }
    }


    if (fdwInit & CBM_CREATDIB)
    {
        if (fuUsage == 2) return FALSE;
        if ((fuUsage == DIB_PAL_COLORS) && !hdc)
        {
            return FALSE;
        }

        if (fdwInit & CBM_INIT)
        {
            if (!lpbInit || (lpbInit == (PVOID)0xC0000000)) return FALSE;
        }

        if ((!lpbmi) || (lpbmi == (PVOID)0xc0000000) || (lpbmi->bmiHeader.biSize == 0))
        {
            return FALSE;
        }
    }
    else
    {

        if ((lpbmih == NULL) ||
            (lpbmih == (PVOID)0xC0000000) ||
            (lpbmih->biSize == 0))
        {
            return FALSE;
        }

        if (hdc == (HDC)-1)
        {
            *pdwError = ERROR_INVALID_PARAMETER;
            return FALSE;
        }


        if (lpbmi == (PVOID)0xc0000000) return FALSE;
    }

    return TRUE;
}


void
Test_CreateDIBitmap_Params(void)
{
    HBITMAP hbmp;
    HDC hdc;
    BITMAPINFO bmi =
        {{sizeof(BITMAPINFOHEADER), 4, 4, 1, 8, BI_RGB, 0, 1, 1, 1, 0}, {{0,0,0,0}}};
    BITMAPINFO bmiBroken =
        {{0, -2, -4, 55, 42, 21, 0, 1, 1, 1, 0}, {{0,0,0,0}}};
    BYTE ajBits[10];

    hdc = CreateCompatibleDC(0);
    ok(hdc != 0, "failed\n");

    hbmp = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, ajBits, &bmi, DIB_PAL_COLORS);
    ok(hbmp != 0, "\n");

    hbmp = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, NULL, &bmi, DIB_PAL_COLORS);
    ok(hbmp != 0, "\n");

    hbmp = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, NULL, NULL, DIB_PAL_COLORS);
    ok(hbmp != 0, "\n");

    hbmp = CreateDIBitmap(hdc, &bmi.bmiHeader, 0, (PVOID)0xc0000000, &bmi, DIB_PAL_COLORS);
    ok(hbmp != 0, "\n");

    hbmp = CreateDIBitmap(NULL, &bmi.bmiHeader, CBM_INIT, NULL, &bmi, DIB_PAL_COLORS);
    ok(hbmp != 0, "\n");

    hbmp = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, NULL, &bmiBroken, DIB_PAL_COLORS);
    ok(hbmp != 0, "\n");

    hbmp = CreateDIBitmap(NULL, NULL, 2, NULL, &bmi, 0);
    ok(hbmp != 0, "\n");

    SetLastError(0xbadbad00);
    hbmp = CreateDIBitmap(hdc, NULL, CBM_INIT, ajBits, &bmi, DIB_PAL_COLORS);
    ok(hbmp == 0, "\n");
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xbadbad00);
    hbmp = CreateDIBitmap(hdc, NULL, CBM_INIT, NULL, &bmiBroken, DIB_PAL_COLORS);
    ok(hbmp == 0, "\n");
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xbadbad00);
    hbmp = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, ajBits, &bmi, 2);
    ok(hbmp == 0, "\n");
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xbadbad00);
    hbmp = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, ajBits, NULL, DIB_PAL_COLORS);
    ok(hbmp == 0, "\n");
    ok_err(0xbadbad00);

    SetLastError(0xbadbad00);
    hbmp = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, (PVOID)0xc0000000, &bmi, DIB_PAL_COLORS);
    ok(hbmp == 0, "\n");
    ok_err(0xbadbad00);

    SetLastError(0xbadbad00);
    _SEH2_TRY
    {
        hbmp = CreateDIBitmap(hdc, &bmi.bmiHeader, 0, ajBits, (PVOID)0xc0000000, DIB_PAL_COLORS);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        hbmp = (HBITMAP)-1;
    }
    _SEH2_END;
    ok(hbmp == (HBITMAP)-1, "\n");
    ok_err(0xbadbad00);

    SetLastError(0xbadbad00);
    hbmp = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, NULL, NULL, 5);
    ok(hbmp == 0, "\n");
    ok_err(ERROR_INVALID_PARAMETER);

    SetLastError(0xbadbad00);
    hbmp = CreateDIBitmap((HDC)-1, &bmi.bmiHeader, CBM_INIT, NULL, &bmi, DIB_PAL_COLORS);
    ok(hbmp == 0, "\n");
    ok_err(0xbadbad00);

    SetLastError(0xbadbad00);
    hbmp = CreateDIBitmap(hdc, &bmiBroken.bmiHeader, CBM_INIT, NULL, &bmi, DIB_PAL_COLORS);
    ok(hbmp == 0, "\n");
    ok_err(ERROR_INVALID_PARAMETER);

    if (1)
    {
        ULONG i1, i2, i3, i4, i5, i6;
        HDC ahdc[3] = {0, hdc, (HDC)-1};
        PBITMAPINFOHEADER apbih[4] = {NULL, &bmi.bmiHeader, &bmiBroken.bmiHeader, (PVOID)0xC0000000};
        ULONG afInitf[12] = {0, 1, 2, 3, CBM_INIT, 4, 5, 6, 7, 8, 0x10, 0x20};
        PVOID apvBits[3] = {NULL, ajBits, (PVOID)0xc0000000};
        PBITMAPINFO apbmi[4] = {NULL, &bmi, &bmiBroken, (PVOID)0xC0000000};
        ULONG aiUsage[5] = {0, 1, 2, 3, 23};
        DWORD dwExpError;
        BOOL bExpSuccess;

        for (i1 = 0; i1 < 3; i1++)
        {
            for (i2 = 0; i2 < 4; i2++)
            {
                for (i3 = 0; i3 < 8; i3++)
                {
                    for (i4 = 0; i4 < 3; i4++)
                    {
                        for (i5 = 0; i5 < 4; i5++)
                        {
                            for (i6 = 0; i6 < 5; i6++)
                            {
                                SetLastError(0xbadbad00);
                                dwExpError = 0xbadbad00;

                                bExpSuccess = GetExpected(&dwExpError, ahdc[i1], apbih[i2], afInitf[i3], apvBits[i4], apbmi[i5], aiUsage[i6]);

                                _SEH2_TRY
                                {
                                    hbmp = CreateDIBitmap(ahdc[i1], apbih[i2], afInitf[i3], apvBits[i4], apbmi[i5], aiUsage[i6]);
                                }
                                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                                {
                                    hbmp = (HBITMAP)0;
                                }
                                _SEH2_END;

                                if (bExpSuccess)
                                {
                                    ok(hbmp != 0, "Expected success for (%ld,%ld,%ld,%ld,%ld,%ld) CreateDIBitmap(%p, %p, 0x%lx, %p, %p, %ld)\n",
                                       i1, i2, i3, i4, i5, i6,
                                       ahdc[i1], apbih[i2], afInitf[i3], apvBits[i4], apbmi[i5], aiUsage[i6]);
                                }
                                else
                                {
                                    ok(hbmp == 0, "Expected failure for (%ld,%ld,%ld,%ld,%ld,%ld) CreateDIBitmap(%p, %p, 0x%lx, %p, %p, %ld)\n",
                                       i1, i2, i3, i4, i5, i6,
                                       ahdc[i1], apbih[i2], afInitf[i3], apvBits[i4], apbmi[i5], aiUsage[i6]);
                                }

                              //  ok(GetLastError() == dwExpError, "Expected error %ld got %ld for (%ld,%ld,%ld,%ld,%ld,%ld) CreateDIBitmap(%p, %p, 0x%lx, %p, %p, %ld)\n",
                              //         dwExpError, GetLastError(), i1, i2, i3, i4, i5, i6,
                              //         ahdc[i1], apbih[i2], afInitf[i3], apvBits[i4], apbmi[i5], aiUsage[i6]);
                            }
                        }
                    }
                }
            }
        }
    }


}

void
Test_CreateDIBitmap_DIB_PAL_COLORS(void)
{
    struct
    {
        BITMAPINFOHEADER bmiHeader;
        WORD bmiColors[8];
    } bmibuffer;
    BITMAPINFO *pbmi = (PVOID)&bmibuffer;
    HBITMAP hbmp;
    ULONG bits[16] = {0};
    HDC hdc;
    HPALETTE hpalOld;
    USHORT i;

    hdc = CreateCompatibleDC(0);
    ok(hdc != 0, "failed\n");

    /* Select a palette */
    hpalOld = SelectPalette(hdc, ghpal, FALSE);
    ok(hpalOld != NULL, "error=%ld\n", GetLastError());

    /* Initialize a BITMAPINFO */
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = 2;
    pbmi->bmiHeader.biHeight = -2;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biBitCount = 8;
    pbmi->bmiHeader.biCompression = BI_RGB;
    pbmi->bmiHeader.biSizeImage = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 1;
    pbmi->bmiHeader.biYPelsPerMeter = 1;
    pbmi->bmiHeader.biClrUsed = 8;
    pbmi->bmiHeader.biClrImportant = 0;

    for( i = 0; i < 8; i++ )
    {
        bmibuffer.bmiColors[i] = i;
    }

    /* Create the bitmap */
    hbmp = CreateDIBitmap(hdc, &pbmi->bmiHeader, CBM_INIT, bits, pbmi, DIB_PAL_COLORS);
    ok(hbmp != 0, "failed\n");

    SelectObject(hdc, hbmp);


}

void
Test_CreateDIBitmap1(void)
{
    BITMAPINFO bmi;
    HBITMAP hbmp;
    BITMAP bitmap;
    ULONG bits[128] = {0};
    BYTE rlebits[] = {2, 0, 0, 0, 2, 1, 0, 1};
    HDC hdc;
    int ret;

    hdc = GetDC(0);

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 2;
    bmi.bmiHeader.biHeight = 2;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 16;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 1;
    bmi.bmiHeader.biYPelsPerMeter = 1;
    bmi.bmiHeader.biClrUsed = 0;
    bmi.bmiHeader.biClrImportant = 0;

    hbmp = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, bits, &bmi, DIB_RGB_COLORS);
    ok(hbmp != 0, "failed\n");

    ret = GetObject(hbmp, sizeof(bitmap), &bitmap);
    ok(ret != 0, "failed\n");
    ok(bitmap.bmType == 0, "\n");
    ok(bitmap.bmWidth == 2, "\n");
    ok(bitmap.bmHeight == 2, "\n");
    ok(bitmap.bmWidthBytes == 8, "bmWidthBytes = %ld\n", bitmap.bmWidthBytes);
    ok(bitmap.bmPlanes == 1, "\n");
    ok(bitmap.bmBitsPixel == GetDeviceCaps(hdc, BITSPIXEL), "\n");
    ok(bitmap.bmBits == 0, "\n");

    SetLastError(0);
    bmi.bmiHeader.biCompression = BI_RLE8;
    bmi.bmiHeader.biBitCount = 8;
    bmi.bmiHeader.biSizeImage = 8;
    bmi.bmiHeader.biClrUsed = 1;
    hbmp = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT, rlebits, &bmi, DIB_PAL_COLORS);
    ok(hbmp != 0, "failed\n");
    ok(GetLastError() == 0, "GetLastError() == %ld\n", GetLastError());

    ret = GetObject(hbmp, sizeof(bitmap), &bitmap);
    ok(ret != 0, "failed\n");
    ok(bitmap.bmType == 0, "\n");
    ok(bitmap.bmWidth == 2, "\n");
    ok(bitmap.bmHeight == 2, "\n");
    ok(bitmap.bmWidthBytes == 8, "bmWidthBytes = %ld\n", bitmap.bmWidthBytes);
    ok(bitmap.bmPlanes == 1, "\n");
    ok(bitmap.bmBitsPixel == GetDeviceCaps(hdc, BITSPIXEL), "\n");
    ok(bitmap.bmBits == 0, "\n");


}

void Test_CreateDIBitmap_RLE8()
{
    struct
    {
        BITMAPINFOHEADER bmiHeader;
        WORD wColors[4];
        BYTE ajBuffer[20];
    } PackedDIB =
    {
        {sizeof(BITMAPINFOHEADER), 4, 4, 1, 8, BI_RLE8, 20, 1, 1, 4, 0},
        {0, 1, 2, 7},
        {4,0,   0,2,0,1,0,2,3,1,   2,1, 2,2,   1,3,1,0,1,2, },
    };
    HDC hdc;
    HBITMAP hbmp;

    hdc = CreateCompatibleDC(0);

    SetLastError(0xbadbad00);
    hbmp = CreateDIBitmap(hdc, &PackedDIB.bmiHeader, CBM_INIT, &PackedDIB.ajBuffer, (PVOID)&PackedDIB, DIB_PAL_COLORS);
    ok(hbmp != 0, "CreateDIBitmap failed.\n");
    ok_err(0xbadbad00);
    DeleteObject(hbmp);

    PackedDIB.bmiHeader.biSizeImage = 2;
    hbmp = CreateDIBitmap(hdc, &PackedDIB.bmiHeader, CBM_INIT, &PackedDIB.ajBuffer, (PVOID)&PackedDIB, DIB_PAL_COLORS);
    ok(hbmp != 0, "CreateDIBitmap failed.\n");
    ok_err(0xbadbad00);
    DeleteObject(hbmp);

    PackedDIB.bmiHeader.biSizeImage = 1;
    hbmp = CreateDIBitmap(hdc, &PackedDIB.bmiHeader, CBM_INIT, &PackedDIB.ajBuffer, (PVOID)&PackedDIB, DIB_PAL_COLORS);
    ok(hbmp != 0, "CreateDIBitmap failed.\n");
    ok_err(0xbadbad00);
    DeleteObject(hbmp);

    PackedDIB.bmiHeader.biSizeImage = 0;
    hbmp = CreateDIBitmap(hdc, &PackedDIB.bmiHeader, CBM_INIT, &PackedDIB.ajBuffer, (PVOID)&PackedDIB, DIB_PAL_COLORS);
    ok(hbmp == 0, "CreateDIBitmap succeeded, expected failure\n");
    ok_err(0xbadbad00);

    /* Test a line that is too long */
    PackedDIB.bmiHeader.biSizeImage = 20;
    PackedDIB.ajBuffer[0] = 17;
    hbmp = CreateDIBitmap(hdc, &PackedDIB.bmiHeader, CBM_INIT, &PackedDIB.ajBuffer, (PVOID)&PackedDIB, DIB_PAL_COLORS);
    ok(hbmp != 0, "CreateDIBitmap failed\n");
    ok_err(0xbadbad00);
    DeleteObject(hbmp);


}

void
Test_CreateDIBitmap_CBM_CREATDIB(void)
{
    HBITMAP hbmp, hbmpOld;
    HDC hdc;
    BITMAPINFO bmi =
        {{sizeof(BITMAPINFOHEADER), 4, 4, 1, 8, BI_RGB, 0, 1, 1, 1, 0}, {{0,1,2,3}}};
    BYTE ajBits[10] = {0,1,2,3,4,5,6,7,8,9};
    BITMAP bitmap;
    struct
    {
        BITMAPINFOHEADER bmiHeader;
        WORD wColors[4];
    } bmiRLE =
    {
        {sizeof(BITMAPINFOHEADER), 8, 2, 1, 8, BI_RLE8, 20, 1, 1, 4, 0},
        {0, 1, 2, 7}
    };
    BYTE ajBitsRLE[] = {4,0,   0,2,0,1,0,2,3,1,   2,1, 2,2,   1,3,1,0,1,2, };

    hdc = CreateCompatibleDC(0);
    if (hdc == NULL)
    {
        ok(0, "CreateCompatibleDC failed. Skipping tests!\n");
        return;
    }

    SetLastError(0xbadbad00);
    hbmp = CreateDIBitmap(hdc, NULL, CBM_CREATDIB, ajBits, NULL, DIB_RGB_COLORS);
    ok(hbmp == 0, "CreateDIBitmap should fail.\n");
    ok_int(GetLastError(), 0xbadbad00);

    hbmp = CreateDIBitmap(hdc, NULL, CBM_CREATDIB, ajBits, &bmi, DIB_RGB_COLORS);
    ok(hbmp != 0, "CreateDIBitmap failed.\n");

    ok_long(GetObject(hbmp, sizeof(DIBSECTION), &bitmap), sizeof(BITMAP));
    ok_int(bitmap.bmType, 0);
    ok_int(bitmap.bmWidth, 4);
    ok_int(bitmap.bmHeight, 4);
    ok_int(bitmap.bmWidthBytes, 4);
    ok_int(bitmap.bmPlanes, 1);
    ok_int(bitmap.bmBitsPixel, 8);
    ok_ptr(bitmap.bmBits, 0);

    hbmpOld = SelectObject(hdc, hbmp);
    ok(hbmpOld != NULL, "Couldn't select the bitmap.\n");

    /* Copy it on a dib section */
    memset(gpDIB32, 0x77, sizeof(*gpDIB32));
    ok_long(BitBlt(ghdcDIB32, 0, 0, 4, 4, hdc, 0, 0, SRCCOPY), 1);
    ok_long((*gpDIB32)[0][0], 0x20100);
    ok_long((*gpDIB32)[0][1], 0x20100);
    ok_long((*gpDIB32)[0][2], 0x20100);
    ok_long((*gpDIB32)[0][3], 0x20100);

    SelectObject(hdc, hbmpOld);
    DeleteObject(hbmp);

    hbmp = CreateDIBitmap(hdc, NULL, CBM_CREATDIB | CBM_INIT, ajBits, &bmi, DIB_PAL_COLORS);
    ok(hbmp != 0, "CreateDIBitmap failed.\n");

    ok_long(GetObject(hbmp, sizeof(DIBSECTION), &bitmap), sizeof(BITMAP));
    ok_int(bitmap.bmType, 0);
    ok_int(bitmap.bmWidth, 4);
    ok_int(bitmap.bmHeight, 4);
    ok_int(bitmap.bmWidthBytes, 4);
    ok_int(bitmap.bmPlanes, 1);
    ok_int(bitmap.bmBitsPixel, 8);
    ok_ptr(bitmap.bmBits, 0);

    /* Even with CBM_INIT and lpbmih != 0, pbmi is used for the dimensions */
    hbmp = CreateDIBitmap(hdc, &bmiRLE.bmiHeader, CBM_CREATDIB | CBM_INIT, ajBits, &bmi, DIB_PAL_COLORS);
    ok(hbmp != 0, "CreateDIBitmap failed.\n");

    ok_long(GetObject(hbmp, sizeof(DIBSECTION), &bitmap), sizeof(BITMAP));
    ok_int(bitmap.bmType, 0);
    ok_int(bitmap.bmWidth, 4);
    ok_int(bitmap.bmHeight, 4);
    ok_int(bitmap.bmWidthBytes, 4);
    ok_int(bitmap.bmPlanes, 1);
    ok_int(bitmap.bmBitsPixel, 8);
    ok_ptr(bitmap.bmBits, 0);

    hbmpOld = SelectObject(hdc, hbmp);
    ok(hbmpOld != NULL, "Couldn't select the bitmap.\n");

    /* Copy it on a dib section */
    memset(gpDIB32, 0x77, sizeof(*gpDIB32));
    ok_long(BitBlt(ghdcDIB32, 0, 0, 4, 4, hdc, 0, 0, SRCCOPY), 1);
    ok_long((*gpDIB32)[0][0], 0);
    ok_long((*gpDIB32)[0][1], 0);
    ok_long((*gpDIB32)[0][2], 0);
    ok_long((*gpDIB32)[0][3], 0);

    SelectObject(hdc, hbmpOld);
    DeleteObject(hbmp);

    hbmp = CreateDIBitmap(hdc, NULL, CBM_CREATDIB, ajBitsRLE, (PVOID)&bmiRLE, DIB_PAL_COLORS);
    ok(hbmp == 0, "CreateDIBitmap should fail.\n");
    hbmp = CreateDIBitmap(hdc, NULL, CBM_INIT | CBM_CREATDIB, ajBitsRLE, (PVOID)&bmiRLE, DIB_PAL_COLORS);
    ok(hbmp == 0, "CreateDIBitmap should fail.\n");

    /* Check if a 0 pixel bitmap results in the DEFAULT_BITMAP being returned */
    bmi.bmiHeader.biWidth = 0;
    bmi.bmiHeader.biHeight = 4;
    hbmp = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_CREATDIB, ajBits, &bmi, DIB_PAL_COLORS);
    ok(hbmp == GetStockObject(21), "CreateDIBitmap didn't return the default bitmap.\n");
    bmi.bmiHeader.biWidth = 23;
    bmi.bmiHeader.biHeight = 0;
    hbmp = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_CREATDIB, ajBits, &bmi, DIB_PAL_COLORS);
    ok(hbmp == GetStockObject(21), "CreateDIBitmap didn't return the default bitmap.\n");

    DeleteDC(hdc);
}

START_TEST(CreateDIBitmap)
{
    InitStuff();
    Test_CreateDIBitmap_Params();
    Test_CreateDIBitmap1();
    Test_CreateDIBitmap_DIB_PAL_COLORS();
    Test_CreateDIBitmap_RLE8();
    Test_CreateDIBitmap_CBM_CREATDIB();
}

