/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Some DIB related functions
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#include "precomp.h"

INT g_fileSize = 0;
float g_xDpi = 96;
float g_yDpi = 96;
SYSTEMTIME g_fileTime;

#define WIDTHBYTES(i) (((i) + 31) / 32 * 4)

struct BITMAPINFOEX : BITMAPINFO
{
    RGBQUAD bmiColorsExtra[256 - 1];
};

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
    if (!::GetObjectW(hbm, sizeof(bm), &bm))
        return NULL;

    if (cx == 0 || cy == 0)
    {
        cx = bm.bmWidth;
        cy = bm.bmHeight;
    }

    HBITMAP hbmNew = ::CreateBitmap(cx, cy, 1, 1, NULL);
    if (!hbmNew)
        return NULL;

    HDC hdc1 = ::CreateCompatibleDC(NULL);
    HDC hdc2 = ::CreateCompatibleDC(NULL);
    HGDIOBJ hbm1Old = ::SelectObject(hdc1, hbm);
    HGDIOBJ hbm2Old = ::SelectObject(hdc2, hbmNew);
    ::StretchBlt(hdc2, 0, 0, cx, cy, hdc1, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
    ::SelectObject(hdc1, hbm1Old);
    ::SelectObject(hdc2, hbm2Old);
    ::DeleteDC(hdc1);
    ::DeleteDC(hdc2);
    return hbmNew;
}

HBITMAP CachedBufferDIB(HBITMAP hbm, int minimalWidth, int minimalHeight)
{
    if (minimalWidth <= 0)
        minimalWidth = 1;
    if (minimalHeight <= 0)
        minimalHeight = 1;

    BITMAP bm;
    if (!GetObjectW(hbm, sizeof(bm), &bm))
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
    ::GetObjectW(hBitmap, sizeof(BITMAP), &bm);
    return bm.bmWidth;
}

int
GetDIBHeight(HBITMAP hBitmap)
{
    BITMAP bm;
    ::GetObjectW(hBitmap, sizeof(BITMAP), &bm);
    return bm.bmHeight;
}

BOOL SaveDIBToFile(HBITMAP hBitmap, LPCWSTR FileName, BOOL fIsMainFile, REFGUID guidFileType)
{
    CWaitCursor waitCursor;

    CImageDx img;
    img.Attach(hBitmap);
    HRESULT hr = img.SaveDx(FileName, guidFileType, g_xDpi, g_yDpi);
    img.Detach();

    if (FAILED(hr))
    {
        ShowError(IDS_SAVEERROR, FileName);
        return FALSE;
    }

    if (!fIsMainFile)
        return TRUE;

    WIN32_FIND_DATAW find;
    HANDLE hFind = ::FindFirstFileW(FileName, &find);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        ShowError(IDS_SAVEERROR, FileName);
        return FALSE;
    }
    ::FindClose(hFind);

    SetFileInfo(FileName, &find, TRUE);
    g_imageSaved = TRUE;
    return TRUE;
}

void SetFileInfo(LPCWSTR name, LPWIN32_FIND_DATAW pFound, BOOL isAFile)
{
    // update file time and size
    if (pFound)
    {
        FILETIME ft;
        ::FileTimeToLocalFileTime(&pFound->ftLastWriteTime, &ft);
        ::FileTimeToSystemTime(&ft, &g_fileTime);

        g_fileSize = pFound->nFileSizeLow;
    }
    else
    {
        ZeroMemory(&g_fileTime, sizeof(g_fileTime));
        g_fileSize = 0;
    }

    // update g_szFileName
    if (name && name[0])
    {
        CStringW strName = name;
        ::GetFullPathNameW(strName, _countof(g_szFileName), g_szFileName, NULL);
        // The following code won't work correctly when (name == g_szFileName):
        //   ::GetFullPathNameW(name, _countof(g_szFileName), g_szFileName, NULL);
    }
    else
    {
        ::LoadStringW(g_hinstExe, IDS_DEFAULTFILENAME, g_szFileName, _countof(g_szFileName));
    }

    // set title
    CStringW strTitle;
    strTitle.Format(IDS_WINDOWTITLE, PathFindFileNameW(g_szFileName));
    mainWindow.SetWindowText(strTitle);

    // update file info and recent
    g_isAFile = isAFile;
    if (g_isAFile)
        registrySettings.SetMostRecentFile(g_szFileName);

    g_imageSaved = TRUE;
}

