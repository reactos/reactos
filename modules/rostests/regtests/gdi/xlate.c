
#include <stdio.h>
#include <windows.h>
#include <wine/test.h>

enum
{
    BMF_1BPP = 0,
    BMF_4BPP = 1,
    BMF_8BPP = 2,
    BMF_16BPP_555 = 3,
    BMF_16BPP_565 = 4,
    BMF_24BPP_RGB = 5,
    BMF_24BPP_BGR = 6,
    BMF_32BPP_RGB = 7,
    BMF_32BPP_BGR = 8
};

ULONG bpp[] = {1, 4, 8, 15, 16, 24, 24, 32, 32};

static BYTE ajBits1[] = {0xAA, 0xAA, 0xAA, 0xAA, 0,0,0,0};
static BYTE ajBits8[] = {0x00, 0xFF, 0x80, 0xCC, 0,0,0,0};
static WORD ajBits16[] = {0x0000, 0xFFFF, 0x1000, 0x0C0C, 0,0,0,0};
static DWORD ajBits24[] = {0x0000, 0xFFFF, 0x1000, 0x0C0C, 0,0,0,0};
static DWORD ajBits32[] = {0x0000, 0xFFFF, 0x1000, 0x0C0C, 0,0,0,0};
static HBITMAP hbmp1bpp_a, hbmp1bpp_b;
static HBITMAP hbmp8bpp_a, hbmp8bpp_b;
static HBITMAP hbmp16bpp_a, hbmp16bpp_b;
static HBITMAP hbmp24bpp_a, hbmp24bpp_b;
static HBITMAP hbmp32bpp_a, hbmp32bpp_b;
static HBITMAP hbmpCompat;
static HDC hdcSrc, hdcDst;
static ULONG iDcFormat;

ULONG
GetRealColorDepth()
{
    HBITMAP hbmp;
    HDC hdc;
    struct
    {
        BITMAPINFOHEADER bmiHeader;
        ULONG aulMasks[3];
    } bmi;
    PBITMAPINFO pbmi = (PBITMAPINFO)&bmi;
    ULONG ulColorDepth;

    /* Get the screen DC */
    hdc = GetDC(NULL);

    /* Create a compatible bitmap */
    hbmp = CreateCompatibleBitmap(hdc, 1, 1);

    /* Fill BITMAPINFOHEADER */
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    GetDIBits(hdc, hbmp, 0, 1, NULL, pbmi, DIB_RGB_COLORS);

    /* Call again to fill in the bitfields */
    GetDIBits(hdc, hbmp, 0, 1, NULL, pbmi, DIB_RGB_COLORS);

    /* Get the basic color depth */
    ulColorDepth = bmi.bmiHeader.biBitCount;

    DeleteObject(hbmp);

    switch (ulColorDepth)
    {
        case 1:
            return BMF_1BPP;
        case 4:
            return BMF_4BPP;
        case 8:
            return BMF_8BPP;
        case 16:
            /* Check the red mask */
            if (bmi.aulMasks[0] == 0x7c00)
                return BMF_16BPP_555;
            else
                return BMF_16BPP_565;
        case 24:
            return BMF_4BPP;
        case 32:
            if (bmi.bmiHeader.biCompression == BI_BITFIELDS &&
                bmi.aulMasks[0] == 0xff)
                return BMF_32BPP_RGB;
            else
                return BMF_32BPP_BGR;
    }

    /* Cleanup and return */
    return BMF_32BPP_RGB;
}

