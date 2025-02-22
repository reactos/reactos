/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CImage and CImageDC
 * PROGRAMMER:      Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <atlimage.h>
#include <strsafe.h>
#include "resource.h"

#ifdef HAVE_APITEST
    #include <apitest.h>
#else
    #include "atltest.h"
#endif

struct BITMAPINFOEX : BITMAPINFO
{
    RGBQUAD bmiColorsExtra[256 - 1];
};

static void
Test_PixelAddress(INT iLine, const CImage &image1, const BITMAP &bm, INT x, INT y, BOOL bTopDown)
{
    LPBYTE pb = (LPBYTE)bm.bmBits;

    if (bTopDown)
        pb += bm.bmWidthBytes * y;
    else
        pb += bm.bmWidthBytes * (bm.bmHeight - y - 1);

    pb += (x * bm.bmBitsPixel) / 8;

    LPCVOID addr = image1.GetPixelAddress(x, y);
    ok(pb == addr, "Line %d: (%d, %d): %p vs %p\n", iLine, x, y, pb, addr);
}

static void
Test_BitmapEntry(INT iLine, INT bpp, INT width, INT height, BOOL bTopDown)
{
    HBITMAP hBitmap = ::CreateBitmap(width, height, bpp, 1, NULL);
    ok(hBitmap != NULL, "Line %d: hBitmap was NULL\n", iLine);

    CImage image1;

    ok(image1.IsNull(), "Line %d: IsNull() was TRUE\n", iLine);
    image1.Attach(hBitmap, (bTopDown ? CImage::DIBOR_TOPDOWN : CImage::DIBOR_BOTTOMUP));

    ok(!image1.IsNull(), "Line %d: IsNull() was FALSE\n", iLine);
    ok(!image1.IsDIBSection(), "Line %d: IsDIBSection() was TRUE\n", iLine);

    ok(image1.GetWidth() == width, "Line %d: %d vs %d\n", iLine, image1.GetWidth(), width);
    ok(image1.GetHeight() == height, "Line %d: %d vs %d\n", iLine, image1.GetHeight(), height);
    ok(image1.GetBPP() == bpp, "Line %d: %d vs %d\n", iLine, image1.GetBPP(), 1);
}

static void Test_Bitmap(void)
{
    Test_BitmapEntry(__LINE__, 1, 20, 30, FALSE);
    Test_BitmapEntry(__LINE__, 1, 30, 20, TRUE);
    Test_BitmapEntry(__LINE__, 4, 20, 30, FALSE);
    Test_BitmapEntry(__LINE__, 4, 30, 20, TRUE);
    Test_BitmapEntry(__LINE__, 8, 20, 30, FALSE);
    Test_BitmapEntry(__LINE__, 8, 30, 20, TRUE);
    Test_BitmapEntry(__LINE__, 24, 20, 30, FALSE);
    Test_BitmapEntry(__LINE__, 24, 30, 20, TRUE);
    Test_BitmapEntry(__LINE__, 32, 20, 30, FALSE);
    Test_BitmapEntry(__LINE__, 32, 30, 20, TRUE);
}

static void Test_CompatBitmapEntry(INT iLine, HDC hdc, INT width, INT height)
{
    HBITMAP hBitmap = ::CreateCompatibleBitmap(hdc, width, height);
    ok(hBitmap != NULL, "Line %d: hBitmap was NULL\n", iLine);

    CImage image1;

    ok(image1.IsNull(), "Line %d: IsNull() was TRUE\n", iLine);
    image1.Attach(hBitmap);

    ok(!image1.IsNull(), "Line %d: IsNull() was FALSE\n", iLine);
    ok(!image1.IsDIBSection(), "Line %d: IsDIBSection() was TRUE\n", iLine);

    ok(image1.GetWidth() == width, "Line %d: %d vs %d\n", iLine, image1.GetWidth(), width);
    ok(image1.GetHeight() == height, "Line %d: %d vs %d\n", iLine, image1.GetHeight(), height);
}