HBITMAP InitializeImage(LPCWSTR name, LPWIN32_FIND_DATAW pFound, BOOL isFile)
{
    COLORREF white = RGB(255, 255, 255);
    HBITMAP hBitmap = CreateColorDIB(registrySettings.BMPWidth, registrySettings.BMPHeight, white);
    if (hBitmap == NULL)
    {
        ShowOutOfMemory();
        return NULL;
    }

    HDC hScreenDC = ::GetDC(NULL);
    g_xDpi = (float)::GetDeviceCaps(hScreenDC, LOGPIXELSX);
    g_yDpi = (float)::GetDeviceCaps(hScreenDC, LOGPIXELSY);
    ::ReleaseDC(NULL, hScreenDC);

    return SetBitmapAndInfo(hBitmap, name, pFound, isFile);
}

HBITMAP SetBitmapAndInfo(HBITMAP hBitmap, LPCWSTR name, LPWIN32_FIND_DATAW pFound, BOOL isFile)
{
    // update image
    canvasWindow.updateScrollPos();
    imageModel.PushImageForUndo(hBitmap);
    imageModel.ClearHistory();

    SetFileInfo(name, pFound, isFile);
    g_imageSaved = TRUE;
    return hBitmap;
}

HBITMAP DoLoadImageFile(HWND hwnd, LPCWSTR name, BOOL fIsMainFile)
{
    CWaitCursor waitCursor;

    // find the file
    WIN32_FIND_DATAW find;
    HANDLE hFind = ::FindFirstFileW(name, &find);
    if (hFind == INVALID_HANDLE_VALUE) // does not exist
    {
        ShowError(IDS_LOADERRORTEXT, name);
        return NULL;
    }
    ::FindClose(hFind);

    // is file empty?
    if (find.nFileSizeLow == 0 && find.nFileSizeHigh == 0)
    {
        if (fIsMainFile)
            return InitializeImage(name, &find, TRUE);
    }

    // load the image
    CImageDx img;
    float xDpi = 0, yDpi = 0;
    HRESULT hr = img.LoadDx(name, &xDpi, &yDpi);
    if (FAILED(hr) && fIsMainFile)
    {
        imageModel.ClearHistory();
        hr = img.LoadDx(name, &xDpi, &yDpi);
    }
    if (FAILED(hr))
    {
        ATLTRACE("hr: 0x%08lX\n", hr);
        ShowError(IDS_LOADERRORTEXT, name);
        return NULL;
    }

    HBITMAP hBitmap = img.Detach();
    if (!fIsMainFile)
        return hBitmap;

    if (xDpi <= 0 || yDpi <= 0)
    {
        HDC hDC = ::GetDC(NULL);
        xDpi = (float)::GetDeviceCaps(hDC, LOGPIXELSX);
        yDpi = (float)::GetDeviceCaps(hDC, LOGPIXELSY);
        ::ReleaseDC(NULL, hDC);
    }

    g_xDpi = xDpi;
    g_yDpi = yDpi;

    SetBitmapAndInfo(hBitmap, name, &find, TRUE);
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

HBITMAP SkewDIB(HDC hDC1, HBITMAP hbm, INT nDegree, BOOL bVertical, BOOL bMono)
{
    CWaitCursor waitCursor;

    if (nDegree == 0)
        return CopyDIBImage(hbm);

    const double eTan = tan(abs(nDegree) * M_PI / 180);

    BITMAP bm;
    ::GetObjectW(hbm, sizeof(bm), &bm);
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
                ::BitBlt(hDC2, x, (dy - delta), 1, cy, hDC1, x, 0, SRCCOPY);
            else
                ::BitBlt(hDC2, x, delta, 1, cy, hDC1, x, 0, SRCCOPY);
        }
    }
    else
    {
        for (INT y = 0; y < cy; ++y)
        {
            INT delta = INT(y * eTan);
            if (nDegree > 0)
                ::BitBlt(hDC2, (dx - delta), y, cx, 1, hDC1, 0, y, SRCCOPY);
            else
                ::BitBlt(hDC2, delta, y, cx, 1, hDC1, 0, y, SRCCOPY);
        }
    }

    SelectObject(hDC2, hbm2Old);
    DeleteDC(hDC2);
    return hbmNew;
}

HBITMAP getSubImage(HBITMAP hbmWhole, const RECT& rcPartial)
{
    CRect rc = rcPartial;
    HBITMAP hbmPart = CreateDIBWithProperties(rc.Width(), rc.Height());
    if (!hbmPart)
        return NULL;

    HDC hDC1 = ::CreateCompatibleDC(NULL);
    HDC hDC2 = ::CreateCompatibleDC(NULL);
    HGDIOBJ hbm1Old = ::SelectObject(hDC1, hbmWhole);
    HGDIOBJ hbm2Old = ::SelectObject(hDC2, hbmPart);
    ::BitBlt(hDC2, 0, 0, rc.Width(), rc.Height(), hDC1, rc.left, rc.top, SRCCOPY);
    ::SelectObject(hDC1, hbm1Old);
    ::SelectObject(hDC2, hbm2Old);
    ::DeleteDC(hDC1);
    ::DeleteDC(hDC2);
    return hbmPart;
}