static
ULONG
iXlateFromRGB(ULONG iFormat, COLORREF crColor)
{
    ULONG ulRed, ulGreen, ulBlue;

    ulRed = GetRValue(crColor);
    ulGreen = GetGValue(crColor);
    ulBlue = GetBValue(crColor);

    switch (iFormat)
    {
        case BMF_1BPP:
            return crColor ? 0xffffff : 0;

        case BMF_4BPP:
        case BMF_8BPP:
        case BMF_16BPP_555:
            ulRed = (ulRed & 0xF8) >> 3;
            ulGreen = (ulGreen & 0xF8) >> 3;
            ulBlue = (ulBlue & 0xF8) >> 3;
            return ulRed << 10 | ulGreen << 5 | ulBlue;

        case BMF_16BPP_565:
            ulRed = (ulRed & 0xF8) >> 3;
            ulGreen = (ulGreen & 0xFC) >> 2;
            ulBlue = (ulBlue & 0xF8) >> 3;
            return ulRed << 11 | ulGreen << 5 | ulBlue;

        case BMF_24BPP_RGB:
        case BMF_32BPP_RGB:
            return crColor;

        case BMF_24BPP_BGR:
        case BMF_32BPP_BGR:
            return RGB(ulBlue, ulGreen, ulRed);
    }
    return 0;
}

static
COLORREF
iXlateToRGB(ULONG iFormat, ULONG ulColor)
{
    ULONG ulRed, ulGreen, ulBlue;

    switch (iFormat)
    {
        case BMF_1BPP:
            return ulColor ? 0xffffff : 0;

        case BMF_4BPP:
        case BMF_8BPP:
        case BMF_16BPP_555:
            ulRed = (ulColor & 0x7C00) >> 7;
            ulRed |= ulRed >> 5;
            ulGreen = (ulColor & 0x3E0) >> 2;
            ulGreen |= ulGreen >> 5;
            ulBlue = (ulColor & 0x1F) << 3;
            ulBlue |= ulBlue >> 5;
            return RGB(ulRed, ulGreen, ulBlue);

        case BMF_16BPP_565:
            ulRed = (ulColor & 0xF800) >> 8;
            ulRed |= ulRed >> 5;
            ulGreen = (ulColor & 0x7E0) >> 3;
            ulGreen |= ulGreen >> 6;
            ulBlue = (ulColor & 0x1F) << 3;
            ulBlue |= ulBlue >> 5;
            return RGB(ulRed, ulGreen, ulBlue);

        case BMF_24BPP_RGB:
        case BMF_32BPP_RGB:
            return ulColor;

        case BMF_24BPP_BGR:
        case BMF_32BPP_BGR:
            ulRed = GetRValue(ulColor);
            ulGreen = GetGValue(ulColor);
            ulBlue = GetBValue(ulColor);
            return RGB(ulBlue, ulGreen, ulRed);
    }
    return 0;
}

static
ULONG
GetClosestColor(ULONG iFormat, COLORREF crColor, COLORREF crBackColor)
{
    if (iFormat == BMF_1BPP)
        return crBackColor;
    return iXlateToRGB(iFormat, iXlateFromRGB(iFormat, crColor));
}

ULONG
GetDIBPixel(ULONG iFormat, PVOID pvBits, ULONG x)
{
    switch (iFormat)
    {
        case BMF_1BPP:
            //
        case BMF_16BPP_555:
        case BMF_16BPP_565:
            return *(WORD*)((PCHAR)pvBits + x * sizeof(WORD));

        case BMF_24BPP_RGB:
        case BMF_24BPP_BGR:
            return (*(DWORD*)((PCHAR)pvBits + x * 3)) & 0xffffff;

        case BMF_32BPP_RGB:
        case BMF_32BPP_BGR:
            return *(DWORD*)((PCHAR)pvBits + x * sizeof(DWORD));
    }
    return 0;
}

