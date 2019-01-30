/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for World Transformation and font rendering
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

typedef struct tagBITMAPINFOEX
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[256];
} BITMAPINFOEX, FAR *LPBITMAPINFOEX;

#if 1
    #define SaveBitmapToFile(f, h)
#else
static BOOL SaveBitmapToFile(LPCWSTR pszFileName, HBITMAP hbm)
{
    BOOL f;
    BITMAPFILEHEADER bf;
    BITMAPINFOEX bmi;
    BITMAPINFOHEADER *pbmih;
    DWORD cb, cbColors;
    HDC hDC;
    HANDLE hFile;
    LPVOID pBits;
    BITMAP bm;

    if (!GetObjectW(hbm, sizeof(BITMAP), &bm))
        return FALSE;

    pbmih = &bmi.bmiHeader;
    ZeroMemory(pbmih, sizeof(BITMAPINFOHEADER));
    pbmih->biSize             = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth            = bm.bmWidth;
    pbmih->biHeight           = bm.bmHeight;
    pbmih->biPlanes           = 1;
    pbmih->biBitCount         = bm.bmBitsPixel;
    pbmih->biCompression      = BI_RGB;
    pbmih->biSizeImage        = bm.bmWidthBytes * bm.bmHeight;

    if (bm.bmBitsPixel < 16)
        cbColors = (1 << bm.bmBitsPixel) * sizeof(RGBQUAD);
    else
        cbColors = 0;

    bf.bfType = 0x4d42;
    bf.bfReserved1 = 0;
    bf.bfReserved2 = 0;
    cb = sizeof(BITMAPFILEHEADER) + pbmih->biSize + cbColors;
    bf.bfOffBits = cb;
    bf.bfSize = cb + pbmih->biSizeImage;

    pBits = HeapAlloc(GetProcessHeap(), 0, pbmih->biSizeImage);
    if (pBits == NULL)
        return FALSE;

    f = FALSE;
    hDC = CreateCompatibleDC(NULL);
    if (hDC)
    {
        if (GetDIBits(hDC, hbm, 0, bm.bmHeight, pBits, (BITMAPINFO *)&bmi,
                      DIB_RGB_COLORS))
        {
            hFile = CreateFileW(pszFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL |
                                FILE_FLAG_WRITE_THROUGH, NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                f = WriteFile(hFile, &bf, sizeof(BITMAPFILEHEADER), &cb, NULL) &&
                    WriteFile(hFile, &bmi, sizeof(BITMAPINFOHEADER), &cb, NULL) &&
                    WriteFile(hFile, bmi.bmiColors, cbColors, &cb, NULL) &&
                    WriteFile(hFile, pBits, pbmih->biSizeImage, &cb, NULL);
                CloseHandle(hFile);

                if (!f)
                    DeleteFileW(pszFileName);
            }
        }
        DeleteDC(hDC);
    }
    HeapFree(GetProcessHeap(), 0, pBits);
    return f;
}
#endif

static VOID
setXFORM(XFORM *pxform,
         FLOAT eM11, FLOAT eM12,
         FLOAT eM21, FLOAT eM22,
         FLOAT eDx, FLOAT eDy)
{
    pxform->eM11 = eM11;
    pxform->eM12 = eM12;
    pxform->eM21 = eM21;
    pxform->eM22 = eM22;
    pxform->eDx = eDx;
    pxform->eDy = eDy;
}

