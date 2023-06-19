/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/dib.cpp
 * PURPOSE:     Some DIB related functions
 * PROGRAMMERS: Benedikt Freisen
 */

#include "precomp.h"
#include <math.h>

INT g_fileSize = 0;
float g_xDpi = 96;
float g_yDpi = 96;
SYSTEMTIME g_fileTime;

/* FUNCTIONS ********************************************************/

// Convert DPI (dots per inch) into PPCM (pixels per centimeter)
float PpcmFromDpi(float dpi)
{
    // 1 DPI is 0.0254 meter. 1 centimeter is 1/100 meter.
    return dpi / (0.0254f * 100.0f);
}

HBITMAP
CreateDIBWithProperties(int width, int height)
{
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    return CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
}

HBITMAP
CreateMonoBitmap(int width, int height, BOOL bWhite)
{
    HBITMAP hbm = CreateBitmap(width, height, 1, 1, NULL);
    if (hbm == NULL)
        return NULL;

    if (bWhite)
    {
        HDC hdc = CreateCompatibleDC(NULL);
        HGDIOBJ hbmOld = SelectObject(hdc, hbm);
        RECT rc = { 0, 0, width, height };
        FillRect(hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
        SelectObject(hdc, hbmOld);
        DeleteDC(hdc);
    }

    return hbm;
}

HBITMAP
CreateColorDIB(int width, int height, COLORREF rgb)
{
    HBITMAP ret = CreateDIBWithProperties(width, height);
    if (!ret)
        return NULL;

    if (rgb)
    {
        HDC hdc = CreateCompatibleDC(NULL);
        HGDIOBJ hbmOld = SelectObject(hdc, ret);
        RECT rc;
        SetRect(&rc, 0, 0, width, height);
        HBRUSH hbr = CreateSolidBrush(rgb);
        FillRect(hdc, &rc, hbr);
        DeleteObject(hbr);
        SelectObject(hdc, hbmOld);
        DeleteDC(hdc);
    }

    return ret;
}

HBITMAP CopyMonoImage(HBITMAP hbm, INT cx, INT cy)
{
    BITMAP bm;
    if (!GetObject(hbm, sizeof(bm), &bm))
        return NULL;

    if (cx == 0 || cy == 0)
    {
        cx = bm.bmWidth;
        cy = bm.bmHeight;
    }

    HBITMAP hbmNew = CreateBitmap(cx, cy, 1, 1, NULL);
    if (!hbmNew)
        return NULL;

    HDC hdc1 = CreateCompatibleDC(NULL);
    HDC hdc2 = CreateCompatibleDC(NULL);
    HGDIOBJ hbm1Old = SelectObject(hdc1, hbm);
    HGDIOBJ hbm2Old = SelectObject(hdc2, hbmNew);
    StretchBlt(hdc2, 0, 0, cx, cy, hdc1, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
    SelectObject(hdc1, hbm1Old);
    SelectObject(hdc2, hbm2Old);
    DeleteDC(hdc1);
    DeleteDC(hdc2);
    return hbmNew;
}

HBITMAP CachedBufferDIB(HBITMAP hbm, int minimalWidth, int minimalHeight)
{
    if (minimalWidth <= 0)
        minimalWidth = 1;
    if (minimalHeight <= 0)
        minimalHeight = 1;

    BITMAP bm;
    if (!GetObject(hbm, sizeof(bm), &bm))
        hbm = NULL;

    if (hbm && minimalWidth <= bm.bmWidth && minimalHeight <= bm.bmHeight)
        return hbm;

    if (hbm)
        DeleteObject(hbm);

    return CreateDIBWithProperties((minimalWidth * 3) / 2, (minimalHeight * 3) / 2);
}

int
GetDIBWidth(HBITMAP hBitmap)
{
    BITMAP bm;
    GetObject(hBitmap, sizeof(BITMAP), &bm);
    return bm.bmWidth;
}

int
GetDIBHeight(HBITMAP hBitmap)
{
    BITMAP bm;
    GetObject(hBitmap, sizeof(BITMAP), &bm);
    return bm.bmHeight;
}

BOOL SaveDIBToFile(HBITMAP hBitmap, LPCTSTR FileName, HDC hDC)
{
    CImageDx img;
    img.Attach(hBitmap);
    img.SaveDx(FileName, GUID_NULL, g_xDpi, g_yDpi); // TODO: error handling
    img.Detach();

    WIN32_FIND_DATA find;
    HANDLE hFind = FindFirstFile(FileName, &find);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        ShowFileLoadError(FileName);
        return FALSE;
    }
    FindClose(hFind);

    // update time and size
    FILETIME ft;
    FileTimeToLocalFileTime(&find.ftLastWriteTime, &ft);
    FileTimeToSystemTime(&ft, &g_fileTime);
    g_fileSize = find.nFileSizeLow;

    // TODO: update hRes and vRes

    registrySettings.SetMostRecentFile(FileName);

    g_isAFile = TRUE;
    g_imageSaved = TRUE;
    return TRUE;
}

void ShowFileLoadError(LPCTSTR name)
{
    CString strText;
    strText.Format(IDS_LOADERRORTEXT, (LPCTSTR) name);
    CString strProgramName;
    strProgramName.LoadString(IDS_PROGRAMNAME);
    mainWindow.MessageBox(strText, strProgramName, MB_OK | MB_ICONEXCLAMATION);
}

HBITMAP SetBitmapAndInfo(HBITMAP hBitmap, LPCTSTR name, DWORD dwFileSize, BOOL isFile)
{
    if (hBitmap == NULL)
    {
        COLORREF white = RGB(255, 255, 255);
        hBitmap = CreateColorDIB(registrySettings.BMPWidth,
                                 registrySettings.BMPHeight, white);
        if (hBitmap == NULL)
            return FALSE;

        HDC hScreenDC = GetDC(NULL);
        g_xDpi = GetDeviceCaps(hScreenDC, LOGPIXELSX);
        g_yDpi = GetDeviceCaps(hScreenDC, LOGPIXELSY);
        ReleaseDC(NULL, hScreenDC);

        ZeroMemory(&g_fileTime, sizeof(g_fileTime));
    }

    // update image
    imageModel.PushImageForUndo(hBitmap);
    imageModel.ClearHistory();

    // update g_fileSize
    g_fileSize = dwFileSize;

    // update g_szFileName
    if (name && name[0])
        GetFullPathName(name, _countof(g_szFileName), g_szFileName, NULL);
    else
        LoadString(g_hinstExe, IDS_DEFAULTFILENAME, g_szFileName, _countof(g_szFileName));

    // set title
    CString strTitle;
    strTitle.Format(IDS_WINDOWTITLE, PathFindFileName(g_szFileName));
    mainWindow.SetWindowText(strTitle);

    // update file info and recent
    g_isAFile = isFile;
    if (g_isAFile)
        registrySettings.SetMostRecentFile(g_szFileName);

    g_imageSaved = TRUE;

    return hBitmap;
}

HBITMAP DoLoadImageFile(HWND hwnd, LPCTSTR name, BOOL fIsMainFile)
{
    // find the file
    WIN32_FIND_DATA find;
    HANDLE hFind = FindFirstFile(name, &find);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        // does not exist
        CStringW strText;
        strText.Format(IDS_LOADERRORTEXT, name);
        MessageBoxW(hwnd, strText, NULL, MB_ICONERROR);
        return NULL;
    }
    DWORD dwFileSize = find.nFileSizeLow; // get file size
    FindClose(hFind);

    // is file empty?
    if (dwFileSize == 0)
    {
        if (fIsMainFile)
        {
            FILETIME ft;
            FileTimeToLocalFileTime(&find.ftLastWriteTime, &ft);
            FileTimeToSystemTime(&ft, &g_fileTime);
            return SetBitmapAndInfo(NULL, name, dwFileSize, TRUE);
        }
    }

    // load the image
    CImageDx img;
    img.LoadDx(name, &g_xDpi, &g_yDpi);

    if (g_xDpi <= 0)
        g_xDpi = 96;
    if (g_yDpi <= 0)
        g_yDpi = 96;

    HBITMAP hBitmap = img.Detach();

    if (hBitmap == NULL)
    {
        // cannot open
        CStringW strText;
        strText.Format(IDS_LOADERRORTEXT, name);
        MessageBoxW(hwnd, strText, NULL, MB_ICONERROR);
        return NULL;
    }

    if (fIsMainFile)
    {
        FILETIME ft;
        FileTimeToLocalFileTime(&find.ftLastWriteTime, &ft);
        FileTimeToSystemTime(&ft, &g_fileTime);
        SetBitmapAndInfo(hBitmap, name, dwFileSize, TRUE);
    }

    return hBitmap;
}

