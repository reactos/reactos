
#include "precomp.h"

#include "init.h"

HBITMAP ghbmp1, ghbmp4, ghbmp8, ghbmp16, ghbmp24, ghbmp32;
HBITMAP ghbmpDIB1, ghbmpDIB4, ghbmpDIB8, ghbmpDIB16, ghbmpDIB24, ghbmpDIB32;
HDC ghdcDIB1, ghdcDIB4, ghdcDIB8, ghdcDIB16, ghdcDIB24, ghdcDIB32;
PVOID gpvDIB1, gpvDIB4, gpvDIB8, gpvDIB16, gpvDIB24, gpvDIB32;
ULONG (*gpDIB32)[8][8];
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
    _Out_ HBITMAP *phbmp,
    _Out_ HDC *phdcDIB,
    _Out_ HBITMAP *phbmpDIB,
    _Out_ PVOID *ppvBits)
{
    struct
    {
        BITMAPINFOHEADER bmiHeader;
        ULONG bmiColors[256];
    } bmiBuffer;
    LPBITMAPINFO pbmi = (LPBITMAPINFO)&bmiBuffer;

    /* Create a bitmap */
    *phbmp = CreateBitmap(cx, cy, 1, cBitsPerPixel, NULL);
    if (*phbmp == NULL)
    {
        printf("CreateBitmap failed %lu\n", cBitsPerPixel);
        return FALSE;
    }

    /* Setup bitmap info */
    memset(&bmiBuffer, 0, sizeof(bmiBuffer));
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth = cx;
    pbmi->bmiHeader.biHeight = -(LONG)cy;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biBitCount = cBitsPerPixel;
    pbmi->bmiHeader.biCompression = BI_RGB;
    pbmi->bmiHeader.biSizeImage = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed = 0;
    pbmi->bmiHeader.biClrImportant = 0;

    if (cBitsPerPixel == 1)
    {
        bmiBuffer.bmiColors[0] = 0;
        bmiBuffer.bmiColors[1] = 0xFFFFFF;
        pbmi->bmiHeader.biClrUsed = 2;
    }

    /* Create a compatible DC for the DIB */
    *phdcDIB = CreateCompatibleDC(0);
    if (*phdcDIB == NULL)
    {
        printf("CreateCompatibleDC failed %lu\n", cBitsPerPixel);
        return FALSE;
    }

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
        printf("failed to create a palette\n");
        return FALSE;
    }

    if (!InitPerBitDepth(1, 9, 9, &ghbmp1, &ghdcDIB1, &ghbmpDIB1, &gpvDIB1) ||
        !InitPerBitDepth(4, 5, 5, &ghbmp4, &ghdcDIB4, &ghbmpDIB4, &gpvDIB4) ||
        !InitPerBitDepth(8, 5, 5, &ghbmp8, &ghdcDIB8, &ghbmpDIB8, &gpvDIB8) ||
        !InitPerBitDepth(16, 8, 8, &ghbmp16, &ghdcDIB16, &ghbmpDIB16, &gpvDIB16) ||
        !InitPerBitDepth(24, 8, 8, &ghbmp24, &ghdcDIB24, &ghbmpDIB24, &gpvDIB24) ||
        !InitPerBitDepth(32, 8, 8, &ghbmp32, &ghdcDIB32, &ghbmpDIB32, &gpvDIB32))
    {
        printf("failed to create objects\n");
        return FALSE;
    }

    gpDIB32 = gpvDIB32;

    return TRUE;
}