START_TEST(TextTransform)
{
    HDC hDC;
    BITMAPINFO bmi;
    LPVOID pvBits;
    HBITMAP hbm;
    LOGFONTA lf;
    HFONT hFont;
    HGDIOBJ hbmOld, hFontOld;
    RECT rc;
    WCHAR chBlackBox = L'g';
    SIZE siz;
    POINT pt;
    XFORM xform;
    HBRUSH hWhiteBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    const COLORREF BLACK = RGB(0, 0, 0);
    const COLORREF WHITE = RGB(255, 255, 255);

    hDC = CreateCompatibleDC(NULL);
    ok(hDC != NULL, "hDC was NULL.\n");

    SetBkMode(hDC, TRANSPARENT);
    SetMapMode(hDC, MM_ANISOTROPIC);

    siz.cx = siz.cy = 100;
    pt.x = siz.cx / 2;
    pt.y = siz.cy / 2;
    SetWindowOrgEx(hDC, -pt.x, -pt.y, NULL);

    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = siz.cx;
    bmi.bmiHeader.biHeight = siz.cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    hbm = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    ok(hbm != NULL, "hbm was NULL.\n");

    ZeroMemory(&lf, sizeof(lf));
    lf.lfHeight = -50;
    lf.lfCharSet = DEFAULT_CHARSET;
    lstrcpyA(lf.lfFaceName, "Marlett");
    hFont = CreateFontIndirectA(&lf);
    ok(hFont != NULL, "hFont was NULL.\n");

    hbmOld = SelectObject(hDC, hbm);
    hFontOld = SelectObject(hDC, hFont);

    SetRect(&rc, -siz.cx / 2, -siz.cy / 2, siz.cx / 2, siz.cy / 2);

    FillRect(hDC, &rc, hWhiteBrush);
    SaveBitmapToFile(L"1.bmp", hbm);
    ok_long(GetPixel(hDC, +siz.cx / 4, +siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, -siz.cx / 4, +siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, +siz.cx / 4, -siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, -siz.cx / 4, -siz.cy / 4), WHITE);

    FillRect(hDC, &rc, hWhiteBrush);
    SetTextAlign(hDC, TA_LEFT | TA_BOTTOM);
    TextOutW(hDC, 0, 0, &chBlackBox, 1);
    SaveBitmapToFile(L"2.bmp", hbm);
    ok_long(GetPixel(hDC, +siz.cx / 4, +siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, -siz.cx / 4, +siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, +siz.cx / 4, -siz.cy / 4), BLACK);
    ok_long(GetPixel(hDC, -siz.cx / 4, -siz.cy / 4), WHITE);

    SetGraphicsMode(hDC, GM_ADVANCED);

    ok_int(ModifyWorldTransform(hDC, NULL, MWT_IDENTITY), TRUE);
    FillRect(hDC, &rc, hWhiteBrush);
    setXFORM(&xform, 2, 0, 0, 1, 0, 0);
    ok_int(SetWorldTransform(hDC, &xform), TRUE);
    SetTextAlign(hDC, TA_LEFT | TA_BOTTOM);
    TextOutW(hDC, 0, 0, &chBlackBox, 1);
    ok_int(ModifyWorldTransform(hDC, NULL, MWT_IDENTITY), TRUE);
    SaveBitmapToFile(L"3.bmp", hbm);
    ok_long(GetPixel(hDC, +siz.cx / 4, +siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, -siz.cx / 4, +siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, +siz.cx / 4, -siz.cy / 4), BLACK);
    ok_long(GetPixel(hDC, -siz.cx / 4, -siz.cy / 4), WHITE);

    ok_int(ModifyWorldTransform(hDC, NULL, MWT_IDENTITY), TRUE);
    FillRect(hDC, &rc, hWhiteBrush);
    setXFORM(&xform, 1, 1, 1, 1, 0, 0);
    ok_int(SetWorldTransform(hDC, &xform), FALSE);
    SetTextAlign(hDC, TA_LEFT | TA_BOTTOM);
    TextOutW(hDC, 0, 0, &chBlackBox, 1);
    ok_int(ModifyWorldTransform(hDC, NULL, MWT_IDENTITY), TRUE);
    SaveBitmapToFile(L"4.bmp", hbm);
    ok_long(GetPixel(hDC, +siz.cx / 4, +siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, -siz.cx / 4, +siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, +siz.cx / 4, -siz.cy / 4), BLACK);
    ok_long(GetPixel(hDC, -siz.cx / 4, -siz.cy / 4), WHITE);

    ok_int(ModifyWorldTransform(hDC, NULL, MWT_IDENTITY), TRUE);
    FillRect(hDC, &rc, hWhiteBrush);
    setXFORM(&xform, -1, 0, 0, 1, 0, 0);
    ok_int(SetWorldTransform(hDC, &xform), TRUE);
    SetTextAlign(hDC, TA_LEFT | TA_BOTTOM);
    TextOutW(hDC, 0, 0, &chBlackBox, 1);
    ok_int(ModifyWorldTransform(hDC, NULL, MWT_IDENTITY), TRUE);
    SaveBitmapToFile(L"5.bmp", hbm);
    ok_long(GetPixel(hDC, +siz.cx / 4, +siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, -siz.cx / 4, +siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, +siz.cx / 4, -siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, -siz.cx / 4, -siz.cy / 4), BLACK);

    ok_int(ModifyWorldTransform(hDC, NULL, MWT_IDENTITY), TRUE);
    FillRect(hDC, &rc, hWhiteBrush);
    setXFORM(&xform, 0, 1, 1, 0, 0, 0);
    ok_int(SetWorldTransform(hDC, &xform), TRUE);
    SetTextAlign(hDC, TA_LEFT | TA_BOTTOM);
    TextOutW(hDC, 0, 0, &chBlackBox, 1);
    ok_int(ModifyWorldTransform(hDC, NULL, MWT_IDENTITY), TRUE);
    SaveBitmapToFile(L"6.bmp", hbm);
    ok_long(GetPixel(hDC, +siz.cx / 4, +siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, -siz.cx / 4, +siz.cy / 4), BLACK);
    ok_long(GetPixel(hDC, +siz.cx / 4, -siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, -siz.cx / 4, -siz.cy / 4), WHITE);

    ok_int(ModifyWorldTransform(hDC, NULL, MWT_IDENTITY), TRUE);
    FillRect(hDC, &rc, hWhiteBrush);
    setXFORM(&xform, 0, -1, 1, 0, 0, 0);
    ok_int(SetWorldTransform(hDC, &xform), TRUE);
    SetTextAlign(hDC, TA_LEFT | TA_BOTTOM);
    TextOutW(hDC, 0, 0, &chBlackBox, 1);
    ok_int(ModifyWorldTransform(hDC, NULL, MWT_IDENTITY), TRUE);
    SaveBitmapToFile(L"7.bmp", hbm);
    ok_long(GetPixel(hDC, +siz.cx / 4, +siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, -siz.cx / 4, +siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, +siz.cx / 4, -siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, -siz.cx / 4, -siz.cy / 4), BLACK);

    ok_int(ModifyWorldTransform(hDC, NULL, MWT_IDENTITY), TRUE);
    FillRect(hDC, &rc, hWhiteBrush);
    setXFORM(&xform, 0, 1, -1, 0, 0, 0);
    ok_int(SetWorldTransform(hDC, &xform), TRUE);
    SetTextAlign(hDC, TA_LEFT | TA_BOTTOM);
    TextOutW(hDC, 0, 0, &chBlackBox, 1);
    ok_int(ModifyWorldTransform(hDC, NULL, MWT_IDENTITY), TRUE);
    SaveBitmapToFile(L"8.bmp", hbm);
    ok_long(GetPixel(hDC, +siz.cx / 4, +siz.cy / 4), BLACK);
    ok_long(GetPixel(hDC, -siz.cx / 4, +siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, +siz.cx / 4, -siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, -siz.cx / 4, -siz.cy / 4), WHITE);

    ok_int(ModifyWorldTransform(hDC, NULL, MWT_IDENTITY), TRUE);
    FillRect(hDC, &rc, hWhiteBrush);
    setXFORM(&xform, -1, 0, 0.5, -1, 0, 0);
    ok_int(SetWorldTransform(hDC, &xform), TRUE);
    SetTextAlign(hDC, TA_LEFT | TA_BOTTOM);
    TextOutW(hDC, 0, 0, &chBlackBox, 1);
    ok_int(ModifyWorldTransform(hDC, NULL, MWT_IDENTITY), TRUE);
    SaveBitmapToFile(L"9.bmp", hbm);
    ok_long(GetPixel(hDC, +siz.cx / 4, +siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, -siz.cx / 4, +siz.cy / 4), BLACK);
    ok_long(GetPixel(hDC, +siz.cx / 4, -siz.cy / 4), WHITE);
    ok_long(GetPixel(hDC, -siz.cx / 4, -siz.cy / 4), WHITE);

    SelectObject(hDC, hFontOld);
    SelectObject(hDC, hbmOld);

    DeleteObject(hFont);
    DeleteObject(hbm);

    DeleteDC(hDC);
}