HBITMAP Rotate90DegreeBlt(HDC hDC1, INT cx, INT cy, BOOL bRight, BOOL bMono)
{
    HBITMAP hbm2;
    if (bMono)
        hbm2 = ::CreateBitmap(cy, cx, 1, 1, NULL);
    else
        hbm2 = CreateDIBWithProperties(cy, cx);
    if (!hbm2)
        return NULL;

    HDC hDC2 = CreateCompatibleDC(NULL);
    HGDIOBJ hbm2Old = SelectObject(hDC2, hbm2);
    if (bRight)
    {
        for (INT y = 0; y < cy; ++y)
        {
            for (INT x = 0; x < cx; ++x)
            {
                COLORREF rgb = GetPixel(hDC1, x, y);
                SetPixelV(hDC2, cy - (y + 1), x, rgb);
            }
        }
    }
    else
    {
        for (INT y = 0; y < cy; ++y)
        {
            for (INT x = 0; x < cx; ++x)
            {
                COLORREF rgb = GetPixel(hDC1, x, y);
                SetPixelV(hDC2, y, cx - (x + 1), rgb);
            }
        }
    }
    SelectObject(hDC2, hbm2Old);
    DeleteDC(hDC2);
    return hbm2;
}

#ifndef M_PI
    #define M_PI 3.14159265
