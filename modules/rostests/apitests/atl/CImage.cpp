/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for CImage
 * PROGRAMMER:      Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <atlimage.h>
#include "resource.h"

#ifdef HAVE_APITEST
    #include <apitest.h>
#else
    #include "atltest.h"
#endif

const TCHAR* szFiles[] = {
    TEXT("ant.png"),
    TEXT("ant.tif"),
    TEXT("ant.gif"),
    TEXT("ant.jpg"),
    TEXT("ant.bmp"),
};

static TCHAR szTempPath[MAX_PATH];
TCHAR* file_name(const TCHAR* file)
{
    static TCHAR buffer[MAX_PATH];
    lstrcpy(buffer, szTempPath);
    lstrcat(buffer, TEXT("\\"));
    lstrcat(buffer, file);
    return buffer;
}

static void write_bitmap(HINSTANCE hInst, int id, TCHAR* file)
{
    HRSRC rsrc;

    rsrc = FindResource(hInst, MAKEINTRESOURCE(id), RT_BITMAP);
    ok(rsrc != NULL, "Expected to find an image resource\n");
    if (rsrc)
    {
        void *rsrc_data;
        HANDLE hfile;
        BOOL ret;
        HGLOBAL glob = LoadResource(hInst, rsrc);
        DWORD rsrc_size = SizeofResource(hInst, rsrc);

        rsrc_data = LockResource(glob);

        hfile = CreateFile(file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        ok(hfile != INVALID_HANDLE_VALUE, "Unable to open temp file: %lu\n", GetLastError());
        if (hfile != INVALID_HANDLE_VALUE)
        {
            BITMAPFILEHEADER bfh = { 0 };
            DWORD dwWritten;

            bfh.bfType = 'MB';
            bfh.bfSize = rsrc_size + sizeof(BITMAPFILEHEADER);
            bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
            bfh.bfReserved1 = bfh.bfReserved2 = 0;
            ret = WriteFile(hfile, &bfh, sizeof(bfh), &dwWritten, NULL);
            ok(ret, "Unable to write temp file: %lu\n", GetLastError());
            ret = WriteFile(hfile, rsrc_data, rsrc_size, &dwWritten, NULL);
            ok(ret, "Unable to write temp file: %lu\n", GetLastError());
            CloseHandle(hfile);
        }
        UnlockResource(rsrc_data);
    }
}

typedef Gdiplus::GpStatus (WINAPI *STARTUP)(ULONG_PTR *, const Gdiplus::GdiplusStartupInput *, Gdiplus::GdiplusStartupOutput *);
typedef void (WINAPI *SHUTDOWN)(ULONG_PTR);
typedef Gdiplus::GpStatus (WINGDIPAPI *CREATEBITMAPFROMFILE)(GDIPCONST WCHAR*, Gdiplus::GpBitmap **);
typedef Gdiplus::GpStatus (WINGDIPAPI *GETPIXELFORMAT)(Gdiplus::GpImage *image, Gdiplus::PixelFormat *format);
typedef Gdiplus::GpStatus (WINGDIPAPI *DISPOSEIMAGE)(Gdiplus::GpImage *);

static HINSTANCE               hinstGdiPlus;
static ULONG_PTR               gdiplusToken;

static STARTUP                 Startup;
static SHUTDOWN                Shutdown;
static CREATEBITMAPFROMFILE    CreateBitmapFromFile;
static GETPIXELFORMAT          GetImagePixelFormat;
static DISPOSEIMAGE            DisposeImage;

template <typename TYPE>
TYPE AddrOf(const char *name)
{
    FARPROC proc = ::GetProcAddress(hinstGdiPlus, name);
    return reinterpret_cast<TYPE>(proc);
}

static void init_gdip()
{
    hinstGdiPlus = ::LoadLibraryA("gdiplus.dll");
    Startup = AddrOf<STARTUP>("GdiplusStartup");
    Shutdown = AddrOf<SHUTDOWN>("GdiplusShutdown");
    CreateBitmapFromFile = AddrOf<CREATEBITMAPFROMFILE>("GdipCreateBitmapFromFile");
    GetImagePixelFormat = AddrOf<GETPIXELFORMAT>("GdipGetImagePixelFormat");
    DisposeImage = AddrOf<DISPOSEIMAGE>("GdipDisposeImage");
}

static void determine_file_bpp(TCHAR* tfile, Gdiplus::PixelFormat expect_pf)
{
    using namespace Gdiplus;
    GpBitmap *pBitmap = NULL;

#ifdef UNICODE
    WCHAR* file = tfile;
#else
    WCHAR file[MAX_PATH];
    ::MultiByteToWideChar(CP_ACP, 0, tfile, -1, file, MAX_PATH);
#endif

    if (Startup == NULL)
        init_gdip();

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Startup(&gdiplusToken, &gdiplusStartupInput, NULL);


    Gdiplus::GpStatus status = CreateBitmapFromFile(file, &pBitmap);
    ok(status == Gdiplus::Ok, "Expected status to be %i, was: %i\n", (int)Gdiplus::Ok, (int)status);
    ok(pBitmap != NULL, "Expected a valid bitmap\n");
    if (pBitmap)
    {
        PixelFormat pf;
        GetImagePixelFormat(pBitmap, &pf);
        ok(pf == expect_pf, "Expected PixelFormat to be 0x%x, was: 0x%x\n", (int)expect_pf, (int)pf);

        DisposeImage(pBitmap);
    }
    Shutdown(gdiplusToken);
}

static void Test_LoadSaveImage(void)
{
    HRESULT hr;
    TCHAR* file;
    BOOL bOK;
    int width, height, bpp;
    size_t n;
    CImage image1, image2;
    COLORREF color;
    HDC hDC;

    HINSTANCE hInst = GetModuleHandle(NULL);
    GetTempPath(MAX_PATH, szTempPath);

    image1.LoadFromResource(hInst, IDB_ANT);
    ok(!image1.IsNull(), "Expected image1 is not null\n");

    width = image1.GetWidth();
    ok(width == 48, "Expected width to be 48, was: %d\n", width);
    height = image1.GetHeight();
    ok(height == 48, "Expected height to be 48, was: %d\n", height);
    bpp = image1.GetBPP();
    ok(bpp == 8, "Expected bpp to be 8, was: %d\n", bpp);

    image2.LoadFromResource(hInst, IDB_CROSS);
    ok(!image2.IsNull(), "Expected image2 is not null\n");
    image2.SetTransparentColor(RGB(255, 255, 255));

    width = image2.GetWidth();
    ok(width == 32, "Expected width to be 32, was: %d\n", width);
    height = image2.GetHeight();
    ok(height == 32, "Expected height to be 32, was: %d\n", height);
    bpp = image2.GetBPP();
    ok(bpp == 8, "Expected bpp to be 8, was: %d\n", bpp);

    color = image1.GetPixel(5, 5);
    ok(color == RGB(166, 202, 240), "Expected color to be 166, 202, 240; was: %i, %i, %i\n", GetRValue(color), GetGValue(color), GetBValue(color));

    hDC = image1.GetDC();
    bOK = image2.Draw(hDC, 0, 0);
    image1.ReleaseDC();
    ok(bOK != FALSE, "Expected bDraw to be TRUE, was: %d\n", bOK);
    image2.Destroy();

    color = image1.GetPixel(5, 5);
    ok(color == RGB(255, 0,0), "Expected color to be 255, 0, 0; was: %i, %i, %i\n", GetRValue(color), GetGValue(color), GetBValue(color));

    file = file_name(TEXT("ant.bmp"));
    write_bitmap(hInst, IDB_ANT, file);

    init_gdip();

    determine_file_bpp(file, PixelFormat8bppIndexed);

    hr = image2.Load(file);
    ok(hr == S_OK, "Expected hr to be S_OK, was: %08lx\n", hr);
    ok(!image2.IsNull(), "Expected image1 is not null\n");
    bOK = DeleteFile(file);
    ok(bOK, "Expected bOK to be TRUE, was: %d\n", bOK);

    width = image2.GetWidth();
    ok_int(width, 48);
    height = image2.GetHeight();
    ok_int(height, 48);
    bpp = image2.GetBPP();
    ok(bpp == 32 || bpp == 8, "bpp was %d\n", bpp);

    for (n = 0; n < _countof(szFiles); ++n)
    {
        file = file_name(szFiles[n]);
        image2.Destroy();

        if (n == 0)
            hr = image1.Save(file, Gdiplus::ImageFormatPNG);
        else
            hr = image1.Save(file);
        ok(hr == S_OK, "Expected hr to be S_OK, was: %08lx (for %i)\n", hr, n);

        bOK = (GetFileAttributes(file) != 0xFFFFFFFF);
        ok(bOK, "Expected bOK to be TRUE, was: %d (for %i)\n", bOK, n);

        hr = image2.Load(file);
        ok(hr == S_OK, "Expected hr to be S_OK, was: %08lx (for %i)\n", hr, n);

        width = image2.GetWidth();
        ok(width == 48, "Expected width to be 48, was: %d (for %i)\n", width, n);
        height = image2.GetHeight();
        ok(height == 48, "Expected height to be 48, was: %d (for %i)\n", height, n);
        bpp = image2.GetBPP();
        if (n == 3)
        {
            ok(bpp == 24 || bpp == 32, "Expected bpp to be 24 or 32, was: %d (for %i)\n", bpp, n);
            determine_file_bpp(file, PixelFormat24bppRGB);
        }
        else
        {
            determine_file_bpp(file, PixelFormat8bppIndexed);
        }
        color = image1.GetPixel(5, 5);
        ok(color == RGB(255, 0,0), "Expected color to be 255, 0, 0; was: %i, %i, %i (for %i)\n", GetRValue(color), GetGValue(color), GetBValue(color), n);

        bOK = DeleteFile(file);
        ok(bOK, "Expected bOK to be TRUE, was: %d (for %i)\n", bOK, n);
    }
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

static INT FindFilterItem(const WCHAR *filter, const WCHAR *item)
{
    INT iFilter = 0;
    size_t cbItem = wcslen(item) * sizeof(WCHAR);
    BOOL bFoundSep = TRUE;
    for (; *filter; ++filter)
    {
        if (bFoundSep && memcmp(item, filter, cbItem) == 0)
            return (iFilter + 1) / 2;

        bFoundSep = (*filter == L'|');
        if (bFoundSep)
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

    ok_int(memcmp(strImporters, L"All Image Files|", sizeof(L"All Image Files|") - sizeof(UNICODE_NULL)), 0);
    ok_int(iBMP, FindFilterItem(strImporters, L"BMP (*.BMP;*.DIB;*.RLE)|*.BMP;*.DIB;*.RLE|"));
    ok_int(iJPEG, FindFilterItem(strImporters, L"JPEG (*.JPG;*.JPEG;*.JPE;*.JFIF)|*.JPG;*.JPEG;*.JPE;*.JFIF|"));
    ok_int(iGIF, FindFilterItem(strImporters, L"GIF (*.GIF)|*.GIF|"));
    ok_int(iPNG, FindFilterItem(strImporters, L"PNG (*.PNG)|*.PNG|"));
    ok_int(iTIFF, FindFilterItem(strImporters, L"TIFF (*.TIF;*.TIFF)|*.TIF;*.TIFF|"));

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

    ok_int(iBMP, FindFilterItem(strImporters, L"BMP (*.BMP;*.DIB;*.RLE)|*.BMP;*.DIB;*.RLE|"));
    ok_int(iJPEG, FindFilterItem(strImporters, L"JPEG (*.JPG;*.JPEG;*.JPE;*.JFIF)|*.JPG;*.JPEG;*.JPE;*.JFIF|"));
    ok_int(iGIF, FindFilterItem(strImporters, L"GIF (*.GIF)|*.GIF|"));
    ok_int(iPNG, FindFilterItem(strImporters, L"PNG (*.PNG)|*.PNG|"));
    ok_int(iTIFF, FindFilterItem(strImporters, L"TIFF (*.TIF;*.TIFF)|*.TIF;*.TIFF|"));
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

    ok_int(iBMP, FindFilterItem(strExporters, L"BMP (*.BMP;*.DIB;*.RLE)|*.BMP;*.DIB;*.RLE|"));
    ok_int(iJPEG, FindFilterItem(strExporters, L"JPEG (*.JPG;*.JPEG;*.JPE;*.JFIF)|*.JPG;*.JPEG;*.JPE;*.JFIF|"));
    ok_int(iGIF, FindFilterItem(strExporters, L"GIF (*.GIF)|*.GIF|"));
    ok_int(iPNG, FindFilterItem(strExporters, L"PNG (*.PNG)|*.PNG|"));
    ok_int(iTIFF, FindFilterItem(strExporters, L"TIFF (*.TIF;*.TIFF)|*.TIF;*.TIFF|"));

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

    ok_int(iBMP, FindFilterItem(strExporters, L"BMP (*.BMP;*.DIB;*.RLE)|*.BMP;*.DIB;*.RLE|"));
    ok_int(iJPEG, FindFilterItem(strExporters, L"JPEG (*.JPG;*.JPEG;*.JPE;*.JFIF)|*.JPG;*.JPEG;*.JPE;*.JFIF|"));
    ok_int(iGIF, FindFilterItem(strExporters, L"GIF (*.GIF)|*.GIF|"));
    ok_int(iPNG, FindFilterItem(strExporters, L"PNG (*.PNG)|*.PNG|"));
    ok_int(iTIFF, FindFilterItem(strExporters, L"TIFF (*.TIF;*.TIFF)|*.TIF;*.TIFF|"));
}

START_TEST(CImage)
{
    Test_LoadSaveImage();
    Test_Importer();
    Test_Exporter();
}