static
void
Initialize()
{
    hdcSrc = CreateCompatibleDC(0);
    hdcDst = CreateCompatibleDC(0);

    hbmpCompat = CreateCompatibleBitmap(GetDC(0), 4, 2);
    ok(hbmpCompat != 0, "CreateCompatibleBitmap failed\n");

    iDcFormat = GetRealColorDepth();
    printf("got iDcFormat = %ld\n", iDcFormat);

    hbmp1bpp_a = CreateBitmap(4, 2, 1, 1, ajBits1);
    ok(hbmp1bpp_a != 0, "CreateBitmap failed\n");

    hbmp1bpp_b = CreateBitmap(4, 2, 1, 1, ajBits1);
    ok(hbmp1bpp_b != 0, "CreateBitmap failed\n");

    hbmp8bpp_a = CreateBitmap(4, 2, 1, 8, ajBits8);
    ok(hbmp8bpp_a != 0, "CreateBitmap failed\n");

    hbmp8bpp_b = CreateBitmap(4, 2, 1, 8, ajBits8);
    ok(hbmp8bpp_b != 0, "CreateBitmap failed\n");

    hbmp16bpp_a = CreateBitmap(4, 2, 1, 16, ajBits16);
    ok(hbmp16bpp_a != 0, "CreateBitmap failed\n");

    hbmp16bpp_b = CreateBitmap(4, 2, 1, 16, ajBits16);
    ok(hbmp16bpp_b != 0, "CreateBitmap failed\n");

    hbmp24bpp_a = CreateBitmap(4, 2, 1, 24, ajBits24);
    ok(hbmp24bpp_a != 0, "CreateBitmap failed\n");

    hbmp24bpp_b = CreateBitmap(4, 2, 1, 24, ajBits24);
    ok(hbmp24bpp_b != 0, "CreateBitmap failed\n");

    hbmp32bpp_a = CreateBitmap(4, 2, 1, 32, ajBits32);
    ok(hbmp32bpp_a != 0, "CreateBitmap failed\n");

    hbmp32bpp_b = CreateBitmap(4, 2, 1, 32, ajBits32);
    ok(hbmp32bpp_b != 0, "CreateBitmap failed\n");

}

void
Test_SrcMono1(ULONG iBmpFormat, HBITMAP hbmpDst, PVOID pvBits)
{
    COLORREF c, expected;
    HBRUSH hbr;
    RECT rect;
    struct
    {
        BITMAPINFOHEADER bmiHeader;
        ULONG bmiColors[2];
        BYTE aj[32];
    } bmi;

    SelectObject(hdcSrc, hbmp1bpp_a);
    SelectObject(hdcDst, hbmpDst);

    /* Set default dc fore and back colors */
    SetTextColor(hdcSrc, 0x000000);
    SetBkColor(hdcSrc, 0xffffff);
    SetTextColor(hdcDst, 0x000000);
    SetBkColor(hdcDst, 0xffffff);

    /* Do a bitblt operation */
    ok(BitBlt(hdcDst, 0, 0, 2, 2, hdcSrc, 0, 0, SRCCOPY), "(%ld): BitBlt failed", iBmpFormat);

    /* Test background color */
    c = GetPixel(hdcDst, 0, 0);
    expected = 0xffffff;
    ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);

    /* Test foreground color */
    c = GetPixel(hdcDst, 1, 0);
    expected = 0x000000;
    ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);

    if (pvBits)
    {
        c = GetDIBPixel(iBmpFormat, pvBits, 0);
        expected = iXlateFromRGB(iBmpFormat,  GetBkColor(hdcSrc));
        ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);

        c = GetDIBPixel(iBmpFormat, pvBits, 1);
        expected = iXlateFromRGB(iBmpFormat, GetTextColor(hdcSrc));
        ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);
    }

    /* Set different dc fore and back colors */
    SetTextColor(hdcSrc, 0xf00f0f);
    SetBkColor(hdcSrc, 0xf0ff0f);
    SetTextColor(hdcDst, 0xefFee5);
    SetBkColor(hdcDst, 0x100121);

    /* Make sure this alone didn't affect the resulting colors */
    c = GetPixel(hdcDst, 0, 0);
    expected = 0xffffff;
    ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);
    c = GetPixel(hdcDst, 1, 0);
    expected = 0x000000;
    ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);

    /* Repeat the bitblt operation */
    ok(BitBlt(hdcDst, 0, 0, 2, 2, hdcSrc, 0, 0, SRCCOPY), "(%ld): BitBlt failed", iBmpFormat);

    /* Test background color */
    c = GetPixel(hdcDst, 0, 0);
    expected = GetClosestColor(iBmpFormat, GetBkColor(hdcDst), 0xffffff);
    ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);

    /* Test foreground color */
    c = GetPixel(hdcDst, 1, 0);
    expected = GetClosestColor(iBmpFormat, GetTextColor(hdcDst), 0);
    ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);

    if (pvBits)
    {
        c = GetDIBPixel(iBmpFormat, pvBits, 0);
        expected = iXlateFromRGB(iBmpFormat, GetBkColor(hdcDst));
        ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);

        c = GetDIBPixel(iBmpFormat, pvBits, 1);
        expected = iXlateFromRGB(iBmpFormat, GetTextColor(hdcDst));
        ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);
    }

    /* Set inverted fore and back colors */
    SetTextColor(hdcSrc, 0);
    SetBkColor(hdcSrc, 0xffffff);
    SetTextColor(hdcDst, 0xffffff);
    SetBkColor(hdcDst, 0x000000);

    /* Repeat the bitblt operation */
    ok(BitBlt(hdcDst, 0, 0, 2, 2, hdcSrc, 0, 0, SRCCOPY), "(%ld): BitBlt failed", iBmpFormat);

    /* Test background color */
    c = GetPixel(hdcDst, 0, 0);
    expected = GetClosestColor(iBmpFormat, GetBkColor(hdcDst), 0xffffff);
    ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);

    /* Test foreground color */
    c = GetPixel(hdcDst, 1, 0);
    expected = GetClosestColor(iBmpFormat, GetTextColor(hdcDst), 0);
    ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);

    if (pvBits)
    {
        c = GetDIBPixel(iBmpFormat, pvBits, 0);
        expected = iXlateFromRGB(iBmpFormat, GetBkColor(hdcDst));
        ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);

        c = GetDIBPixel(iBmpFormat, pvBits, 1);
        expected = iXlateFromRGB(iBmpFormat, GetTextColor(hdcDst));
        ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);
    }