#endif

HBITMAP SkewDIB(HDC hDC1, HBITMAP hbm, INT nDegree, BOOL bVertical, BOOL bMono)
{
    if (nDegree == 0)
        return CopyDIBImage(hbm);

    const double eTan = tan(abs(nDegree) * M_PI / 180);

    BITMAP bm;
    GetObjectW(hbm, sizeof(bm), &bm);
    INT cx = bm.bmWidth, cy = bm.bmHeight, dx = 0, dy = 0;
    if (bVertical)
        dy = INT(cx * eTan);
    else
        dx = INT(cy * eTan);

    if (dx == 0 && dy == 0)
        return CopyDIBImage(hbm);

    HBITMAP hbmNew;
    if (bMono)
        hbmNew = CreateMonoBitmap(cx + dx, cy + dy, FALSE);
    else
        hbmNew = CreateColorDIB(cx + dx, cy + dy, RGB(255, 255, 255));
    if (!hbmNew)
        return NULL;

    HDC hDC2 = CreateCompatibleDC(NULL);
    HGDIOBJ hbm2Old = SelectObject(hDC2, hbmNew);
    if (bVertical)
    {
        for (INT x = 0; x < cx; ++x)
        {
            INT delta = INT(x * eTan);
            if (nDegree > 0)
                BitBlt(hDC2, x, (dy - delta), 1, cy, hDC1, x, 0, SRCCOPY);
            else
                BitBlt(hDC2, x, delta, 1, cy, hDC1, x, 0, SRCCOPY);
        }
    }
    else
    {
        for (INT y = 0; y < cy; ++y)
        {
            INT delta = INT(y * eTan);
            if (nDegree > 0)
                BitBlt(hDC2, (dx - delta), y, cx, 1, hDC1, 0, y, SRCCOPY);
            else
                BitBlt(hDC2, delta, y, cx, 1, hDC1, 0, y, SRCCOPY);
        }
    }

    SelectObject(hDC2, hbm2Old);
    DeleteDC(hDC2);
    return hbmNew;
}

struct BITMAPINFODX : BITMAPINFO
{
    RGBQUAD bmiColorsAdditional[256 - 1];
};

