
#include <stdio.h>
#include <windows.h>
#include <wine/test.h>

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
static HDC hdcSrc, hdcDst;
static ULONG dstbpp;

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

    /* Get the basic color depth */
    ulColorDepth = bmi.bmiHeader.biBitCount;

    /* Special case 16 bpp */
    if (ulColorDepth == 16)
    {
        /* Call again to fill in the bitfields */
        GetDIBits(hdc, hbmp, 0, 1, NULL, pbmi, DIB_RGB_COLORS);

        /* Check the red mask */
        if (bmi.aulMasks[0] == 0x7c00)
            ulColorDepth = 15;
    }

    /* Cleanup and return */
    DeleteObject(hbmp);
    return ulColorDepth;
}

static
ULONG
GetClosestColor(ULONG bpp, COLORREF crColor)
{
    ULONG ulRed, ulGreen, ulBlue;
    
    ulRed = GetRValue(crColor);
    ulGreen = GetGValue(crColor);
    ulBlue = GetBValue(crColor);

    switch (bpp)
    {
        case 1:
            return crColor ? 0xffffff : 0;
        case 8:
        case 15:
            ulRed &= 0xF8;
            ulGreen &= 0xF8;
            ulBlue &= 0xF8;
            printf("ulRed = %lx\n", ulRed);
            ulRed |= ulRed >> 5;
            ulGreen |= ulGreen >> 5;
            ulBlue |= ulBlue >> 5;
            printf("ulRed = %lx\n", ulRed);
            return RGB(ulRed, ulGreen, ulBlue);

        case 16:
            ulRed &= 0xF8;
            ulGreen &= 0xFC;
            ulBlue &= 0xF8;
            ulRed |= ulRed >> 5;
            ulGreen |= ulGreen >> 6;
            ulBlue |= ulBlue >> 5;
            return RGB(ulRed, ulGreen, ulBlue);

        case 24:
        case 32:
            return crColor;
    }
    return 0;
}


static
void
Initialize()
{
    hdcSrc = CreateCompatibleDC(0);
    hdcDst = CreateCompatibleDC(0);

    dstbpp = GetRealColorDepth();

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
Test_SrcMono1(UINT cBits, HBITMAP hbmpDst)
{
    ULONG c, expected;
    HBRUSH hbr;
    RECT rect;
    ULONG dstbpp = GetDeviceCaps(hdcDst, BITSPIXEL);
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
    ok(BitBlt(hdcDst, 0, 0, 2, 2, hdcSrc, 0, 0, SRCCOPY), "%dbpp: BitBlt failed", cBits);

    /* Check resulting colors */
    c = GetPixel(hdcDst, 0, 0);
    ok(c == RGB(255, 255, 255), "%dbpp: wrong color, expected 0, got %lx\n", cBits, c);
    c = GetPixel(hdcDst, 1, 0);
    ok(c == RGB(0, 0, 0), "%dbpp: wrong color, expected ffffff, got %lx\n", cBits, c);

    /* Set different dc fore and back colors */
    SetTextColor(hdcSrc, 0xf00f0f);
    SetBkColor(hdcSrc, 0xf0ff0f);
    SetTextColor(hdcDst, 0xffFFff);
    SetBkColor(hdcDst, 0x000000);

    /* Make sure this alone didn't affect the resulting colors */
    c = GetPixel(hdcDst, 0, 0);
    ok(c == RGB(255, 255, 255), "%dbpp: wrong color, expected 0, got %lx\n", cBits, c);
    c = GetPixel(hdcDst, 1, 0);
    ok(c == RGB(0, 0, 0), "%dbpp: wrong color, expected ffffff, got %lx\n", cBits, c);

    /* Repeat the bitblt operation */
    ok(BitBlt(hdcDst, 0, 0, 2, 2, hdcSrc, 0, 0, SRCCOPY), "%dbpp: BitBlt failed", cBits);

    /* Finally test effect of the fore / cack color on the operation */
    c = GetPixel(hdcDst, 0, 0);
    expected = cBits >= dstbpp ? GetBkColor(hdcDst) : 0xffffff;
    ok(c == expected, "%dbpp: wrong color, expected %lx, got %lx\n", cBits, expected, c);
    c = GetPixel(hdcDst, 1, 0);
    expected = cBits >= dstbpp ? GetTextColor(hdcDst) : 0;
    ok(c == expected, "%dbpp: wrong color, expected %lx, got %lx\n", cBits, expected, c);

    /* Set dc fore and back colors */
    SetTextColor(hdcDst, 0x102030);
    SetBkColor(hdcDst, 0xeeccee);
    SetBkMode(hdcDst, OPAQUE);

    /* Create a hatch brush */
    hbr = CreateHatchBrush(HS_DIAGCROSS, 0x123456);

    /* Fill the destination bitmap */
    rect.left = rect.top = 0;
    rect.bottom = rect.right = 4;
    ok(FillRect(hdcDst, &rect, hbr), "FillRect failed\n");

    /* Test the fore color of the hatch brush */
    c = GetPixel(hdcDst, 0, 0);
    expected = cBits >= dstbpp ? 0x123456 : 0;
    expected = GetClosestColor(dstbpp, expected);
    ok(c == expected, "%dbpp: wrong color, expected %lx, got %lx\n", cBits, expected, c);

    /* Test the back color of the hatch brush */
    c = GetPixel(hdcDst, 1, 0);
    expected = cBits >= dstbpp ? GetBkColor(hdcDst) : 0xffffff;
    expected = GetClosestColor(dstbpp, expected);
    ok(c == expected, "%dbpp: wrong color, expected %lx, got %lx\n", cBits, expected, c);

    DeleteObject(hbr);

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

    /* Test the fore and back color of the dib brush */
    c = GetPixel(hdcDst, 0, 1);
    expected = cBits >= dstbpp ? bmi.bmiColors[1] : 0;
    expected = GetClosestColor(dstbpp, expected);
    ok(c == expected, "%dbpp: wrong color, expected %lx, got %lx\n", cBits, expected, c);

    c = GetPixel(hdcDst, 1, 0);
    expected = cBits >= dstbpp ? bmi.bmiColors[0] : 0xffffff;
    expected = GetClosestColor(dstbpp, expected);
    ok(c == expected, "%dbpp: wrong color, expected %lx, got %lx\n", cBits, expected, c);

    DeleteObject(hbr);



}


void
Test_SrcMono()
{
 
    Test_SrcMono1(1, hbmp1bpp_b);
    Test_SrcMono1(8, hbmp8bpp_b);
    Test_SrcMono1(16, hbmp16bpp_b);
    Test_SrcMono1(24, hbmp24bpp_b);
    Test_SrcMono1(32, hbmp32bpp_b);


}

START_TEST(xlate)
{
    Initialize();

 	Test_SrcMono(); 
}