static void Test_CompatBitmap(void)
{
    HDC hdc = ::CreateCompatibleDC(NULL);

    Test_CompatBitmapEntry(__LINE__, hdc, 20, 30);
    Test_CompatBitmapEntry(__LINE__, hdc, 20, 30);
    Test_CompatBitmapEntry(__LINE__, hdc, 20, 30);
    Test_CompatBitmapEntry(__LINE__, hdc, 20, 30);
    Test_CompatBitmapEntry(__LINE__, hdc, 20, 30);

    ::DeleteDC(hdc);

    hdc = ::GetDC(NULL);

    Test_CompatBitmapEntry(__LINE__, hdc, 20, 30);
    Test_CompatBitmapEntry(__LINE__, hdc, 20, 30);
    Test_CompatBitmapEntry(__LINE__, hdc, 20, 30);
    Test_CompatBitmapEntry(__LINE__, hdc, 20, 30);
    Test_CompatBitmapEntry(__LINE__, hdc, 20, 30);

    ::ReleaseDC(NULL, hdc);
}

static void
Test_DIBSectionEntry(INT iLine, HDC hdc, INT bpp, INT width, INT height, BOOL bTopDown)
{
    // Initialize BITMAPINFOEX
    BITMAPINFOEX bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = (bTopDown ? -height : height);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = bpp;
    switch (bpp)
    {
        case 1:
            bmi.bmiHeader.biClrUsed = 2;
            bmi.bmiColorsExtra[0].rgbBlue = 0xFF;
            bmi.bmiColorsExtra[0].rgbGreen = 0xFF;
            bmi.bmiColorsExtra[0].rgbRed = 0xFF;
            break;
        case 4:
        case 8:
            bmi.bmiHeader.biClrUsed = 3;
            bmi.bmiColorsExtra[0].rgbBlue = 0xFF;
            bmi.bmiColorsExtra[0].rgbGreen = 0xFF;
            bmi.bmiColorsExtra[0].rgbRed = 0xFF;
            bmi.bmiColorsExtra[1].rgbBlue = 0;
            bmi.bmiColorsExtra[1].rgbGreen = 0;
            bmi.bmiColorsExtra[1].rgbRed = 0xFF;
            break;
        default:
            break;
    }

    // Create a DIB bitmap
    HBITMAP hBitmap = ::CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
    ok(hBitmap != NULL, "Line %d: hBitmap was NULL\n", iLine);

    BITMAP bm;
    ::GetObject(hBitmap, sizeof(bm), &bm);
    INT pitch = (bTopDown ? bm.bmWidthBytes : -bm.bmWidthBytes);

    CImage image1;

    ok(image1.IsNull(), "Line %d: IsNull() was FALSE\n", iLine);
 
    image1.Attach(hBitmap, (bTopDown ? CImage::DIBOR_TOPDOWN : CImage::DIBOR_BOTTOMUP));

    ok(!image1.IsNull(), "Line %d: IsNull() was FALSE\n", iLine);
    ok(image1.IsDIBSection(), "Line %d: IsDIBSection() was FALSE\n", iLine);
    if (bpp == 4 || bpp == 8)
    {
        ok(image1.GetTransparentColor() == 0xFFFFFFFF, "Line %d: 0x%08lX\n", iLine,
           image1.GetTransparentColor());
    }

    switch (bpp)
    {
        case 1:
            ok(image1.GetMaxColorTableEntries() == 2,
               "Line %d: %d\n", iLine, image1.GetMaxColorTableEntries());
            break;
        case 4:
            ok(image1.GetMaxColorTableEntries() == 16,
               "Line %d: %d\n", iLine, image1.GetMaxColorTableEntries());
            break;
        case 8:
            ok(image1.GetMaxColorTableEntries() == 256,
               "Line %d: %d\n", iLine, image1.GetMaxColorTableEntries());
            break;
        case 24:
        case 32:
            ok(image1.GetMaxColorTableEntries() == 0,
               "Line %d: %d\n", iLine, image1.GetMaxColorTableEntries());
            break;
    }

    ok(image1.GetWidth() == width, "Line %d: %d vs %d\n", iLine, image1.GetWidth(), width);
    ok(image1.GetHeight() == height, "Line %d: %d vs %d\n", iLine, image1.GetHeight(), height);
    ok(image1.GetBPP() == bpp, "Line %d: %d vs %d\n", iLine, image1.GetBPP(), bpp);
    ok(image1.GetPitch() == pitch, "Line %d: %d vs %d\n", iLine, image1.GetPitch(), pitch);

    LPBYTE pbBits = (LPBYTE)bm.bmBits;
    if (!bTopDown)
        pbBits += bm.bmWidthBytes * (height - 1);
    ok(image1.GetBits() == pbBits, "Line %d: %p vs %p\n", iLine, image1.GetBits(), pbBits);

    // Test Color Table
    if (bpp <= 8)
    {
        DWORD Colors[3];
        C_ASSERT(sizeof(DWORD) == sizeof(RGBQUAD));
        FillMemory(Colors, sizeof(Colors), 0xCC);
        image1.GetColorTable(0, _countof(Colors), (RGBQUAD *)Colors);
        ok(Colors[0] == 0, "Line %d: 0x%08lX\n", iLine, Colors[0]);
        ok(Colors[1] == 0xFFFFFF, "Line %d: 0x%08lX\n", iLine, Colors[1]);
        if (bpp >= 4)
            ok(Colors[2] == 0xFF0000, "Line %d: 0x%08lX\n", iLine, Colors[2]);
    }

    // Test SetPixel/GetPixel
    COLORREF color;
    image1.SetPixel(0, 0, RGB(255, 255, 255));
    color = image1.GetPixel(0, 0);
    ok(color == RGB(255, 255, 255), "Line %d: color was 0x%08lX\n", iLine, color);
    image1.SetPixel(0, 0, RGB(0, 0, 0));
    color = image1.GetPixel(0, 0);
    ok(color == RGB(0, 0, 0), "Line %d: color was 0x%08lX\n", iLine, color);

    // Test GetDC/ReleaseDC
    {
        HDC hdc1 = image1.GetDC();
        ok(hdc1 != NULL, "Line %d: hdc1 was NULL\n", iLine);
        ::SetPixelV(hdc1, 2, 2, RGB(255, 255, 255));
        {
            HDC hdc2 = image1.GetDC();
            ok(hdc2 != NULL, "Line %d: hdc2 was NULL\n", iLine);
            color = ::GetPixel(hdc2, 2, 2);
            ok(color == RGB(255, 255, 255), "Line %d: color was 0x%08lX\n", iLine, color);
            image1.ReleaseDC();
        }
        image1.ReleaseDC();
    }

    // Test CImageDC
    {
        CImageDC hdc1(image1);
        ok(hdc1 != NULL, "Line %d: hdc1 was NULL\n", iLine);
        ::SetPixelV(hdc1, 1, 0, RGB(255, 255, 255));
        {
            CImageDC hdc2(image1);
            ok(hdc2 != NULL, "Line %d: hdc2 was NULL\n", iLine);
            color = ::GetPixel(hdc2, 1, 0);
            ok(color == RGB(255, 255, 255), "Line %d: color was 0x%08lX\n", iLine, color);
        }
    }

    HRESULT hr;
    TCHAR szFileName[MAX_PATH];
    LPCTSTR dotexts[] =
    {
        TEXT(".bmp"), TEXT(".jpg"), TEXT(".png"), TEXT(".gif"), TEXT(".tif")
    };

    // Test Save/Load
    for (UINT iDotExt = 0; iDotExt < _countof(dotexts); ++iDotExt)
    {
        ::ExpandEnvironmentStrings(TEXT("%TEMP%\\CImage"), szFileName, _countof(szFileName));
        StringCchCat(szFileName, _countof(szFileName), dotexts[iDotExt]);
        hr = image1.Save(szFileName);
        ok(hr == S_OK, "Line %d: %d: hr was 0x%08lX\n", iLine, iDotExt, hr);

        CImage image2;
        hr = image2.Load(szFileName);
        ok(hr == S_OK, "Line %d: %d: hr was 0x%08lX\n", iLine, iDotExt, hr);
        ::DeleteFile(szFileName);

        CImageDC hdc2(image2);
        ok(hdc2 != NULL, "Line %d: %d: hdc2 was NULL\n", iLine, iDotExt);
        color = ::GetPixel(hdc2, 0, 0);
        ok(color == RGB(0, 0, 0), "Line %d: %d: color was 0x%08lX\n", iLine, iDotExt, color);
        color = ::GetPixel(hdc2, 1, 0);
        ok(color == RGB(255, 255, 255), "Line %d: %d: color was 0x%08lX\n", iLine, iDotExt, color);
    }

    // Test GetPixelAddress
    Test_PixelAddress(iLine, image1, bm, 0, 0, bTopDown);
    Test_PixelAddress(iLine, image1, bm, 10, 0, bTopDown);
    Test_PixelAddress(iLine, image1, bm, 0, 10, bTopDown);
    Test_PixelAddress(iLine, image1, bm, 4, 6, bTopDown);
    Test_PixelAddress(iLine, image1, bm, 6, 2, bTopDown);
}

