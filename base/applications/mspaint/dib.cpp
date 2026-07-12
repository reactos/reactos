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

COLORREF QuadToRGBValue(const RGBQUAD& quad)
{
    return RGB(quad.rgbRed, quad.rgbGreen, quad.rgbBlue);
}

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
        FillDIBByColor(hbm, RGB(255, 255, 255));

    return hbm;
}

HBITMAP
CreateColorDIB(int width, int height, COLORREF rgb)
{
    HBITMAP ret = CreateDIBWithProperties(width, height);
    if (!ret)
        return NULL;

    if (rgb)
        FillDIBByColor(ret, rgb);

    return ret;
}

HBITMAP CopyMonoImage(HBITMAP hbm, INT cx, INT cy, INT stretchMode)
{
    BITMAP bm;
    if (!::GetObjectW(hbm, sizeof(bm), &bm))
        return NULL;

    if (cx == 0 || cy == 0)
    {
        cx = bm.bmWidth;
        cy = bm.bmHeight;
    }

    HDC hdc1 = ::CreateCompatibleDC(NULL);
    HDC hdc2 = ::CreateCompatibleDC(NULL);
    HBITMAP hbmNew = ::CreateBitmap(cx, cy, 1, 1, NULL);
    HGDIOBJ hbm1Old = ::SelectObject(hdc1, hbm);
    HGDIOBJ hbm2Old = ::SelectObject(hdc2, hbmNew);
    ::SetStretchBltMode(hdc2, stretchMode);
    ::StretchBlt(hdc2, 0, 0, cx, cy, hdc1, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
    ::SelectObject(hdc1, hbm1Old);
    ::SelectObject(hdc2, hbm2Old);
    ::DeleteDC(hdc1);
    ::DeleteDC(hdc2);
    return hbmNew;
}

HBITMAP CopyDIBImage(HBITMAP hbm, INT cx, INT cy, INT stretchMode, COLORREF rgbColor)
{
    HBITMAP hbmNew = (HBITMAP)CopyImage(hbm, IMAGE_BITMAP, cx, cy, LR_CREATEDIBSECTION);
    if (!hbmNew)
        return NULL;

    if (stretchMode == STRETCH_HALFTONE)
    {
        FillDIBByColor(hbmNew, rgbColor);
        return hbmNew;
    }

    BITMAP bm;
    if (!::GetObjectW(hbm, sizeof(bm), &bm))
        return NULL;

    if (cx == 0 || cy == 0)
    {
        cx = bm.bmWidth;
        cy = bm.bmHeight;
    }

    HDC hdc1 = ::CreateCompatibleDC(NULL);
    HDC hdc2 = ::CreateCompatibleDC(NULL);
    HGDIOBJ hbm1Old = ::SelectObject(hdc1, hbm);
    HGDIOBJ hbm2Old = ::SelectObject(hdc2, hbmNew);
    ::SetStretchBltMode(hdc2, stretchMode);
    ::StretchBlt(hdc2, 0, 0, cx, cy, hdc1, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
    ::SelectObject(hdc1, hbm1Old);
    ::SelectObject(hdc2, hbm2Old);
    ::DeleteDC(hdc1);
    ::DeleteDC(hdc2);

    FillDIBByColor(hbmNew, rgbColor);
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
        imageModel.ClearHistory();
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
    paletteModel.SetColorInfo(hBitmap);

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

    LPCWSTR pchDotExt = PathFindExtensionW(name);
    BOOL bBMP = !lstrcmpiW(pchDotExt, L".bmp") || !lstrcmpiW(pchDotExt, L".dib");

    // load the image
    HBITMAP hBitmap;
    float xDpi = 0, yDpi = 0;
    HRESULT hr;
    if (bBMP)
    {
        hr = LoadBitmapFromFile(&hBitmap, name, &xDpi, &yDpi);
        if (FAILED(hr) && fIsMainFile)
        {
            imageModel.ClearHistory();
            hr = LoadBitmapFromFile(&hBitmap, name, &xDpi, &yDpi);
        }
    }
    else
    {
        CImageDx img;
        hr = img.LoadDx(name, &xDpi, &yDpi);
        if (FAILED(hr) && fIsMainFile)
        {
            imageModel.ClearHistory();
            hr = img.LoadDx(name, &xDpi, &yDpi);
        }
        hBitmap = img.Detach();
    }

    if (FAILED(hr))
    {
        ATLTRACE("hr: 0x%08lX\n", hr);
        ShowError(IDS_LOADERRORTEXT, name);
        return NULL;
    }

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

HBITMAP Rotate90DegreeBitmap(HBITMAP hbm, BOOL bRight)
{
    BITMAP bm;
    if (!GetObjectW(hbm, sizeof(bm), &bm))
        return NULL;

    const LONG cx = bm.bmWidth, cy = bm.bmHeight;
    HBITMAP hbm2 = CopyDIBImage(hbm, cy, cx, STRETCH_DELETESCANS);
    if (!hbm2)
        return NULL;

    HDC hDC1 = CreateCompatibleDC(NULL);
    HDC hDC2 = CreateCompatibleDC(NULL);
    HGDIOBJ hbm1Old = SelectObject(hDC1, hbm);
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
    SelectObject(hDC1, hbm1Old);
    DeleteDC(hDC2);
    DeleteDC(hDC1);
    return hbm2;
}

HBITMAP SkewDIB(HDC hDC1, HBITMAP hbm, INT nDegree, BOOL bVertical)
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

    HBITMAP hbmNew = CopyDIBImage(hbm, cx + dx, cy + dy, STRETCH_DELETESCANS,
                                  paletteModel.GetBgColor());
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
    HBITMAP hbmPart = CopyDIBImage(hbmWhole, rc.Width(), rc.Height());
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

void FillDIBByColor(HBITMAP hbm, COLORREF rgbColor)
{
    BITMAP bm;
    if (rgbColor == CLR_INVALID || !GetObjectW(hbm, sizeof(bm), &bm))
        return;

    HDC hDC = CreateCompatibleDC(NULL);
    HGDIOBJ hbmOld = SelectObject(hDC, hbm);
    HBRUSH hbr = CreateSolidBrush(rgbColor);
    HGDIOBJ hbrOld = SelectObject(hDC, hbr);
    RECT rc = { 0, 0, bm.bmWidth, bm.bmHeight };
    FillRect(hDC, &rc, hbr);
    SelectObject(hDC, hbrOld);
    SelectObject(hDC, hbmOld);
    DeleteDC(hDC);
}

struct BITMAPINFODX : BITMAPINFO
{
    RGBQUAD bmiColorsAdditional[256 - 1];
};

const float INCHES_PER_METER = 0.0254f;

HRESULT LoadBitmapFromFile(HBITMAP* phBitmap, LPCWSTR filename, float* xDpi, float* yDpi)
{
    *phBitmap = nullptr;
    if (!filename)
        return E_UNEXPECTED;

    HANDLE hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    HBITMAP hBitmap = nullptr;

    BITMAPFILEHEADER bfh;
    DWORD bytesRead;
    if (!ReadFile(hFile, &bfh, sizeof(bfh), &bytesRead, nullptr) ||
        bytesRead != sizeof(bfh) || bfh.bfType != 0x4D42)
    {
        CloseHandle(hFile);
        return E_FAIL;
    }

    DWORD headerSize = 0;
    if (!ReadFile(hFile, &headerSize, sizeof(headerSize), &bytesRead, nullptr) ||
        bytesRead != sizeof(headerSize))
    {
        CloseHandle(hFile);
        return E_FAIL;
    }

    SetFilePointer(hFile, sizeof(BITMAPFILEHEADER), nullptr, FILE_BEGIN);

    DWORD infoSize = bfh.bfOffBits - sizeof(BITMAPFILEHEADER);
    PBITMAPINFO pBi = (PBITMAPINFO)malloc(infoSize);
    if (!pBi)
    {
        CloseHandle(hFile);
        return E_OUTOFMEMORY;
    }

    if (!ReadFile(hFile, pBi, infoSize, &bytesRead, nullptr) || bytesRead != infoSize)
    {
        free(pBi);
        CloseHandle(hFile);
        return E_FAIL;
    }

    float localXDpi = 96.0f;
    float localYDpi = 96.0f;
    if (headerSize >= sizeof(BITMAPINFOHEADER))
    {
        PBITMAPINFOHEADER pBih = &(pBi->bmiHeader);
        localXDpi =
            (pBih->biXPelsPerMeter > 0) ? (pBih->biXPelsPerMeter * INCHES_PER_METER) : 96.0f;
        localYDpi =
            (pBih->biYPelsPerMeter > 0) ? (pBih->biYPelsPerMeter * INCHES_PER_METER) : 96.0f;
    }

    if (xDpi)
        *xDpi = localXDpi;
    if (yDpi)
        *yDpi = localYDpi;

    PVOID pBits = nullptr;
    HDC hDC = GetDC(nullptr);
    *phBitmap = hBitmap = CreateDIBSection(hDC, pBi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    ReleaseDC(nullptr, hDC);

    BOOL bOK = FALSE;
    if (hBitmap)
    {
        SetFilePointer(hFile, bfh.bfOffBits, nullptr, FILE_BEGIN);
        DWORD bitsSize = bfh.bfSize - bfh.bfOffBits;
        bOK = ReadFile(hFile, pBits, bitsSize, &bytesRead, nullptr) && bitsSize == bytesRead;
        if (!bOK)
        {
            DeleteObject(hBitmap);
            *phBitmap = NULL;
        }
    }

    free(pBi);
    CloseHandle(hFile);
    return (bOK ? S_OK : E_FAIL);
}

HRESULT SaveBitmapToFile(HBITMAP hBitmap, LPCWSTR filename, float xDpi, float yDpi)
{
    if (!hBitmap || !filename)
        return E_INVALIDARG;

    BITMAP bm;
    if (!GetObject(hBitmap, sizeof(BITMAP), &bm))
        return E_FAIL;

    BITMAPINFOHEADER bih = { 0 };
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = bm.bmWidth;
    bih.biHeight = bm.bmHeight;
    bih.biPlanes = 1;
    bih.biBitCount = bm.bmBitsPixel;
    bih.biCompression = BI_RGB;

    bih.biXPelsPerMeter = static_cast<LONG>(round(xDpi / INCHES_PER_METER));
    bih.biYPelsPerMeter = static_cast<LONG>(round(yDpi / INCHES_PER_METER));

    DWORD rowSize = ((bm.bmWidth * bm.bmBitsPixel + 31) / 32) * 4;
    bih.biSizeImage = rowSize * bm.bmHeight;

    BITMAPFILEHEADER bfh = { 0 };
    bfh.bfType = 0x4D42; // "BM"
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + bih.biSizeImage;

    PVOID pBits = malloc(bih.biSizeImage);
    if (!pBits)
        return E_OUTOFMEMORY;

    HDC hDC = GetDC(nullptr);
    if (!GetDIBits(hDC, hBitmap, 0, bm.bmHeight, pBits, (PBITMAPINFO)&bih, DIB_RGB_COLORS))
    {
        ReleaseDC(nullptr, hDC);
        free(pBits);
        return E_FAIL;
    }
    ReleaseDC(nullptr, hDC);

    HANDLE hFile = CreateFileW(filename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        free(pBits);
        return HRESULT_FROM_WIN32(error);
    }

    DWORD bytesWritten;
    BOOL bOK = WriteFile(hFile, &bfh, sizeof(bfh), &bytesWritten, nullptr) &&
               WriteFile(hFile, &bih, sizeof(bih), &bytesWritten, nullptr) &&
               WriteFile(hFile, pBits, bih.biSizeImage, &bytesWritten, nullptr);
    CloseHandle(hFile);
    free(pBits);

    if (!bOK)
        DeleteFileW(filename);

    return (bOK ? S_OK : E_FAIL);
}

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
    BITMAP bm;
    if (!::GetObjectW(hbm, sizeof(bm), &bm) && (bm.bmBitsPixel != 1))
        return FALSE;

    RGBQUAD colors[2];
    HDC hDC = CreateCompatibleDC(NULL);
    HGDIOBJ hbmOld = SelectObject(hDC, hbm);
    INT ret = GetDIBColorTable(hDC, 0, (UINT)_countof(colors), colors);
    SelectObject(hDC, hbmOld);
    DeleteDC(hDC);

    if (ret != 2)
        return FALSE;

    const COLORREF clr0 = QuadToRGBValue(colors[0]), clr1 = QuadToRGBValue(colors[1]);
    return (clr0 == RGB(0, 0, 0) || clr0 == RGB(255, 255, 255)) &&
           (clr1 == RGB(0, 0, 0) || clr1 == RGB(255, 255, 255));
}

/**
 * @brief Packs a flat index image into a stride-aligned packed-pixel buffer.
 *
 * Converts a per-pixel index array (one byte per pixel) into the packed format
 * required by Windows DIBs: 8 bpp is a straight byte copy, 4 bpp packs two
 * nibbles per byte (high nibble first), and 1 bpp packs eight pixels per byte
 * (MSB first).  The destination buffer is zeroed before packing so unused
 * padding bits are always zero.
 *
 * @param indexImg  Source buffer; one byte per pixel, row-major, no padding.
 * @param W         Image width in pixels.
 * @param H         Image height in pixels.
 * @param nBpp      Bits per pixel of the destination format (1, 4, or 8).
 * @param dstBuf    Destination buffer; must be at least @p dstStride * @p H bytes.
 * @param dstStride Row stride of the destination buffer in bytes (DWORD-aligned).
 */
static void
PackIndexImage(const BYTE* indexImg, SIZE_T W, SIZE_T H, INT nBpp, PBYTE dstBuf, INT dstStride)
{
    ZeroMemory(dstBuf, dstStride * H);

    for (SIZE_T y = 0; y < H; ++y)
    {
        const BYTE* src = indexImg + y * W;
        PBYTE dst = dstBuf + y * dstStride;

        switch (nBpp)
        {
            case 8:
                CopyMemory(dst, src, W);
                break;
            case 4:
                for (SIZE_T x = 0; x < W; ++x)
                {
                    BYTE v = src[x] & 0x0F;
                    if (x & 1)
                        dst[x >> 1] |= v;
                    else
                        dst[x >> 1] |= (v << 4);
                }
                break;
            case 1:
                for (SIZE_T x = 0; x < W; ++x)
                {
                    if (src[x])
                        dst[x >> 3] |= (BYTE)(0x80 >> (x & 7));
                }
                break;
        }
    }
}

/**
 * @brief Fills a palette array with a standard color set for the given bit depth.
 *
 * - 1 bpp: two entries - black and white.
 * - 4 bpp: the 16-color Windows standard palette (RGBQUAD / BGRA order).
 * - 8 bpp: 216-entry 6 x 6 x 6 RGB color cube followed by 40 evenly-spaced
 *          grayscale entries (256 entries total).
 *
 * @param nBpp    Bit depth that determines the palette size (1, 4, or 8).
 * @param palette Output array; must hold at least @c (1 << nBpp) RGBQUAD entries.
 */
static void BuildPalette(INT nBpp, RGBQUAD* palette)
{
    if (nBpp == 1)
    {
        palette[0] = { 0,   0,   0,   0 }; // Black
        palette[1] = { 255, 255, 255, 0 }; // White
    }
    else if (nBpp == 4)
    {
        // Windows standard 16 colors (BGRA)
        static const RGBQUAD win16[16] =
        {
            {   0,   0,   0, 0 }, {   0,   0, 128, 0 }, {   0, 128,   0, 0 }, {   0, 128, 128, 0 },
            { 128,   0,   0, 0 }, { 128,   0, 128, 0 }, { 128, 128,   0, 0 }, { 192, 192, 192, 0 },
            { 128, 128, 128, 0 }, {   0,   0, 255, 0 }, {   0, 255,   0, 0 }, {   0, 255, 255, 0 },
            { 255,   0,   0, 0 }, { 255,   0, 255, 0 }, { 255, 255,   0, 0 }, { 255, 255, 255, 0 },
        };
        for (INT i = 0; i < 16; ++i)
            palette[i] = win16[i];
    }
    else if (nBpp == 8)
    {
        // 6 x 6 x 6 color cubes (216 colors)
        static const BYTE step6[6] = { 0, 51, 102, 153, 204, 255 };
        INT idx = 0;
        for (INT ri = 0; ri < 6; ++ri)
        {
            for (INT gi = 0; gi < 6; ++gi)
            {
                for (INT bi = 0; bi < 6; ++bi)
                {
                    palette[idx++] = { step6[bi], step6[gi], step6[ri], 0 };
                }
            }
        }

        // 40 grayscale colors
        for (INT i = 0; i < 40; ++i, ++idx)
        {
            BYTE v = (BYTE)((i * 255 + 19) / 39); // 0..255
            palette[idx] = { v, v, v, 0 };
        }
    }
}

HBITMAP ConvertToBlackAndWhite(HBITMAP hbm)
{
    return CreateNBppBitmap(hbm, 1);
}

/**
 * @brief Creates a new DIB with a reduced color depth from an existing bitmap.
 *
 * Reads the source bitmap as a 24-bit BGR DIB, optionally applies
 * Floyd-Steinberg dithering to map pixels to a standard palette, packs the
 * result into the target bit depth, and returns a new @c HBITMAP.
 *
 * @param hBitmap  Handle to the source bitmap.  Must not be @c NULL.
 * @param nBpp     Desired bit depth of the output bitmap (1, 4, 8, or 24).
 * @return         Handle to the newly created bitmap, or @c NULL on failure.
 *                 The caller is responsible for destroying the returned handle
 *                 with @c DeleteObject.
 */
HBITMAP CreateNBppBitmap(HBITMAP hBitmap, INT nBpp)
{
    CWaitCursor waitCursor;

    if (!hBitmap)
        return NULL;

    if (nBpp == 32)
        nBpp = 24; // We don't support 32-bpp

    if (nBpp != 1 && nBpp != 4 && nBpp != 8 && nBpp != 24)
        return NULL;

    BITMAP bm;
    if (!GetObject(hBitmap, sizeof(bm), &bm))
        return NULL;

    const SIZE_T W = bm.bmWidth, H = bm.bmHeight;
    if (W <= 0 || H <= 0 || W > MAXLONG || H > MAXLONG)
        return NULL;

    const SIZE_T srcStride = WIDTHBYTES(W * 24);
    CHeapPtr<BYTE, CLocalAllocator> srcBuf;
    if (!srcBuf.Allocate(srcStride * H))
        return NULL;

    BITMAPINFOHEADER bihSrc = {};
    bihSrc.biSize     = sizeof(bihSrc);
    bihSrc.biWidth    = (LONG)W;
    bihSrc.biHeight   = -(LONG)H; // Top-down
    bihSrc.biPlanes   = 1;
    bihSrc.biBitCount = 24;

    BITMAPINFO biSrc = {};
    biSrc.bmiHeader = bihSrc;

    HDC hScreenDC = GetDC(NULL);
    if (!hScreenDC)
        return NULL;

    BOOL bGot = ((SIZE_T)GetDIBits(hScreenDC, hBitmap, 0, H, srcBuf, &biSrc, DIB_RGB_COLORS) == H);
    ReleaseDC(NULL, hScreenDC);
    if (!bGot)
        return NULL;

    if (nBpp == 24)
    {
        PVOID pBits = NULL;
        HDC hdc = GetDC(NULL);
        HBITMAP hbmResult = CreateDIBSection(hdc, &biSrc, DIB_RGB_COLORS, &pBits, NULL, 0);
        ReleaseDC(NULL, hdc);
        if (hbmResult && pBits)
            CopyMemory(pBits, srcBuf, srcStride * H);
        return hbmResult;
    }

    const INT nColors = 1 << nBpp;
    CHeapPtr<RGBQUAD, CLocalAllocator> palette;
    if (!palette.Allocate(nColors))
        return NULL;

    BuildPalette(nBpp, palette);

    CHeapPtr<BYTE, CLocalAllocator> indexImg;
    if (!indexImg.Allocate(W * H))
        return NULL;
    FloydSteinberg(srcBuf, srcStride, W, H, palette, nColors, indexImg);

    const SIZE_T dstStride = WIDTHBYTES(W * nBpp);
    CHeapPtr<BYTE, CLocalAllocator> dstBuf;
    if (!dstBuf.Allocate(dstStride * H))
        return NULL;
    PackIndexImage(indexImg, W, H, nBpp, dstBuf, dstStride);

    const SIZE_T biBytes = sizeof(BITMAPINFOHEADER) + nColors * sizeof(RGBQUAD);
    CHeapPtr<BYTE, CLocalAllocator> biMem;
    if (!biMem.Allocate(biBytes))
        return NULL;

    PBITMAPINFO pBI = reinterpret_cast<PBITMAPINFO>((PBYTE)biMem);
    pBI->bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
    pBI->bmiHeader.biWidth        = (LONG)W;
    pBI->bmiHeader.biHeight       = -(LONG)H; // Top-down
    pBI->bmiHeader.biPlanes       = 1;
    pBI->bmiHeader.biBitCount     = (WORD)nBpp;
    pBI->bmiHeader.biCompression  = BI_RGB;
    pBI->bmiHeader.biSizeImage    = (DWORD)(dstStride * H);
    pBI->bmiHeader.biClrUsed      = (DWORD)nColors;
    pBI->bmiHeader.biClrImportant = (DWORD)nColors;
    for (INT i = 0; i < nColors; i++)
        pBI->bmiColors[i] = palette[i];

    PVOID pBits = NULL;
    HBITMAP hbmResult = NULL;
    HDC hdc = GetDC(NULL);
    if (hdc)
    {
        hbmResult = CreateDIBSection(hdc, pBI, DIB_RGB_COLORS, &pBits, NULL, 0);
        ReleaseDC(NULL, hdc);
    }

    if (hbmResult && pBits)
        CopyMemory(pBits, dstBuf, (size_t)dstStride * H);

    return hbmResult;
}
