
#include <stdio.h>
#include <windef.h>
#include <wingdi.h>

HBITMAP ghbmpDIB32;
HDC ghdcDIB32;
PULONG pulDIB32Bits;
HBITMAP ghbmpDIB4;
HDC ghdcDIB4;
PULONG pulDIB4Bits;
HPALETTE ghpal;

struct
{
    WORD palVersion;
    WORD palNumEntries;
    PALETTEENTRY logpalettedata[8];
} gpal =
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

BOOL InitStuff(void)
{
    BITMAPINFO bmi32 =
        {{sizeof(BITMAPINFOHEADER), 4, -4, 1, 32, BI_RGB, 0, 1, 1, 0, 0}};
    BITMAPINFO bmi4 =
        {{sizeof(BITMAPINFOHEADER), 4, -4, 1, 4, BI_RGB, 0, 1, 1, 0, 0}};

    ghdcDIB32 = CreateCompatibleDC(0);
    ghdcDIB4 = CreateCompatibleDC(0);

    ghbmpDIB32 = CreateDIBSection(ghdcDIB32, &bmi32, DIB_PAL_COLORS, (PVOID*)&pulDIB32Bits, 0, 0 );
    if (!ghbmpDIB32) return FALSE;

    ghbmpDIB4 = CreateDIBSection(ghdcDIB4, &bmi4, DIB_PAL_COLORS, (PVOID*)&pulDIB4Bits, 0, 0 );
    if (!ghbmpDIB32) return FALSE;

    SelectObject(ghdcDIB32, ghbmpDIB32);

    /* Initialize a logical palette */
    ghpal = CreatePalette((LOGPALETTE*)&gpal);
    if (!ghpal)
    {
        printf("failed to create a palette \n");
        return FALSE;
    }

    return TRUE;
}