static void Test_DIBSection(void)
{
    HDC hdc = ::CreateCompatibleDC(NULL);

    Test_DIBSectionEntry(__LINE__, hdc, 1, 30, 20, FALSE);
    Test_DIBSectionEntry(__LINE__, hdc, 1, 20, 30, TRUE);
    Test_DIBSectionEntry(__LINE__, hdc, 4, 30, 20, FALSE);
    Test_DIBSectionEntry(__LINE__, hdc, 4, 20, 30, TRUE);
    Test_DIBSectionEntry(__LINE__, hdc, 8, 30, 20, FALSE);
    Test_DIBSectionEntry(__LINE__, hdc, 8, 20, 30, TRUE);
    Test_DIBSectionEntry(__LINE__, hdc, 24, 30, 20, FALSE);
    Test_DIBSectionEntry(__LINE__, hdc, 24, 20, 30, TRUE);
    Test_DIBSectionEntry(__LINE__, hdc, 32, 30, 20, FALSE);
    Test_DIBSectionEntry(__LINE__, hdc, 32, 20, 30, TRUE);

    ::DeleteDC(hdc);
}

static void Test_ResBitmap(void)
{
    HINSTANCE hInst = GetModuleHandle(NULL);

    CImage image1;
    ok_int(image1.IsNull(), TRUE);
    image1.LoadFromResource(hInst, IDB_ANT);
    ok_int(image1.IsNull(), FALSE);

    ok_int(image1.GetWidth(), 48);
    ok_int(image1.GetHeight(), 48);
    ok_int(image1.GetBPP(), 8);
    ok_int(image1.GetPitch(), -48);

    CImage image2;
    ok_int(image2.IsNull(), TRUE);
    image2.LoadFromResource(hInst, IDB_CROSS);
    ok_int(image2.IsNull(), FALSE);

    ok_int(image2.GetWidth(), 32);
    ok_int(image2.GetHeight(), 32);
    ok_int(image2.GetBPP(), 8);
    ok_int(image2.GetPitch(), -32);
}