/* Hatch brush ****************************************************************/

    /* Set dc fore and back colors */
    SetTextColor(hdcDst, 0x102030);
    SetBkColor(hdcDst, 0xeeccdd);
    SetBkMode(hdcDst, OPAQUE);

    /* Create a hatch brush */
    hbr = CreateHatchBrush(HS_DIAGCROSS, 0x123456);

    /* Fill the destination bitmap */
    rect.left = rect.top = 0;
    rect.bottom = rect.right = 4;
    ok(FillRect(hdcDst, &rect, hbr), "FillRect failed\n");

    /* Test the fore color of the hatch brush */
    c = GetPixel(hdcDst, 0, 0);
    expected = GetClosestColor(iBmpFormat, 0x123456, 0);
    ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);

    /* Test the back color of the hatch brush */
    c = GetPixel(hdcDst, 1, 0);
    expected = GetClosestColor(iBmpFormat, GetBkColor(hdcDst), 0xffffff);
    ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);

    if (pvBits)
    {
        c = GetDIBPixel(iBmpFormat, pvBits, 0);
        expected = iXlateFromRGB(iBmpFormat, 0x123456);
        ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);

        c = GetDIBPixel(iBmpFormat, pvBits, 1);
        expected = iXlateFromRGB(iBmpFormat, GetBkColor(hdcDst));
        ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);
    }

    DeleteObject(hbr);

