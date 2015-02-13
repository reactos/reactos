
#include <stdio.h>
#include <windef.h>
#include <wingdi.h>
#include "init.h"

HBITMAP ghbmp1, ghbmp4, ghbmp8, ghbmp16, ghbmp24, ghbmp32;
HDC ghdcBmp1, ghdcBmp4, ghdcBmp8, ghdcBmp16, ghdcBmp24, ghdcBmp32;
HBITMAP ghbmpDIB1, ghbmpDIB4, ghbmpDIB8, ghbmpDIB16, ghbmpDIB24, ghbmpDIB32;
HDC ghdcDIB1, ghdcDIB4, ghdcDIB8, ghdcDIB16, ghdcDIB24, ghdcDIB32;
PVOID gpvDIB1, gpvDIB4, gpvDIB8, gpvDIB16, gpvDIB24, gpvDIB32;
PULONG pulDIB32Bits;
PULONG pulDIB4Bits;
HPALETTE ghpal;

MYPAL gpal =
{
    0x300, 8,
    {
        { 0x10, 0x20, 0x30, PC_NOCOLLAPSE },
        { 0x20, 0x30, 0x40, PC_NOCOLLAPSE },
        { 0x30, 0x40, 0x50, PC_NOCOLLAPSE },
        { 0x40, 0x50, 0x60, PC_NOCOLLAPSE },
        { 0x50, 0x60, 0x70, PC_NOCOLLAPSE },
        { 0x60, 0x70, 0x80, PC_NOCOLLAPSE },
        { 0x70, 0x80, 0x90, PC_NOCOLLAPSE },
        { 0x80, 0x90, 0xA0, PC_NOCOLLAPSE },
    }
};

BOOL
InitPerBitDepth(
    _In_ ULONG cBitsPerPixel,
    _In_ ULONG cx,
    _In_ ULONG cy,
    _Out_ HDC *phdcBmp,
    _Out_ HBITMAP *phbmp,
    _Out_ HDC *phdcDIB,
    _Out_ HBITMAP *phbmpDIB,
    _Out_ PVOID *ppvBits)
{
    struct
    {
        BITMAPINFOHEADER bmiHeader;
        RGBQUAD bmiColors[256];
    } bmiBuffer;
    LPBITMAPINFO pbmi = (LPBITMAPINFO)&bmiBuffer;

    /* Create a compatible DC for the bitmap */
    *phdcBmp = CreateCompatibleDC(0);
    if (*phdcBmp == NULL)
    {
        printf("CreateCompatibleDC failed for %lu bpp\n", cBitsPerPixel);
        return FALSE;
    }

    /* Create a bitmap */
    *phbmp = CreateBitmap(cx, cy, 1, cBitsPerPixel, NULL);
    if (*phbmp == NULL)
    {
        printf("CreateBitmap failed %lu\n", cBitsPerPixel);
        return FALSE;
    }

    SelectObject(*phdcBmp, *phbmp);

    /* Get info about the bitmap */
    memset(&bmiBuffer, 0, sizeof(bmiBuffer));
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    if (!GetDIBits(*phdcBmp, *phbmp, 0, 1, NULL, pbmi, DIB_RGB_COLORS))
    {
        printf("GetDIBits failed %lu\n", cBitsPerPixel);
        return FALSE;
    }

    /* Create a compatible DC for the DIB */
    *phdcDIB = CreateCompatibleDC(0);
    if (*phdcDIB == NULL)
    {
        printf("CreateCompatibleDC failed %lu\n", cBitsPerPixel);
        return FALSE;
    }

    pbmi->bmiHeader.biCompression = BI_RGB;
    pbmi->bmiHeader.biHeight = -pbmi->bmiHeader.biHeight;

    /* Create the DIB section with the same values */
    *phbmpDIB = CreateDIBSection(*phdcDIB, pbmi, DIB_RGB_COLORS, ppvBits, 0, 0 );
    if (*phbmpDIB == NULL)
    {
        printf("CreateDIBSection failed. %lu\n", cBitsPerPixel);
        return FALSE;
    }

    SelectObject(*phdcDIB, *phbmpDIB);

    return TRUE;
}

BOOL InitStuff(void)
{

    /* Initialize a logical palette */
    ghpal = CreatePalette((LOGPALETTE*)&gpal);
    if (!ghpal)
    {
        printf("failed to create a palette \n");
        return FALSE;
    }

    if (!InitPerBitDepth(1, 9, 9, &ghdcBmp1, &ghbmp1, &ghdcDIB1, &ghbmpDIB1, &gpvDIB1) ||
        !InitPerBitDepth(4, 5, 5, &ghdcBmp4, &ghbmp4, &ghdcDIB4, &ghbmpDIB4, &gpvDIB4) ||
        !InitPerBitDepth(8, 5, 5, &ghdcBmp8, &ghbmp8, &ghdcDIB8, &ghbmpDIB8, &gpvDIB8) ||
        !InitPerBitDepth(16, 5, 5, &ghdcBmp16, &ghbmp16, &ghdcDIB16, &ghbmpDIB16, &gpvDIB16) ||
        !InitPerBitDepth(24, 5, 5, &ghdcBmp24, &ghbmp24, &ghdcDIB24, &ghbmpDIB24, &gpvDIB24) ||
        !InitPerBitDepth(32, 4, 4, &ghdcBmp32, &ghbmp32, &ghdcDIB32, &ghbmpDIB32, &gpvDIB32))
    {
        printf("failed to create objects \n");
        return FALSE;
    }

    pulDIB32Bits = gpvDIB32;
    pulDIB4Bits = gpvDIB4;

    return TRUE;
}