static INT FindGUID(REFGUID rguid, const CSimpleArray<GUID>& guids)
{
    for (INT i = 0; i < guids.GetSize(); ++i)
    {
        if (memcmp(&rguid, &guids[i], sizeof(GUID)) == 0)
            return i;
    }
    return -1;
}

static INT FindFilterItem(const TCHAR *filter, const TCHAR *item)
{
    INT iFilter = 0;
    DWORD cbItem = lstrlen(item) * sizeof(TCHAR);
    BOOL bSep = TRUE;

    for (; *filter; ++filter)
    {
        if (bSep && memcmp(item, filter, cbItem) == 0)
            return (iFilter + 1) / 2;

        bSep = (*filter == TEXT('|'));
        if (bSep)
            ++iFilter;
    }

    return -1;
}

static void Test_Importer(void)
{
    HRESULT hr;
    ATL::IAtlStringMgr *mgr = CAtlStringMgr::GetInstance();
    CSimpleArray<GUID> aguidFileTypes;
    INT iNULL, iBMP, iJPEG, iGIF, iPNG, iTIFF, iEMF, iWMF;

    // Try importer with "All Image Files"
    CSimpleString strImporters(mgr);
    aguidFileTypes.RemoveAll();
    hr = CImage::GetImporterFilterString(strImporters, aguidFileTypes, TEXT("All Image Files"), 0);
    ok(hr == S_OK, "Expected hr to be S_OK, was: %ld\n", hr);
    ok(aguidFileTypes.GetSize() >= 8,
       "Expected aguidFileTypes.GetSize() to be >= 8, was %d.", aguidFileTypes.GetSize());

    iNULL = FindGUID(GUID_NULL, aguidFileTypes);
    iBMP = FindGUID(Gdiplus::ImageFormatBMP, aguidFileTypes);
    iJPEG = FindGUID(Gdiplus::ImageFormatJPEG, aguidFileTypes);
    iGIF = FindGUID(Gdiplus::ImageFormatGIF, aguidFileTypes);
    iPNG = FindGUID(Gdiplus::ImageFormatPNG, aguidFileTypes);
    iTIFF = FindGUID(Gdiplus::ImageFormatTIFF, aguidFileTypes);
    iEMF = FindGUID(Gdiplus::ImageFormatEMF, aguidFileTypes);
    iWMF = FindGUID(Gdiplus::ImageFormatWMF, aguidFileTypes);

    ok_int(iNULL, 0);
    ok(iBMP > 0, "iBMP was %d\n", iBMP);
    ok(iJPEG > 0, "iJPEG was %d\n", iJPEG);
    ok(iGIF > 0, "iGIF was %d\n", iGIF);
    ok(iPNG > 0, "iPNG was %d\n", iPNG);
    ok(iTIFF > 0, "iTIFF was %d\n", iTIFF);
    ok(iEMF > 0, "iEMF was %d\n", iEMF);
    ok(iWMF > 0, "iWMF was %d\n", iWMF);

    ok_int(memcmp(strImporters, TEXT("All Image Files|"), sizeof(TEXT("All Image Files|")) - sizeof(TCHAR)), 0);
    ok_int(iBMP, FindFilterItem(strImporters, TEXT("BMP (*.BMP;*.DIB;*.RLE)|*.BMP;*.DIB;*.RLE|")));
    ok_int(iJPEG, FindFilterItem(strImporters, TEXT("JPEG (*.JPG;*.JPEG;*.JPE;*.JFIF)|*.JPG;*.JPEG;*.JPE;*.JFIF|")));
    ok_int(iGIF, FindFilterItem(strImporters, TEXT("GIF (*.GIF)|*.GIF|")));
    ok_int(iPNG, FindFilterItem(strImporters, TEXT("PNG (*.PNG)|*.PNG|")));
    ok_int(iTIFF, FindFilterItem(strImporters, TEXT("TIFF (*.TIF;*.TIFF)|*.TIF;*.TIFF|")));

    // Try importer without "All Image Files"
    aguidFileTypes.RemoveAll();
    strImporters.Empty();
    hr = CImage::GetImporterFilterString(strImporters, aguidFileTypes, NULL, 0);
    ok(hr == S_OK, "Expected hr to be S_OK, was: %ld\n", hr);
    ok(aguidFileTypes.GetSize() >= 7,
       "Expected aguidFileTypes.GetSize() to be >= 7, was %d.", aguidFileTypes.GetSize());

    iNULL = FindGUID(GUID_NULL, aguidFileTypes);
    iBMP = FindGUID(Gdiplus::ImageFormatBMP, aguidFileTypes);
    iJPEG = FindGUID(Gdiplus::ImageFormatJPEG, aguidFileTypes);
    iGIF = FindGUID(Gdiplus::ImageFormatGIF, aguidFileTypes);
    iPNG = FindGUID(Gdiplus::ImageFormatPNG, aguidFileTypes);
    iTIFF = FindGUID(Gdiplus::ImageFormatTIFF, aguidFileTypes);
    iEMF = FindGUID(Gdiplus::ImageFormatEMF, aguidFileTypes);
    iWMF = FindGUID(Gdiplus::ImageFormatWMF, aguidFileTypes);

    ok_int(iNULL, -1);
    ok_int(iBMP, 0);
    ok(iJPEG > 0, "iJPEG was %d\n", iJPEG);
    ok(iGIF > 0, "iGIF was %d\n", iGIF);
    ok(iPNG > 0, "iPNG was %d\n", iPNG);
    ok(iTIFF > 0, "iTIFF was %d\n", iTIFF);
    ok(iEMF > 0, "iEMF was %d\n", iEMF);
    ok(iWMF > 0, "iWMF was %d\n", iWMF);

    ok_int(iBMP, FindFilterItem(strImporters, TEXT("BMP (*.BMP;*.DIB;*.RLE)|*.BMP;*.DIB;*.RLE|")));
    ok_int(iJPEG, FindFilterItem(strImporters, TEXT("JPEG (*.JPG;*.JPEG;*.JPE;*.JFIF)|*.JPG;*.JPEG;*.JPE;*.JFIF|")));
    ok_int(iGIF, FindFilterItem(strImporters, TEXT("GIF (*.GIF)|*.GIF|")));
    ok_int(iPNG, FindFilterItem(strImporters, TEXT("PNG (*.PNG)|*.PNG|")));
    ok_int(iTIFF, FindFilterItem(strImporters, TEXT("TIFF (*.TIF;*.TIFF)|*.TIF;*.TIFF|")));
}