HGLOBAL BitmapToClipboardDIB(HBITMAP hBitmap)
{
    BITMAP bm;
    if (!GetObject(hBitmap, sizeof(BITMAP), &bm))
        return NULL;

    BITMAPINFODX bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bm.bmWidth;
    bmi.bmiHeader.biHeight = bm.bmHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = bm.bmBitsPixel;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = bm.bmWidthBytes * bm.bmHeight;

    INT cColors;
    if (bm.bmBitsPixel < 16)
        cColors = 1 << bm.bmBitsPixel;
    else
        cColors = 0;

    HDC hDC = CreateCompatibleDC(NULL);

    if (cColors)
    {
        HGDIOBJ hbmOld = SelectObject(hDC, hBitmap);
        cColors = GetDIBColorTable(hDC, 0, cColors, bmi.bmiColors);
        SelectObject(hDC, hbmOld);
    }

    DWORD cbColors = cColors * sizeof(RGBQUAD);
    DWORD dwSize = sizeof(BITMAPINFOHEADER) + cbColors + bmi.bmiHeader.biSizeImage;
    HGLOBAL hGlobal = GlobalAlloc(GHND | GMEM_SHARE, dwSize);
    if (hGlobal)
    {
        LPBYTE pb = (LPBYTE)GlobalLock(hGlobal);
        if (pb)
        {
            CopyMemory(pb, &bmi, sizeof(BITMAPINFOHEADER));
            pb += sizeof(BITMAPINFOHEADER);

            CopyMemory(pb, bmi.bmiColors, cbColors);
            pb += cbColors;

            GetDIBits(hDC, hBitmap, 0, bm.bmHeight, pb, &bmi, DIB_RGB_COLORS);

            GlobalUnlock(hGlobal);
        }
        else
        {
            GlobalFree(hGlobal);
            hGlobal = NULL;
        }
    }

    DeleteDC(hDC);

    return hGlobal;
}

HBITMAP BitmapFromClipboardDIB(HGLOBAL hGlobal)
{
    LPBYTE pb = (LPBYTE)GlobalLock(hGlobal);
    if (!pb)
        return NULL;

    LPBITMAPINFO pbmi = (LPBITMAPINFO)pb;
    pb += pbmi->bmiHeader.biSize;

    INT cColors = 0, cbColors = 0;
    if (pbmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        LPBITMAPCOREINFO pbmci = (LPBITMAPCOREINFO)pbmi;
        WORD BitCount = pbmci->bmciHeader.bcBitCount;
        if (BitCount < 16)
        {
            cColors = (1 << BitCount);
            cbColors = cColors * sizeof(RGBTRIPLE);
            pb += cbColors;
        }
    }
    else if (pbmi->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER))
    {
        WORD BitCount = pbmi->bmiHeader.biBitCount;
        if (BitCount < 16)
        {
            cColors = (1 << BitCount);
            cbColors = cColors * sizeof(RGBQUAD);
            pb += cbColors;
        }
    }

    HDC hDC = CreateCompatibleDC(NULL);
    HBITMAP hBitmap = CreateDIBSection(hDC, pbmi, DIB_RGB_COLORS, NULL, NULL, 0);
    if (hBitmap)
    {
        SetDIBits(hDC, hBitmap, 0, labs(pbmi->bmiHeader.biHeight), pb, pbmi, DIB_RGB_COLORS);
    }
    DeleteDC(hDC);

    GlobalUnlock(hGlobal);

    return hBitmap;
}

HBITMAP BitmapFromHEMF(HENHMETAFILE hEMF)
{
    ENHMETAHEADER header;
    if (!GetEnhMetaFileHeader(hEMF, sizeof(header), &header))
        return NULL;

    CRect rc = *(LPRECT)&header.rclBounds;
    INT cx = rc.Width(), cy = rc.Height();
    HBITMAP hbm = CreateColorDIB(cx, cy, RGB(255, 255, 255));
    if (!hbm)
        return NULL;

    HDC hDC = CreateCompatibleDC(NULL);
    HGDIOBJ hbmOld = SelectObject(hDC, hbm);
    PlayEnhMetaFile(hDC, hEMF, &rc);
    SelectObject(hDC, hbmOld);
    DeleteDC(hDC);

    return hbm;
}