/* DIB brush ******************************************************************/

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 8;
    bmi.bmiHeader.biHeight = 8;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 1;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 1;
    bmi.bmiHeader.biYPelsPerMeter = 1;
    bmi.bmiHeader.biClrUsed = 2;
    bmi.bmiHeader.biClrImportant = 2;
    bmi.bmiColors[0] = 0xeeeeee;
    bmi.bmiColors[1] = 0x111111;
    memset(bmi.aj, 0xaaaa, sizeof(bmi.aj));
    hbr = CreateDIBPatternBrushPt(&bmi, DIB_RGB_COLORS);
    ok(hbr != 0, "CreateDIBPatternBrushPt failed\n");

    rect.left = rect.top = 0;
    rect.bottom = rect.right = 4;
    ok(FillRect(hdcDst, &rect, hbr),"FillRect failed\n");

    /* Test color 1 of the dib brush */
    c = GetPixel(hdcDst, 0, 0);
    expected = GetClosestColor(iBmpFormat, bmi.bmiColors[1], 0);
    ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);

    /* Test color 0 of the dib brush */
    c = GetPixel(hdcDst, 1, 0);
    expected = GetClosestColor(iBmpFormat, bmi.bmiColors[0], 0xffffff);
    ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);

    if (pvBits)
    {
        c = GetDIBPixel(iBmpFormat, pvBits, 0);
        expected = iXlateFromRGB(iBmpFormat, bmi.bmiColors[1]);
        ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);

        c = GetDIBPixel(iBmpFormat, pvBits, 1);
        expected = iXlateFromRGB(iBmpFormat, bmi.bmiColors[0]);
        ok(c == expected, "(%ld): wrong color, expected %lx, got %lx\n", iBmpFormat, expected, c);
    }

    DeleteObject(hbr);


}

void
Test_SrcMono()
{
    HBITMAP hbmp;
    struct
    {
        BITMAPINFOHEADER bmiHeader;
        ULONG bmiColors[3];
    } bmi;
    PVOID pvBits;
    ULONG c, expected;

    SelectObject(hdcSrc, hbmp1bpp_a);

    Test_SrcMono1(BMF_1BPP, hbmp1bpp_b, 0);
    Test_SrcMono1(iDcFormat, hbmpCompat, 0);

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 2;
    bmi.bmiHeader.biHeight = -2;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 16;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 1;
    bmi.bmiHeader.biYPelsPerMeter = 1;
    bmi.bmiHeader.biClrUsed = 0;
    bmi.bmiHeader.biClrImportant = 0;
    hbmp = CreateDIBSection(hdcDst, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    ok(hbmp != 0, "CreateDIBSection failed\n");
    memset(pvBits, 0x55555555, 8 * 8 * 2);

    SelectObject(hdcDst, hbmp);

    c = GetPixel(hdcDst, 0, 0);
    expected = iXlateToRGB(BMF_16BPP_555, 0x5555);
    ok(c == expected, "expected %lx, got %lx\n", expected, c);

    expected = 0x123456;
    SetPixel(hdcDst, 0, 0, expected);
    expected = iXlateFromRGB(BMF_16BPP_555, expected);
    c = *(volatile WORD*)pvBits;
    ok(c == expected, "expected %lx, got %lx\n", expected, c);

    Test_SrcMono1(BMF_16BPP_555, hbmp, pvBits);

    DeleteObject(hbmp);

    /* Create a 565 DIB section */
    bmi.bmiHeader.biCompression = BI_BITFIELDS;
    bmi.bmiHeader.biClrUsed = 3;
    bmi.bmiHeader.biClrImportant = 3;
    bmi.bmiColors[0] = 0xF800;
    bmi.bmiColors[1] = 0x7E0;
    bmi.bmiColors[2] = 0x1F;
    hbmp = CreateDIBSection(hdcDst, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    ok(hbmp != 0, "CreateDIBSection failed\n");
    SelectObject(hdcDst, hbmp);

    Test_SrcMono1(BMF_16BPP_565, hbmp, pvBits);

    DeleteObject(hbmp);

    /* Create a 32 bpp DIB section */
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biClrUsed = 0;
    bmi.bmiHeader.biClrImportant = 0;
    hbmp = CreateDIBSection(hdcDst, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    ok(hbmp != 0, "CreateDIBSection failed\n");
    SelectObject(hdcDst, hbmp);

    Test_SrcMono1(BMF_32BPP_BGR, hbmp, pvBits);

    DeleteObject(hbmp);

}


START_TEST(xlate)
{
    Initialize();

 	Test_SrcMono();
}

// trunk: 41 failures