void putSubImage(HBITMAP hbmWhole, const RECT& rcPartial, HBITMAP hbmPart)
{
    CRect rc = rcPartial;
    HDC hDC1 = ::CreateCompatibleDC(NULL);
    HDC hDC2 = ::CreateCompatibleDC(NULL);
    HGDIOBJ hbm1Old = ::SelectObject(hDC1, hbmWhole);
    HGDIOBJ hbm2Old = ::SelectObject(hDC2, hbmPart);
    ::BitBlt(hDC1, rc.left, rc.top, rc.Width(), rc.Height(), hDC2, 0, 0, SRCCOPY);
    ::SelectObject(hDC1, hbm1Old);
    ::SelectObject(hDC2, hbm2Old);
    ::DeleteDC(hDC1);
    ::DeleteDC(hDC2);
}

struct BITMAPINFODX : BITMAPINFO
{
    RGBQUAD bmiColorsAdditional[256 - 1];
};

HGLOBAL BitmapToClipboardDIB(HBITMAP hBitmap)
{
    CWaitCursor waitCursor;

    BITMAP bm;
    if (!GetObjectW(hBitmap, sizeof(BITMAP), &bm))
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
    CWaitCursor waitCursor;

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
    CWaitCursor waitCursor;

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

BOOL IsBitmapBlackAndWhite(HBITMAP hbm)
{
    CWaitCursor waitCursor;

    BITMAP bm;
    if (!::GetObjectW(hbm, sizeof(bm), &bm))
        return FALSE;

    if (bm.bmBitsPixel == 1)
        return TRUE;

    BITMAPINFOEX bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = bm.bmWidth;
    bmi.bmiHeader.biHeight = bm.bmHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;

    DWORD widthbytes = WIDTHBYTES(24 * bm.bmWidth);
    DWORD cbBits = widthbytes * bm.bmHeight;
    LPBYTE pbBits = new BYTE[cbBits];

    HDC hdc = ::CreateCompatibleDC(NULL);
    ::GetDIBits(hdc, hbm, 0, bm.bmHeight, pbBits, &bmi, DIB_RGB_COLORS);
    ::DeleteDC(hdc);

    BOOL bBlackAndWhite = TRUE;
    for (LONG y = 0; y < bm.bmHeight; ++y)
    {
        LPBYTE pbLine = &pbBits[widthbytes * y];
        for (LONG x = 0; x < bm.bmWidth; ++x)
        {
            BYTE Blue = *pbLine++;
            BYTE Green = *pbLine++;
            BYTE Red = *pbLine++;
            COLORREF rgbColor = RGB(Red, Green, Blue);
            if (rgbColor != RGB(0, 0, 0) && rgbColor != RGB(255, 255, 255))
            {
                bBlackAndWhite = FALSE;
                goto Finish;
            }
        }
    }

Finish:
    delete[] pbBits;

    return bBlackAndWhite;
}

HBITMAP ConvertToBlackAndWhite(HBITMAP hbm)
{
    CWaitCursor waitCursor;

    BITMAP bm;
    if (!::GetObjectW(hbm, sizeof(bm), &bm))
        return NULL;

    BITMAPINFOEX bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bm.bmWidth;
    bmi.bmiHeader.biHeight = bm.bmHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 1;
    bmi.bmiColors[1].rgbBlue = 255;
    bmi.bmiColors[1].rgbGreen = 255;
    bmi.bmiColors[1].rgbRed = 255;
    HDC hdc = ::CreateCompatibleDC(NULL);
    LPVOID pvMonoBits;
    HBITMAP hMonoBitmap = ::CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvMonoBits, NULL, 0);
    if (!hMonoBitmap)
    {
        ::DeleteDC(hdc);
        return NULL;
    }

    HBITMAP hNewBitmap = CreateDIBWithProperties(bm.bmWidth, bm.bmHeight);
    if (hNewBitmap)
    {
        ::GetDIBits(hdc, hbm, 0, bm.bmHeight, pvMonoBits, &bmi, DIB_RGB_COLORS);
        ::SetDIBits(hdc, hNewBitmap, 0, bm.bmHeight, pvMonoBits, &bmi, DIB_RGB_COLORS);
    }
    ::DeleteObject(hMonoBitmap);
    ::DeleteDC(hdc);

    return hNewBitmap;
}