static void Test_Exporter(void)
{
    HRESULT hr;
    ATL::IAtlStringMgr *mgr = CAtlStringMgr::GetInstance();
    CSimpleArray<GUID> aguidFileTypes;
    INT iNULL, iBMP, iJPEG, iGIF, iPNG, iTIFF;

    // Try exporter with "All Image Files"
    CSimpleString strExporters(mgr);
    aguidFileTypes.RemoveAll();
    hr = CImage::GetExporterFilterString(strExporters, aguidFileTypes, TEXT("All Image Files"), 0);
    ok(hr == S_OK, "Expected hr to be S_OK, was: %ld\n", hr);
    ok(aguidFileTypes.GetSize() >= 6,
       "Expected aguidFileTypes.GetSize() to be >= 6, was %d.", aguidFileTypes.GetSize());

    iNULL = FindGUID(GUID_NULL, aguidFileTypes);
    iBMP = FindGUID(Gdiplus::ImageFormatBMP, aguidFileTypes);
    iJPEG = FindGUID(Gdiplus::ImageFormatJPEG, aguidFileTypes);
    iGIF = FindGUID(Gdiplus::ImageFormatGIF, aguidFileTypes);
    iPNG = FindGUID(Gdiplus::ImageFormatPNG, aguidFileTypes);
    iTIFF = FindGUID(Gdiplus::ImageFormatTIFF, aguidFileTypes);

    ok_int(iNULL, 0);
    ok(iBMP > 0, "iBMP was %d\n", iBMP);
    ok(iJPEG > 0, "iJPEG was %d\n", iJPEG);
    ok(iGIF > 0, "iGIF was %d\n", iGIF);
    ok(iPNG > 0, "iPNG was %d\n", iPNG);
    ok(iTIFF > 0, "iTIFF was %d\n", iTIFF);

    ok_int(iBMP, FindFilterItem(strExporters, TEXT("BMP (*.BMP;*.DIB;*.RLE)|*.BMP;*.DIB;*.RLE|")));
    ok_int(iJPEG, FindFilterItem(strExporters, TEXT("JPEG (*.JPG;*.JPEG;*.JPE;*.JFIF)|*.JPG;*.JPEG;*.JPE;*.JFIF|")));
    ok_int(iGIF, FindFilterItem(strExporters, TEXT("GIF (*.GIF)|*.GIF|")));
    ok_int(iPNG, FindFilterItem(strExporters, TEXT("PNG (*.PNG)|*.PNG|")));
    ok_int(iTIFF, FindFilterItem(strExporters, TEXT("TIFF (*.TIF;*.TIFF)|*.TIF;*.TIFF|")));

    // Try exporter without "All Image Files"
    strExporters.Empty();
    aguidFileTypes.RemoveAll();
    hr = CImage::GetExporterFilterString(strExporters, aguidFileTypes, NULL, 0);
    ok(hr == S_OK, "Expected hr to be S_OK, was: %ld\n", hr);
    ok(aguidFileTypes.GetSize() >= 5,
       "Expected aguidFileTypes.GetSize() to be >= 5, was %d.", aguidFileTypes.GetSize());

    iNULL = FindGUID(GUID_NULL, aguidFileTypes);
    iBMP = FindGUID(Gdiplus::ImageFormatBMP, aguidFileTypes);
    iJPEG = FindGUID(Gdiplus::ImageFormatJPEG, aguidFileTypes);
    iGIF = FindGUID(Gdiplus::ImageFormatGIF, aguidFileTypes);
    iPNG = FindGUID(Gdiplus::ImageFormatPNG, aguidFileTypes);
    iTIFF = FindGUID(Gdiplus::ImageFormatTIFF, aguidFileTypes);

    ok_int(iNULL, -1);
    ok_int(iBMP, 0);
    ok(iJPEG > 0, "iJPEG was %d\n", iJPEG);
    ok(iGIF > 0, "iGIF was %d\n", iGIF);
    ok(iPNG > 0, "iPNG was %d\n", iPNG);
    ok(iTIFF > 0, "iTIFF was %d\n", iTIFF);

    ok_int(iBMP, FindFilterItem(strExporters, TEXT("BMP (*.BMP;*.DIB;*.RLE)|*.BMP;*.DIB;*.RLE|")));
    ok_int(iJPEG, FindFilterItem(strExporters, TEXT("JPEG (*.JPG;*.JPEG;*.JPE;*.JFIF)|*.JPG;*.JPEG;*.JPE;*.JFIF|")));
    ok_int(iGIF, FindFilterItem(strExporters, TEXT("GIF (*.GIF)|*.GIF|")));
    ok_int(iPNG, FindFilterItem(strExporters, TEXT("PNG (*.PNG)|*.PNG|")));
    ok_int(iTIFF, FindFilterItem(strExporters, TEXT("TIFF (*.TIF;*.TIFF)|*.TIF;*.TIFF|")));
}

START_TEST(CImage)
{
    Test_Bitmap();
    Test_CompatBitmap();
    Test_DIBSection();
    Test_ResBitmap();
    Test_Importer();
    Test_Exporter();
}
