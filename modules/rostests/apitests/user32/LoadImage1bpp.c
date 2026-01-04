
#include "precomp.h"
#include "resource_1bpp.h"

START_TEST(LoadImage1bpp)
{
    HDC hdc1, hdc2;
    HBITMAP hBmp1, hBmp2;
    BITMAP bitmap1, bitmap2;
    BITMAPINFO bmi;
    UINT size;
    HGLOBAL hMem;
    LPVOID lpBits;
    BYTE img[4 * 4] = { 0 };

    hdc1 = CreateCompatibleDC(NULL);
    hBmp1 = (HBITMAP)LoadImageW(GetModuleHandle(NULL), MAKEINTRESOURCEW(201), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
    SelectObject(hdc1, hBmp1);
    GetObject(hBmp1, sizeof(BITMAP), &bitmap1);

    ok(bitmap1.bmBitsPixel == 1, "Should have been '1', but got '%d'\n", bitmap1.bmBitsPixel);
    ok(bitmap1.bmWidth == 4, "Should have been '4', but got '%d'\n", bitmap1.bmWidth);
    ok(bitmap1.bmHeight == 4, "Should have been '4', but got '%d'\n", bitmap1.bmHeight);

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = bitmap1.bmWidth;
    bmi.bmiHeader.biHeight      = bitmap1.bmHeight;
    bmi.bmiHeader.biPlanes      = bitmap1.bmPlanes;
    bmi.bmiHeader.biBitCount    = bitmap1.bmBitsPixel;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage   = 0;

    /* Get the size of the bitmap */
    size = ((bitmap1.bmWidth * bitmap1.bmBitsPixel + 31) / 32) * 4 * bitmap1.bmHeight;
    ok(size == 16, "Expected 16, but size is %d\n", size);

    hMem = GlobalAlloc(GMEM_MOVEABLE, size);
    lpBits = GlobalLock(hMem);
    GetDIBits(hdc1, hBmp1, 0, bitmap1.bmHeight, lpBits, &bmi, DIB_RGB_COLORS);

    /* Get bytes from bitmap (we know its 4x4 1BPP */
    memcpy(img, (VOID *)((INT_PTR)lpBits), 16);

    ok(img[0] == 0x60, "Got 0x%02x, expected 0x60\n", img[0]);
    ok(img[1] == 0, "Got 0x%02x, expected 0\n", img[1]);
    ok(img[2] == 0, "Got 0x%02x, expected 0\n", img[2]);
    ok(img[3] == 0, "Got 0x%02x, expected 0\n", img[3]);
    ok(img[4] == 0x90, "Got 0x%02x, expected x90\n", img[4]);
    ok(img[5] == 0, "Got 0x%02x, expected 0x60\n", img[5]);
    ok(img[6] == 0, "Got 0x%02x, expected 0\n", img[6]);
    ok(img[7] == 0, "Got 0x%02x, expected 0\n", img[7]);

    ok(img[8] == 0x90, "Got 0x%02x, expected 0x90\n", img[8]);
    ok(img[9] == 0, "Got 0x%02x, expected 0\n", img[9]);
    ok(img[10] == 0, "Got 0x%02x, expected 0\n", img[10]);
    ok(img[11] == 0, "Got 0x%02x, expected 0\n", img[11]);

    ok(img[12] == 0x60, "Got 0x%02x, expected x60\n", img[12]);
    ok(img[13] == 0, "Got 0x%02x, expected 0x60\n", img[13]);
    ok(img[14] == 0, "Got 0x%02x, expected 0\n", img[14]);
    ok(img[15] == 0, "Got 0x%02x, expected 0\n", img[15]);

    hdc2 = CreateCompatibleDC(NULL);
    hBmp2 = (HBITMAP)LoadImageW(GetModuleHandle(NULL), MAKEINTRESOURCEW(202), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
    SelectObject(hdc2, hBmp2);
    GetObject(hBmp2, sizeof(BITMAP), &bitmap2);
    ok(bitmap2.bmBitsPixel == 1, "Should have been '1', but got %d\n", bitmap2.bmBitsPixel);
    ok(bitmap2.bmWidth == 4, "Should have been '4', but got '%d'\n", bitmap2.bmWidth);
    ok(bitmap2.bmHeight == 4, "Should have been '4', but got '%d'\n", bitmap2.bmHeight);

    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = bitmap2.bmWidth;
    bmi.bmiHeader.biHeight      = bitmap2.bmHeight;
    bmi.bmiHeader.biPlanes      = bitmap2.bmPlanes;
    bmi.bmiHeader.biBitCount    = bitmap2.bmBitsPixel;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage   = 0;

    /* Get the size of the bitmap */
    size = ((bitmap2.bmWidth * bitmap2.bmBitsPixel + 31) / 32) * 4 * bitmap2.bmHeight;
    ok(size == 16, "Expected 16, but size is %d\n", size);

    GetDIBits(hdc2, hBmp2, 0, bitmap2.bmHeight, lpBits, &bmi, DIB_RGB_COLORS);

    /* Clear img array for new test */
    memset(img, 0, 16);

    /* Get bytes from bitmap (we know its 4x4 1BPP */
    memcpy(img, (VOID *)((INT_PTR)lpBits), 16);

    ok(img[0] == 0x60, "Got 0x%02x, expected 0x60\n", img[0]);
    ok(img[1] == 0, "Got 0x%02x, expected 0\n", img[1]);
    ok(img[2] == 0, "Got 0x%02x, expected 0\n", img[2]);
    ok(img[3] == 0, "Got 0x%02x, expected 0\n", img[3]);

    ok(img[4] == 0x90, "Got 0x%02x, expected x90\n", img[4]);
    ok(img[5] == 0, "Got 0x%02x, expected 0x60\n", img[5]);
    ok(img[6] == 0, "Got 0x%02x, expected 0\n", img[6]);
    ok(img[7] == 0, "Got 0x%02x, expected 0\n", img[7]);

    ok(img[8] == 0x90, "Got 0x%02x, expected 0x90\n", img[8]);
    ok(img[9] == 0, "Got 0x%02x, expected 0\n", img[9]);
    ok(img[10] == 0, "Got 0x%02x, expected 0\n", img[10]);
    ok(img[11] == 0, "Got 0x%02x, expected 0\n", img[11]);

    ok(img[12] == 0x60, "Got 0x%02x, expected x60\n", img[12]);
    ok(img[13] == 0, "Got 0x%02x, expected 0x60\n", img[13]);
    ok(img[14] == 0, "Got 0x%02x, expected 0\n", img[14]);
    ok(img[15] == 0, "Got 0x%02x, expected 0\n", img[15]);

    GlobalUnlock(hMem);
    GlobalFree(hMem);
    DeleteDC(hdc1);
    DeleteDC(hdc2);

}
